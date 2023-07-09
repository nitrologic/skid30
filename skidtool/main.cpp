#include <conio.h>
#include <assert.h>
#include <iostream>
#include <sstream>
#include  <iomanip>
#include <vector>
#include "libpng/png.h"
#include <zlib.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

// console log output helpers

void writeByte(int b) {
	std::cout << "0x" << std::setfill('0') << std::setw(2) << std::right << std::hex << b << std::dec;
}
void writeShort(int b) {
	std::cout << "0x" << std::setfill('0') << std::setw(4) << std::right << std::hex << b << std::dec;
}
void writeNamedInt(const char* name, int b) {
	std::cout << name << " " << std::setfill('0') << std::setw(8) << std::right << std::hex << b << std::dec;
}
void writeAddress(int b) {
	std::cout << std::setfill('0') << std::setw(6) << std::right << std::hex << (b&0xffffff) << std::dec;
}
void writeData32(int b) {
	std::cout << std::setfill('0') << std::setw(8) << std::right << std::hex << b << std::dec;
}
void writeData16(int b) {
	std::cout << std::setfill('0') << std::setw(4) << std::right << std::hex << (b&0xffff) << std::dec;
}
void writeData8(int b) {
	std::cout << std::setfill('0') << std::setw(2) << std::right << std::hex << (b&0xff) << std::dec;
}
void writeHome() {
	std::cout << "\033[H" << std::flush;
}
void writeClear() {
	std::cout << "\033[2J" << "\033[H" << std::flush;
}

void writeEOL() {
	std::cout << "\033[K" << std::endl;
}
void writeSpace() {
	std::cout << " ";
}
void writeChar(int c) {
	std::cout << (char)c;
}
void writeString(std::string s) {
	std::cout << s;
}
void writeIndex(int i) {
	std::cout << i;
}
void writeCC4Big(int tag) {
	for (int i = 0; i < 4; i++) {
		int b = (tag >> ((3 - i) * 8)) & 0xff;
		if (b < 32 || b>127)
			b = '#';
		std::cout << (char)(b);
	}
}


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

struct filedecoder {

    FILE *f;

	filedecoder(std::string s) {
		f = fopen(s.c_str(), "rb");
	}

	u8 readByte() {
		u8 result;
		fread(&result, sizeof(u8), 1, f);
		return result;
	}

	u16 readShort() {
        u16 result;
        fread(&result,sizeof(u16),1,f);
        return result;
	}

	u32 readInt() {
		u32 result;
		fread(&result, sizeof(u32), 1, f);
		return result;
	}

	u16 readBigShort() {
		u16 big = readShort();
		u16 little = ((big << 8) & 0xff00) | ((big >> 8) & 0xff);
		return little;
	}

	u32 readBigInt() {
		u32 big=readInt();
		u32 little = ((big << 24) & 0xff000000) | ((big << 8) & 0xff0000) | ((big >> 8) & 0xff00) | ((big >> 24) & 0xff);
		return little;
	}

	// skip is forwards only

	void skip(int bytes) {
		if (bytes>0) {
			fseek(f, bytes, SEEK_CUR);
		}
	}

	bool eof() {
		return feof(f)!=0;
	}

};


// physical address and mask == physical address

// big endian byte order

struct memory32 {
	u32 physical;
	u32 mask;

	memory32(u32 phys, u32 bits) : physical(phys), mask(bits) {
	}

	// address interface is local with physical bits already removed

	virtual int read16(int address) {
		return 0;
	}

	virtual void write16(int address, int value) {

	}

	// little endian helpers

	virtual int read8(int address) {
		int odd = address & 1;
		int word = read16(address - odd);
		word >>= 8 * odd;
		return word;
	}

	virtual int read32(int address) {
		int w0 = read16(address);
		int w1 = read16(address+2);
//		return (w1 << 16) | (w0 & 0xffff);
		return (w0 << 16) | (w1 & 0xffff);
	}

