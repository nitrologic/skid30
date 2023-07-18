// this version is deprecated see skidtool.cpp for more recent action

#define RUN_CYCLES_PER_TICK 1024
//128
//1024

#include <cassert>

#ifdef WIN32
#include <windows.h>
#include <conio.h>
#include <synchapi.h>

int getch2() {
	if (_kbhit()) {
		return getch();
	}
	return -1;
}
void usleep(int micros) {
	int millis = micros / 1e6;
	Sleep(millis);
}
int millis() {
	return GetTickCount();
}
#else

#include "tty_getch.h"
#include <time.h>

#ifndef LINUX

int millis(){
	uint64_t t=clock_gettime_nsec_np(CLOCK_UPTIME_RAW);
	return (int)(t/1e6);
}
#else

int millis(){
	timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
	return spec.tv_sec*1000+spec.tv_nsec/1e6;
}
#endif
#endif

#include <assert.h>
#include <sstream>
#include <vector>

#include "machine.h"

// acid500 monitor

// console log output helpers

// ctrl shift period

struct block {
	void* raw;
	block(size_t size) {
		raw = malloc(size);
	}
	~block() {
		free(raw);
	}
};

// address is 24 bit 6hexdigit

// for validation purposes only
#ifdef KICKSTART
rom16 kickstart(0xf80000, 0xff80000, "../../media/kick.rom", 524288 ); // 512K
#endif

ram16 chipmem(0x000000, 0xff00000, 0x100000);	// 2MB
chipset16 chipset(0xdff000, 0xffff000, 0x100); // 256 16 bit registers dff000..dff1fe 
interface8 cia_a(0xbfe000, 0xffff000, 0x1000); // 256 16 bit registers dff000..dff1fe 
interface8 cia_b(0xbfd000, 0xffff000, 0x1000); // 256 16 bit registers dff000..dff1fe 

// chinnamasta soc

extern "C" {
#include "musashi/m68k.h"
#include "musashi/m68kcpu.h"
#include "musashi/m68kops.h"
#include "musashi/sim.h"
#include "musashi/mmu.h"
#include "musashi/m68kops.h"
}

struct MemEvent {
	int time;
	int address; // bit31 - R=0 W=1 bit 30-29 - byte,short,long 
	int data;
	int pc;

	MemEvent(int t32,int a32, int d32, int pc32) :time(t32), address(a32), data(d32), pc(pc32) {}
};

typedef std::vector<MemEvent> MemEvents;

//int m68k_execute(int num_cycles)

const char readwrite[] = { 'R','W' };
const char longshortbyte[] = {'l','s','b','?'};

const int DumpLimit=5000;

struct acid68000 {

	int tick=0;

	int memoryError = 0;

	memory32* mem;
	MemEvents memlog;

	void log_bus(int readwrite, int byteshortlong, int address, int value) {
		bool enable=(readwrite)?(mem->flags&2):(mem->flags&1);
		int star=(mem->flags&4)?1:0;
		int err = (address == memoryError)?1:0;
		if(enable||err){
			int a32 = ((star|err) << 31) | (readwrite << 30) | (byteshortlong << 28) | (address & 0xffffff);
			int pc=readRegister(16);
			memlog.emplace_back(tick, a32, value, pc);
		}
	}

	void dumplog(int max) {
		int n = memlog.size();
		if(max==0) max=(n<DumpLimit)?n:DumpLimit;
		int begin = n - max;//5;
		if (begin < 0) begin = 0;
		for (int i = begin; i < n; i++) {
			MemEvent& e = memlog[i];
			int t32 = e.time;
			int a32 = e.address;
			int d32 = e.data;
			int pc32 = e.pc;
			int star=(a32 >> 31) & 1;
			int rw = (a32 >> 30) & 1;
			int opsize = (a32 >> 28) & 3;
			int a24 = a32 & 0xffffff;

			// tick: R/W l/s/b address data 

			writeData32(t32);
			writeSpace();
			writeChar(readwrite[rw]);
			writeSpace();
			writeChar(longshortbyte[opsize]);
			writeSpace();
			writeAddress(a24);
			writeSpace();

			switch (opsize) {
			case 0:
				writeData32(d32);
				break;
			case 1:
				writeData16(d32);
				break;
			case 2:
				writeData8(d32);
				break;
			}
			if(star) {
				writeSpace();
				writeAddress(pc32);
			}
			writeEOL();
		}

	}

	void writeRegister(int reg, int value) {
		m68k_set_reg((m68k_register_t)reg, (unsigned int)value);
	}

	int readRegister(int reg) {
		void* context = 0;
		unsigned int value = m68k_get_reg(context, (m68k_register_t) reg);
		return (int)value;
	}

	int decode(int physicalAddress) {
#ifdef KICKSTART		
		if ((physicalAddress & kickstart.mask) == kickstart.physical) {
			mem = &kickstart;
			return physicalAddress & (~kickstart.mask);
		}
#endif		
		if ((physicalAddress & chipset.mask) == chipset.physical) {
			mem = &chipset;
			return physicalAddress & (~chipset.mask);
		}
		if ((physicalAddress & chipmem.mask) == chipmem.physical) {
			mem = &chipmem;
			return physicalAddress & (~chipmem.mask);
		}
		if ((physicalAddress & cia_a.mask) == cia_a.physical) {
			mem = &cia_a;
			return physicalAddress & (~cia_a.mask);
		}
		if ((physicalAddress & cia_b.mask) == cia_b.physical) {
			mem = &cia_b;
			return physicalAddress & (~cia_b.mask);
		}

		memoryError = physicalAddress;
//		writeAddress(physicalAddress);

		return -1;
	}

	int read8(int physicalAddress) {
		int address=decode(physicalAddress);
		if (address < 0) {
			log_bus(0, 2, physicalAddress, 0);
			return 0; // free pass hackers are us
		}
		int value=mem->read8(address);
		log_bus(0, 2, physicalAddress, value);
		return value;
	}

	int read16(int a32) {
		int qbit = a32 & 0x80000000;
		int physicalAddress = a32 & 0xffffff;
		int address = decode(physicalAddress);
		if (address < 0) {
			log_bus(0, 1, physicalAddress, 0);
			return 0; // free pass hackers are us
		}
		int value = mem->read16(address);
		if(qbit==0) log_bus(0, 1, physicalAddress, value);
		return value;
	}

	int read32(int a32) {
		int qbit = a32 & 0x80000000;
		int physicalAddress = a32 & 0xffffff;
		int address = decode(physicalAddress);
		if (address < 0) {
			log_bus(0, 0, physicalAddress, 0);
			return 0; // free pass hackers are us
		}
		int value = mem->read32(address);
		if(qbit==0) log_bus(0, 0, physicalAddress, value);
		return value;
	}

	void write8(int physicalAddress, int value) {
		int address = decode(physicalAddress);
		log_bus(1, 2, physicalAddress, value);
		if(!memoryError) mem->write8(address, value);
	}

	void write16(int physicalAddress, int value) {
		int address = decode(physicalAddress);
		log_bus(1, 1, physicalAddress, value);
		mem->write16(address, value);
	}

	void write32(int physicalAddress, int value) {
		int address = decode(physicalAddress);
		log_bus(1, 0, physicalAddress, value);
		mem->write32(address, value);
	}

	void qwrite32(int physicalAddress, int value) {
		int address = decode(physicalAddress);
		mem->write32(address, value);
	}
	void qwrite16(int physicalAddress, int value) {
		int address = decode(physicalAddress);
		mem->write16(address, value);
	}
};

acid68000 acid500;


// musashi entry points to acid cpu address bus