	virtual void write8(int address, int value) {
		int odd = address & 1;
		address -= odd;
		int word = read16(address);
		if (odd) {
			word = (word & 0xff) | (value << 8);
		}
		else {
			word = (word & 0xff00) | (value & 0xff);
		}
		write16(address,word);
	}

	virtual void write32(int address, int value) {
//		write16(address, value);
//		write16(address+2, (value>>16));
		write16(address, (value>>16));
		write16(address + 2, (value &0xffff));
	}
};

struct rom16 : memory32 {
	std::vector<u16> shorts;
	rom16(u32 physical, u32 mask, std::string path,int wordCount):memory32(physical,mask),shorts(wordCount) {
		filedecoder fd(path);
		for (int i = 0; i < wordCount; i++) {
			shorts[i] = fd.readBigShort();
//			shorts[i] = fd.readShort();
		}
		if (!fd.eof()) {
			std::cout << "unexpected end of rom16 file" << std::endl;
		}
	}
	virtual int read16(int address) {
		return shorts[address>>1];
	}
};

struct ram16 : memory32 {
	std::vector<u16> shorts;
	ram16(u32 p,u32 m, int wordCount) : memory32(p,m), shorts(wordCount) {
	}
	virtual void write16(int address,int value) {
		shorts[address>>1]=value;
	}
	virtual int read16(int address) {
		return shorts[address>>1];
	}
};

// a headless paula denise agnus chipset

struct chipset16 : memory32 {
	std::vector<u16> shorts;
	chipset16(u32 p, u32 m, int wordCount) : memory32(p, m), shorts(wordCount) {
	}
	virtual void write16(int address, int value) {
		shorts[address >> 1] = value;
	}
	virtual int read16(int address) {
		return shorts[address >> 1];
	}
};

// address is 24 bit 6hexdigit

rom16 kickstart(0xf80000, 0xf80000, "C:\\nitrologic\\skid30\\media\\kick.rom", 524288 / 2); // 512K
ram16 chipmem(0x000000, 0xfe00000, 0x100000);	// 2MB
chipset16 chipset(0xdff000, 0xffff000, 0x100); // 256 16 bit registers dff000..dff1fe 

// chinnamasta soc



#define musashi

#ifdef musashi

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

	MemEvent(int t32,int a32, int d32) :time(t32), address(a32), data(d32) {}
};

typedef std::vector<MemEvent> MemEvents;

//int m68k_execute(int num_cycles)

const char readwrite[] = { 'r','w' };
const char longshortbyte[] = {'l','w','b','?'};

struct acid68000 {

	int tick=0;
	memory32* mem;
	MemEvents memlog;

	void log_bus(int readwrite, int byteshortlong, int address, int value) {
		int a32 = (readwrite << 30) | (byteshortlong << 28) | (address & 0xffffff);
		memlog.emplace_back(tick, a32, value);
	}