unsigned int mmu_read_byte(unsigned int address){
	return acid500.read8(address);
}
unsigned int mmu_read_word(unsigned int address){
	return acid500.read16(address);
}
unsigned int mmu_read_long(unsigned int address){
	return acid500.read32(address);
}
void mmu_write_byte(unsigned int address, unsigned int value){
	acid500.write8(address,value);
}
void mmu_write_word(unsigned int address, unsigned int value){
	acid500.write16(address,value);
}
void mmu_write_long(unsigned int address, unsigned int value){
	acid500.write32(address,value);
}

uint  read_imm_8(void) { 
	return 0; 
}
uint  read_imm_16(void) { 
	return 0; 
}
uint  read_imm_32(void) { 
	return 0; 
}

// address with qbit so does not clog up bus log

unsigned int cpu_read_word_dasm(unsigned int address)
{
	return acid500.read16(address | 0x80000000);
}

unsigned int cpu_read_long_dasm(unsigned int address)
{
	return acid500.read32(address | 0x80000000);
}




typedef std::vector<u16> Chunk;

// write to acid500

void loadHunk(std::string path,int physical) {
	writeString("Reading Hunk from ");
	writeString(path);
	writeEOL();

	filedecoder fd(path);

	// page 256 hunk__header

	u16 h0 = fd.readBigShort();
	u16 h1 = fd.readBigShort();
	bool magic = (h0 == 0) && (h1 == 1011);

	if (!magic) {
		writeString("loadHunk fail magic");
		writeEOL();
		return;
	}

	while (!fd.eof()) {
		u32 l0 = fd.readBigInt();
		if (l0 == 0) break;
		for (int i = 0; i < l0; i++) {
			u32 name = fd.readBigInt();
			writeNamedInt("hunk ",i);
			writeSpace();
			writeNamedInt("name ",name);
			writeEOL();
		}
	}

	if (fd.eof()) {
		std::cout << "unexpected data remaining in file" << std::endl;
	}

	u32 tablsize = fd.readBigInt();
	u32 firsthunkf = fd.readBigInt();
	u32 lasthunkl = fd.readBigInt();
	int n = lasthunkl - firsthunkf + 1;

	// warning - word addressing ahead

	std::vector<int> sizeWords(n);
	std::vector<int> offsetWords(n);
	int totalWords = 0;
//	std::vector<std::vector<u32>> hunks(n);
	for (int i = 0; i < n; i++) {
		u32 hunkSize=fd.readBigInt();

		int bits = (hunkSize >> 30) & 3;
		hunkSize &= 0x3fffffff;

		if (bits == 3) {
			hunkSize = fd.readBigInt();
		}

		u32 hunkSizeWords = hunkSize * 2;
		sizeWords[i] = hunkSizeWords;
		offsetWords[i] = totalWords;
		totalWords += hunkSizeWords;
		if (hunkSize == 0) {
			std::cout << "todo: support empty bss hunks" << std::endl;
		}
	}
	writeNamedInt("total words", totalWords);
	writeEOL();

	Chunk chunk(totalWords);

	writeNamedInt("hunk count", n);
	writeEOL();

	int index = 0;
	bool parseHunk = true;
	while (parseHunk) 
	{
		if (fd.eof()) {
			std::cout << "unexpected end of file" << std::endl;
			break;
		}
		u32 type = fd.readBigInt();

		type &= 0xffff;

		switch (type) {
		case 1001: // HUNK___CODE
		{			
			std::cout << "HUNK_CODE" << std::endl;
			int target = offsetWords[index];

			u32 size = fd.readBigInt();
			//		writeInt(code);
			//		writeSpace();
			for (int j = 0; j < size; j++) {
				int l32 = fd.readBigInt();	// make little endian?
				chunk[target + j*2 + 0] = (l32 >> 16);
				chunk[target + j*2 + 1] = (l32 & 0xffff);

				if (j < 8) {
					writeNamedInt("$",l32);
					writeSpace();
				}
			}
			writeEOL();
			break;
		}
		case 1002: //HUNK__DATA
		{
			std::cout << "HUNK_DATA" << std::endl;
			int target = offsetWords[index];
			u32 count = fd.readBigInt();
			for (int i = 0; i < count; i++) {
				int l32 = fd.readBigInt();
				chunk[target+i*2+0] = (l32 >> 16);
				chunk[target+i*2+1] = (l32 & 0xffff);
			}
			break;
		}
		case 1004:	//RELOC32
		{
			std::cout << "HUNK_RELOC32" << std::endl;
//			std::vector<u32>& hunk = hunks[index];
			while (true) {
				u32 number = fd.readBigInt();
				if (number == 0)
					break;
				int current = offsetWords[index];
				u32 index32 = fd.readBigInt();
				int target = offsetWords[index32];
				u32 reloc32 = physical + target * 2;
				for (int i = 0; i < number; i++) {
					u32 offset = fd.readBigInt();
					u32 word = current + offset / 2;
					u32 loc32=(chunk[word] << 16) | (chunk[word + 1]&0xfff);
					loc32 = loc32 + reloc32;
					chunk[word] = (loc32 >> 16);
					chunk[word + 1] = loc32 & 0xffff;
				}
			}
			break;
		}
		case 1010:	//HUNK__END
		{
			std::cout << "HUNK_END" << std::endl;
			index++;
			if (index == n) {
				parseHunk = false;
			}
			break;
		}
		default:
		{
			std::cout << "type " << std::hex << type << " not supported " << std::endl;
			assert(false);
			break;
		}
		}
	}
	if(!fd.eof()){
		std::cout << "unexpected data remaining in file" << std::endl;
		u32 i = 0;
		// debug section?
		while (!fd.eof()) {
			u16 h0 = fd.readBigShort();
			writeIndex(i++);
			writeSpace();
			writeShort(h0);
			writeEOL();
		}

	}

	writeString("hunks parsed");
	writeEOL();

	for (int i = 0; i < totalWords; i++) {
		acid500.qwrite16(physical+i*2,chunk[i]);
	}

	return;
}



void make_hex(char* buff, unsigned int pc, unsigned int length)
{
	char* ptr = buff;

	for (; length > 0; length -= 2)
	{
		sprintf(ptr, "%04x", cpu_read_word_dasm(pc));
		pc += 2;
		ptr += 4;
		if (length > 2)
			*ptr++ = ' ';
	}
}
void disassemble(int pc,int count)
{
	unsigned int instr_size;
	char buff[100];
	char buff2[100];

	while (count--)
	{
		instr_size = m68k_disassemble(buff, pc, M68K_CPU_TYPE_68000);
		make_hex(buff2, pc, instr_size);
		printf("%06x: %-20s: %s \033[K\n", pc, buff2, buff);


		pc += instr_size;
	}
	writeEOL();
}

const char* title = "☰☰☰☰☰☰☰☰☰☰ ACID500 monitor";
const char* help = "[s]tep [c]ontinue [pause] [r]eset [h]ome [q]uit";

void debugCode(int pc24) {
	int key = 0;
	int run = 0;
	int err = 0;
	bool refresh=true;

	acid500.qwrite32(0, 0x400); //sp
	acid500.qwrite32(4, 0xac1d0000); //exec
	acid500.qwrite32(8, pc24); //pc

//	acid500.writeRegister(16, pc24);

	int pc = pc24;//acid500.readRegister(16);

	writeClear();

	m68k_init();
	m68k_set_cpu_type(M68K_CPU_TYPE_68000);
	m68k_pulse_reset();

	// refresh has 20 milli second sanity delay
	int drawtime=millis();
	while (true) {
		int t=millis();
		int elapsed=t-drawtime;
		if(refresh && elapsed>19){
			writeHome();
			writeString(title);
			writeEOL();
			writeEOL();
			writeNamedInt("key", key);
			writeEOL();
			writeNamedInt("error", acid500.memoryError);
			writeEOL();
			writeNamedInt("elapsed", elapsed);
			writeEOL();
			writeNamedInt("TICK", acid500.tick);
			writeEOL();
			writeEOL();

			writeNamedInt("PC", pc);
			writeEOL();

			for (int i = 0; i < 2; i++) {
				for (int j = 0; j < 8; j++) {
					int r = i * 8 + j;
					int r32=acid500.readRegister(r);
					writeChar(i == 0 ? 'D' : 'A');
					writeChar('0' + j);
					writeNamedInt("",r32);
					if(j==3)
						writeEOL();
					else
						writeSpace();
				}
				writeEOL();
			}
			writeEOL();

			disassemble(pc, 6);

			acid500.dumplog(5);

			writeString(help);
			writeEOL();

			drawtime=millis();
			refresh=false;
		}

//		key=run?0:tty_getch();
//		key=tty_getch();
//		key=getch();
		key=getch2();

		if (key == 'q') break;

		if(key=='h'){
			writeClear();
			refresh=true;
		}

		if (key == 's') {
			m68k_execute(1);
			acid500.tick++;
			refresh=true;
		}

//		if (key == 'r') {
//			run = 1 - run;
//		}

		if(key=='c'){
			run=1;
		}
		if(key=='p'){
			run=0;
		}

		if (run) {
			int n=RUN_CYCLES_PER_TICK;
			m68k_execute(n);
			acid500.tick+=n; // may error early
			refresh=true;
			if (acid500.memoryError) {
				run = false;
				err = acid500.memoryError;
			}

		}

		pc = acid500.readRegister(16);

//		usleep(1000);
	}


	acid500.dumplog(0);

}