	void dumplog() {
		int n = memlog.size();
		int begin = n - 5;
		if (begin < 0) begin = 0;
		for (int i = begin; i < n; i++) {
			MemEvent& e = memlog[i];
			int t32 = e.time;
			int a32 = e.address;
			int d32 = e.data;
			int rw = (a32 >> 30) & 1;
			int opsize = (a32 >> 28) & 3;

			writeIndex(t32);
			writeSpace();
			writeChar(readwrite[rw]);
			writeSpace();
			writeChar(longshortbyte[opsize]);
			writeSpace();
			writeAddress(a32);
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
		if ((physicalAddress & kickstart.mask) == kickstart.physical) {
			mem = &kickstart;
			return physicalAddress & (~kickstart.mask);
		}
		if ((physicalAddress & chipmem.mask) == chipmem.physical) {
			mem = &chipmem;
			return physicalAddress & (~chipmem.mask);
		}
		if ((physicalAddress & chipset.mask) == chipset.physical) {
			mem = &chipset;
			return physicalAddress & (~chipset.mask);
		}
		return -1;
	}

	int read8(int physicalAddress) {
		int address=decode(physicalAddress);
		int value=mem->read8(address);
		log_bus(0, 2, physicalAddress, value);
		return value;
	}
	int read16(int a32) {
		int qbit = a32 & 0x80000000;
		int physicalAddress = a32 & 0xffffff;
		int address = decode(physicalAddress);
		int value = mem->read16(address);
		if(qbit==0) log_bus(0, 1, physicalAddress, value);
		return value;
	}
	int read32(int a32) {
		int qbit = a32 & 0x80000000;
		int physicalAddress = a32 & 0xffffff;
		int address = decode(physicalAddress);
		int value = mem->read32(address);
		if(qbit==0) log_bus(0, 0, physicalAddress, value);
		return value;
	}
	void write8(int physicalAddress, int value) {
		log_bus(1, 2, physicalAddress, value);
		int address = decode(physicalAddress);
		mem->write8(address, value);
	}
	void write16(int physicalAddress, int value) {
		log_bus(1, 1, physicalAddress, value);
		int address = decode(physicalAddress);
		mem->write16(address, value);
	}
	void write32(int physicalAddress, int value) {
		log_bus(1, 0, physicalAddress, value);
		int address = decode(physicalAddress);
		mem->write32(address, value);
	}
	void qwrite32(int physicalAddress, int value) {
		int address = decode(physicalAddress);
		mem->write32(address, value);
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





// EA DPAINT IFF ILBM FILE FORMAT
// http://www.etwright.org/lwsdk/docs/filefmts/ilbm.html

const int FORM=0x464f524d;
const int ILBM=0x494c424d;
const int BMHD=0x424d4844;
const int BODY=0x424f4459;
const int CMAP=0x434d4150;
const int CRNG=0x43524e47;
const int CAMG=0x43414d47;
const int DPPS=0x44505053;

#define CAMG_HAM 0x800   /* hold and modify */
#define CAMG_EHB 0x80    /* extra halfbrite */

void loadIFF(std::string path){
	writeString("Reading IFF from ");
	writeString(path);
	writeEOL();
	filedecoder fd(path);

	int form = fd.readBigInt();
	if (form != FORM){
		std::cout << "expecting FORM at start of IFF ILBM" << std::endl;
		return;
	}
	int fsize=fd.readBigInt();
	int ilbm=fd.readBigInt();
	if(ilbm!=ILBM){
		writeCC4Big(ilbm);
		std::cout << "expecting ILBM at start of IFF ILBM" << std::endl;
		return;
	}
	int w = 0;
	int h = 0;
	int planes = 0;
	int mask = 0;
	int comp = 0;
	int xorg = 0;
	int yorg = 0;
	int trans = 0;
	int xasp = 0;
	int yasp = 0;
	int pagew = 0;
	int pageh = 0;
	fsize -= 4;

	while (fsize > 0) {
		int token= fd.readBigInt();
		int tsize= fd.readBigInt();

		switch (token) {

		case BODY:{

			int depth = planes;
			int c = 0; 
			int pos = 0; 
			int wid = ((w + 15) >> 4) << 1; 
			int bm = 0;
			if (comp == 0)
			{
				for (int y = 0; y < h; y++)
				{
					int val = 0x10000; 
					if (depth <= 8) val = 1;
					for (int d = 0; d < depth; d++)
					{
						for (int x = 0; x < wid; x++)
						{
							int b = fd.readByte();
							for (int j = 128; j > 0; j = j >> 1)
							{
//								if ((j & b) == j) can.pix[pos] |= val;
								pos++;
							}
						}
						pos -= wid * 8;
						val = val << 1;
						if (val == 0x1000000) { val = 0x100; }
						if (val == 0x10000) { val = 1; }
					}
//					pos += can.modulo;
				}
				break;
			}

			for (int y = 0; y < h; y++)
			{
				int val = 0x10000; 
				if (depth <= 8) val = 1;
				for (int d = 0; d < depth; d++)
				{
					for (int x = 0; x < wid;)
					{
						int b = fd.readByte();				//*iff++
						if (b == -128) continue;
						if (b < 0)
						{
							b = 1 - b;
							c = fd.readByte();
							for (int i = 0; i < b; i++) {
								for (int j = 128; j > 0; j = j >> 1) {
									if ((j & c) == j) {
//										can.pix[pos] |= val; 
										pos++;
									}
								}
							}
							x += b;
						}
						else
						{
							b = 1 + b;
							for (int i = 0; i < b; i++){
								c = fd.readByte();
								for (int j = 128; j > 0; j = j >> 1) { 
//									if ((j & c) == j) can.pix[pos] |= val; 
									pos++; 
								}
							}
							x += b;
						}
					}
					pos -= wid * 8;
					val = val << 1;
					if (val == 0x1000000) { val = 0x100; }
					if (val == 0x10000) { val = 1; }
				}
//				pos += can.modulo;
			}

//			fd.skip(tsize);
		}
			break;
		case DPPS:
			fd.skip(tsize);
			break;
		case CRNG: {
			int pad = fd.readBigShort();
			int rate = fd.readBigShort();
			int flags = fd.readBigShort();
			int low = fd.readByte();
			int high = fd.readByte();
			fd.skip(tsize-8);
		}
			break;
		case CMAP: {
			int n = 1 << planes;
			for (int i = 0; i < n; i++) {
				int r = fd.readByte();
				int g = fd.readByte();
				int b = fd.readByte();
			}
			fd.skip(tsize-3*n);
		}
			break;
		case CAMG: {
			int camg = fd.readBigInt();
			fd.skip(tsize-4);
		}
			 break;
		case BMHD: {
//			fd.skip(16);
			w = fd.readBigShort();
			h = fd.readBigShort();

			xorg = fd.readBigShort();
			yorg = fd.readBigShort();
				
			planes = fd.readByte();
			mask = fd.readByte();
			
			comp = fd.readByte();
			fd.skip(1);

			trans = fd.readBigShort();
			xasp = fd.readByte();
			yasp = fd.readByte();

			pagew = fd.readBigShort();
			pageh = fd.readBigShort();

			//can = new canvas(); can.init(w, h);

			fd.skip(tsize - 20);
		}
			break;
		default:
//			writeString("FORM ILBM TOKEN ");
			writeCC4Big(token);
			writeEOL();
			fd.skip(tsize);
		}
		fsize -= (tsize + 8);
	}

	for (int i = 0; i < 64; i++) {
		int x = fd.readByte();
		std::cout << x << " " << (char)x << std::endl;
	}

	u16 h0 = fd.readBigShort();
	u16 h1 = fd.readBigShort();
	bool magic = (h0 == 0) && (h1 == 1011);

}

typedef std::vector<int> Chunk;

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

	std::vector<int> sizes(n);
	std::vector<int> offsets(n);
	int total = 0;
//	std::vector<std::vector<u32>> hunks(n);
	for (int i = 0; i < n; i++) {
		u32 hunkSize=fd.readBigInt();
		sizes[i] = hunkSize;
		offsets[i] = total;
		total += hunkSize;
		if (hunkSize == 0) {
			std::cout << "todo: support empty bss hunks" << std::endl;
		}
	}
	writeNamedInt("total", total);
	writeEOL();

	Chunk chunk(total);

	writeString("hunkCount:");
	writeIndex(n);
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
			int chunkOffset = offsets[index];

			u32 size = fd.readBigInt();
			//		writeInt(code);
			//		writeSpace();
			for (int j = 0; j < size; j++) {
//				int l = fd.readInt();	// make little endian?
				int l = fd.readBigInt();	// make little endian?
				chunk[chunkOffset+j] = l;
				if (j < 8) {
					writeNamedInt("$",l);
					writeSpace();
				}
			}
			writeEOL();
			break;
		}
		case 1002: //HUNK__DATA
		{
			std::cout << "HUNK_DATA" << std::endl;
			int chunkOffset = offsets[index];

			u32 count = fd.readBigInt();
			//			assert(count == hunk.size());
			for (int i = 0; i < count; i++) {
				int l = fd.readBigInt();
				chunk[chunkOffset+i] = l;
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
				u32 index32 = fd.readBigInt();
				int chunkOffset = offsets[index];

				int targetOffset = offsets[index32];

				for (int i = 0; i < number; i++) {
					u32 offset = fd.readBigInt();

					int o = chunkOffset + (offset >> 2);
					chunk[o]=chunk[o] + (physical + targetOffset);
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

	for (int i = 0; i < total; i++) {
		acid500.qwrite32(physical+i*4,chunk[i]);
	}

	return;
}

const std::string bitplane16(u16 s){
	std::stringstream ss;
	for (int i = 0; i < 16; i++) {
		char c = (s & 1) ? '#' : ' ';
		ss << c;
		s >>= 1;
	}
	return ss.str();
}


const std::string dualbitplane32(u32 s0,u32 s1) {
	std::stringstream ss;
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 8; j++) {
			int bit = (1 << (7 - j)) << (i * 8);

			int bits = ((s0 & bit) ? 2 : 0 ) + ((s1 & bit) ? 1 : 0);

			char c = 48 + bits;	// (s & bit) ? '#' : ' ';
			ss << c;
		}
	}
	return ss.str();
}


const std::string bitplane32(u32 s) {
	std::stringstream ss;
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 8; j++) {
			int bit = (1 << (7-j)) << (i * 8);
			char c = (s & bit) ? '#' : ' ';
			ss << c;
		}
	}
	return ss.str();
}

const std::string bitplane64(u32 s0, u32 s1, u32 s2, u32 s3) {
	return bitplane32(s0) + bitplane32(s1);
}

const std::string bitplane32be(u32 s) {
	std::stringstream ss;
	for (int i = 0; i < 32; i++) {
		char c = (s & 1) ? '#' : ' ';
		ss << c;
		s >>= 1;
	}
	return ss.str();
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

const char* title = "acid500 monitor";
const char* help = "[s]tep [q]uit";

void debugCode(int pc) {
	int key = 0;

	acid500.qwrite32(0, 0x400); //sp
	acid500.qwrite32(4, 0x2000); //pc

	writeClear();

	m68k_init();
	m68k_set_cpu_type(M68K_CPU_TYPE_68000);
	m68k_pulse_reset();

	while (true) {
		writeHome();
		writeString(title);
		writeEOL();
		writeEOL();
		writeNamedInt("key", key);
		writeEOL();
		writeNamedInt("TICK", acid500.tick);
		writeEOL();

		int pc = acid500.readRegister(16);
		writeNamedInt("PC", pc);
		writeEOL();

		for (int i = 0; i < 2; i++) {
			for (int j = 0; j < 8; j++) {
				int r = i * 8 + j;
				int r32=acid500.readRegister(r);
				writeChar(i == 0 ? 'D' : 'A');
				writeChar('0' + j);
				writeNamedInt("",r32);
				writeSpace();
			}
			writeEOL();
		}
		writeEOL();

		disassemble(pc, 6);

		acid500.dumplog();

		writeString(help);
		writeEOL();

		key=getch();
		if (key == 'q') break;
		if (key == 's') {
			m68k_execute(1);
			acid500.tick++;
		}

	}

}

int main() {
	std::cout << "skidtool 0.1" << std::endl;

	const char *iff="C:\\nitrologic\\skid30\\maps\\format.iff";
//	const char* iff = "C:\\nitrologic\\skid30\\archive\\titlescreen.iff";
//	loadIFF(iff);
//audio
//	const char* iff = "C:\\nitrologic\\skid30\\archive\\amigademogfx\\skid.iff";
//	const char* iff = "C:\\nitrologic\\skid30\\archive\\amigademogfx\\impact.iff";
//unknown
//	const char* amiga_binary = "C:\\nitrologic\\skid30\\archive\\genam2";
// triple chunk issue
//	const char* amiga_binary = "C:\\nitrologic\\skid30\\archive\\game";
//  const char* amiga_binary = "C:\\nitrologic\\skid30\\archive\\devpac";
//	const char* amiga_binary = "C:\\nitrologic\\skid30\\archive\\dp";

//amiga 2 chunk hunks
	const char* amiga_binary = "C:\\nitrologic\\skid30\\archive\\virus";
//	const char* amiga_binary = "C:\\nitrologic\\skid30\\archive\\blitz2\\blitz2";
//	const char* amiga_binary = "C:\\nitrologic\\skid30\\archive\\blitz2\\ted";
//  const char* amiga_binary = "C:\\nitrologic\\skid30\\archive\\lha";

	loadHunk(amiga_binary,0x2000);
//	disassemble(0x2000, 6);



	debugCode(0x2000);


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

#else

int decodeCar(std::string src, std::string dest) {
	filedecoder fd(src);
	Image cels(2048, 800, 8, 4);

	for (int i = 0; i < 800; i++) {

		// .aga frames are 512 bytes = 64 x 8 

		for (int j = 0; j < 32; j++) {
			//u16 s0 = fd.readShort();
			//std::cout << "[" << index << "] " <<  bitplane16(s0) << std::endl;

			u32 i0 = fd.readInt();
			u32 i1 = fd.readInt();
			u32 i2 = fd.readInt();
			u32 i3 = fd.readInt();

			//			std::cout << "[" << index << "] " <<  bitplane32(i0) << bitplane32(i1)  << std::endl;
			std::cout << "[" << i << "] " << dualbitplane32(i0, i2) << dualbitplane32(i1, i3) << std::endl;

			int celx = i & 31;
			int cely = i >> 5;

			cels.hlin32x2(celx * 64, cely * 32 + j, i0, i2);
			cels.hlin32x2(celx * 64 + 32, cely * 32 + j, i1, i3);
		}

		std::cout << "-------------------------------------------------" << std::endl;


		if (fd.eof())
			break;

	}

	cels.savePNG(dest.c_str());

	return 0;

}

// todo - support compressed aga for caravan and f1 cars


//int palette[] = { 0,0xff000000,0xffffffff,0xff0000ff };
int palette[] = { 0,0xffffffff,0xff000000,0xff0000ff };

// cars are 64x32
// 800 frames is 32x25
// png is 2048 x 800

struct Image {

	int width;
	int height;
	int bitdepth;
	int channels;
	int span;	//int words per line
	size_t size;

	int* pixels = nullptr;

	Image(int w, int h, int d, int c) {
		setSize(w, h, d, c);
	}

	void plot(int x, int y, int c) {
		pixels[y * span + x] = c;
	}

	void hlin32x2(int celx, int cely, u32 s0, u32 s1) {
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 8; j++) {
				int bit = (1 << (7 - j)) << (i * 8);
				int bits = ((s0 & bit) ? 2 : 0) + ((s1 & bit) ? 1 : 0);
				if (bits) plot(celx, cely, palette[bits]);
				celx++;
			}
		}
	}

	void setSize(int w, int h, int d, int c) {
		if (pixels) {
			delete[]pixels;
		}
		width = w;
		height = h;
		bitdepth = d;
		channels = c;
		int bits = bitdepth * channels;
		if (bits == 24) bits = 32; // RGB -> xRGB
		span = ((width * bits) + 31) / 32;
		size = span * height;
		pixels = new int[size];
		if (pixels == 0) {
			std::cout << "pixel allocation failure" << std::endl;
			// throw here
		}
		zero();
	}

	void zero() {
		memset(pixels, 0x00, 4 * span * height);
	}

	bool writePNG(png_structp& png, png_charp license = NULL) {

		int color_type;
		switch (channels) {
		case 4:
			color_type = PNG_COLOR_TYPE_RGB_ALPHA;
			break;
		case 3:
			color_type = PNG_COLOR_TYPE_RGB;
			break;
		case 2:
			color_type = PNG_COLOR_TYPE_GRAY_ALPHA;
			break;
		case 1:
			color_type = PNG_COLOR_TYPE_GRAY;
			break;
		default:
			return false;
		}

		int bits = channels * bitdepth;
		int bit_depth = bitdepth;

		int interlace_method = PNG_INTERLACE_NONE;
		int compression_method = PNG_COMPRESSION_TYPE_DEFAULT;
		int filter_method = PNG_FILTER_TYPE_DEFAULT;

		png_infop info = png_create_info_struct(png);
		png_set_IHDR(png, info, width, height, bit_depth, color_type, interlace_method, compression_method, filter_method);

		if (license) {
			//			int numText = 1;
			//			png_charp License = "License";
			//			png_text text = { -1,License,license,strlen(license),0 };
			//			png_set_text(png, info, &text, 1);
						/*
					{
						int  compression;       // compression value: -1: tEXt, none 0: zTXt, deflate 1: iTXt, none 2: iTXt, deflate
						png_charp key;          // keyword, 1-79 character description of "text"
						png_charp text;         // comment, may be an empty string (ie "") or a NULL pointer
						png_size_t text_length; // length of the text string
						png_size_t itxt_length; // length of the itxt string
						png_charp lang;         // language code, 0-79 characters or a NULL pointer
						png_charp lang_key;     // keyword translated UTF-8 string, 0 or more chars or a NULL pointer
					} png_text;
					*/
		}
		png_write_info(png, info);	//png_set_bgr(png);

		png_bytep* rows = new png_bytep[height];
		for (int y = 0; y < height; y++) {
			rows[y] = (png_bytep)(pixels + y * span);
		}
		png_write_rows(png, rows, height);
		delete[]rows;

		png_write_end(png, info);
		png_destroy_write_struct(&png, &info);

		return true;
	}

	bool savePNG(const char* path, const char* license = NULL) {
		FILE* fp = fopen(path, "wb");
		if (!fp) return false;
		png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
		png_init_io(png, fp);
		bool result = writePNG(png, (char*)license);
		int closed = fclose(fp);
		if (closed == EOF) {
			result = false;
		}
		return result;
	}

};