int main() {

#ifdef WIN32
	SetConsoleOutputCP(CP_UTF8);
//	SetConsoleCP(CP_UTF8);
#endif

#ifdef NCURSES
	initscr();
	timeout(200);
#endif
	std::cout << "skidtool 0.1" << std::endl;

//	const char *iff="C:\\nitrologic\\skid30\\maps\\format.iff";
//	const char* iff = "C:\\nitrologic\\skid30\\archive\\titlescreen.iff";
//	loadIFF(iff);
//audio
//	const char* iff = "C:\\nitrologic\\skid30\\archive\\amigademogfx\\skid.iff";
//	const char* iff = "C:\\nitrologic\\skid30\\archive\\amigademogfx\\impact.iff";
//unknown
//	const char* amiga_binary = "C:\\nitrologic\\skid30\\archive\\genam2";
// triple chunk issue
//	const char* amiga_binary = "../../archive/game";
//	const char* amiga_binary = "../../archive/devpac";
//	const char* amiga_binary = "../../archive/dp";

//amiga 2 chunk hunks

//	const char* amiga_binary = "../../archive/lha";
	const char* amiga_binary = "../../archive/virus";
//	const char* amiga_binary = "../../archive/blitz2/blitz2";
//	const char* amiga_binary = "../../archive/blitz2/ted";
	loadHunk(amiga_binary,0x2000);
	getch();
//	disassemble(0x2000, 6);
	debugCode(0x2000);
//	debugCode(0xf800d2);

/*
move.l #$aaaaaaaa,d5
nop
moveq #1,d1
jmp 0
*/

/*
	u16 code[] = { 0,0,0,0, 0x2a3c,0xaaaa,0xaaaa,0x4e71,0x7201,0x4ef9,0,0 };

	for (int i = 0; i < 10; i++) {
		u16 w = code[i];
		cpu_write_word_dasm(i*2,w);
	}
*/
	// test it on kickstart rom
#ifdef disassembleRom
	disassemble(0xf80000,4);
	disassemble(0xf800d2, 16);
#endif

//	disassemble_program();

//#define runcode
#ifdef runcode
	m68k_init();
	m68k_set_cpu_type(M68K_CPU_TYPE_68000);
	m68k_pulse_reset();
	m68k_execute(100000);
#endif
//	input_device_reset();
//	output_device_reset();
//	nmi_device_reset();
//	const char* tag = "";

	return 0;
};