int main() {
	std::cout << "skidtool 0.1" << std::endl;
	//	decodeCar("C:\\nitrologic\\skid30\\vehicles\\mini.aga", "C:\\nitrologic\\skid30\\vehicles\\mini.png");
	//	decodeCar("C:\\nitrologic\\skid30\\vehicles\\vw.aga", "C:\\nitrologic\\skid30\\vehicles\\vw.png");
	//	decodeCar("C:\\nitrologic\\skid30\\vehicles\\truck.aga", "C:\\nitrologic\\skid30\\vehicles\\truck.png");
	//	decodeCar("C:\\nitrologic\\skid30\\vehicles\\porsche.aga", "C:\\nitrologic\\skid30\\vehicles\\porsche.png");

	//	decodeCar("C:\\nitrologic\\skid30\\vehicles\\camaro.aga", "C:\\nitrologic\\skid30\\vehicles\\camaro.png");
	//	decodeCar("C:\\nitrologic\\skid30\\vehicles\\cow.aga", "C:\\nitrologic\\skid30\\vehicles\\cow.png");

	//	decodeCar("C:\\nitrologic\\skid30\\vehicles\\midget.aga", "C:\\nitrologic\\skid30\\vehicles\\midget.png");
	//	decodeCar("C:\\nitrologic\\skid30\\vehicles\\police.aga", "C:\\nitrologic\\skid30\\vehicles\\police.png");

	return 0;
}

#endif


