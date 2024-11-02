#include "skidkick.h"

//#include <sys/stat.h>

// acid500 monitor
// 
// (C) Copyright 2024 Simon Armstrong
//
// All rights reserved 
//
// https://github.com/nitrologic/skid30
//

#include <iostream>
#include <filesystem>

/*
Rem
bbdoc: Returns a case sensitive filename if it exists from a case insensitive file path.
End Rem
Function CasedFileName$(path$)
	Local	dir,sub$,s$,f$,folder$,p
	Local	mode,size,mtime,ctime       
	If stat_( path,mode,size,mtime,ctime )=0
		mode:&S_IFMT_
		If mode=S_IFREG_ Or mode=S_IFDIR_ Return path
	EndIf
	folder$="."
	For p=Len(path)-2 To 0 Step -1
		If path[p]=47 Exit
	Next
	If p>0
		sub=path[0..p]
		sub$=CasedFileName(sub$)
		If Not sub$ Return
		path=path$[Len(sub)+1..]
		folder$=sub
	EndIf
	s=path.ToLower()
	dir=opendir_(folder)
	If dir
		While True
			f=readdir_(dir)
			If Not f Exit
			If s=f.ToLower()
				If sub f=sub+"/"+f
				closedir_(dir)
				Return f
			EndIf
		Wend
		closedir_(dir)
	EndIf
End Function
*/

#include <string>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;


bool pathExists(const fs::path& path ){//const std::string& path){
	try{
		return fs::exists(path);
	}catch(fs::filesystem_error e){
		return false;
	}
}

std::string CasedFileName(const std::string& path) {
	fs::path fs_path(path);

	// If the file or directory exists with the exact case, return it
	if (pathExists(fs_path)) {
		return fs_path.string();
	}

	// Separate folder and file name
	fs::path folder = fs_path.parent_path();
	std::string filename = fs_path.filename().string();

	// Convert filename to lowercase for case-insensitive comparison
	std::string filename_lower = filename;
	std::transform(filename_lower.begin(), filename_lower.end(), filename_lower.begin(), ::tolower);

	// If folder path is empty, search in current directory
	if (folder.empty()) {
		folder = ".";
	}

	// Iterate over files in the specified directory
	try{
		for (const auto& entry : fs::directory_iterator(folder)) {

			try{

				std::string entry_name = entry.path().filename().string();
				std::string entry_name_lower = entry_name;
				std::transform(entry_name_lower.begin(), entry_name_lower.end(), entry_name_lower.begin(), ::tolower);

				// Check if filenames match in a case-insensitive manner
				if (filename_lower == entry_name_lower) {
					return (folder / entry_name).string();
				}
			}catch(fs::filesystem_error error){
				std::cout << "fs::filesystem_error : " << error.what() << std::endl;
			}
		}
	}catch(fs::filesystem_error error){
//		std::cout << "*** iterate folder : " << folder << " fs::filesystem_error : " << error.what() << std::endl;
	}

	// Return empty string if no match found
	return "";
}




#define trace_log

//#include <experimental/filesystem>
//namespace filesystem = std::experimental::filesystem;

namespace filesystem = std::filesystem;

#define acid500_cpu M68K_CPU_TYPE_68020

const int PREV_LINES = 4;
const int ASM_LINES = 6;
const int LOG_LINES = 4;

#define RUN_CYCLES_PER_TICK 1024*64

#include <assert.h>
#include <sstream>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <fstream>
#include <ctime>
#include <sys/types.h>

#include "machine.h"
#include "steamstub.h"
#include "skidnative.h"

#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif

#define SIGB_ABORT	0
#define SIGB_CHILD	1
#define SIGB_BLIT	4	/* Note: same as SINGLE */
#define SIGB_SINGLE	4	/* Note: same as BLIT */
#define SIGB_INTUITION	5
#define	SIGB_NET	7
#define SIGB_DOS	8

#define SIGF_ABORT	(1L<<0)
#define SIGF_CHILD	(1L<<1)
#define SIGF_BLIT	(1L<<4)
#define SIGF_SINGLE	(1L<<4)
#define SIGF_INTUITION	(1L<<5)
#define	SIGF_NET	(1L<<7)
#define SIGF_DOS	(1L<<8)

std::string disassembleLine(int pc);

std::string sigbits(int sig){
	std::stringstream ss;
	if(sig&0x1<<0) ss<<"#ABORT";
	if(sig&0x1<<1) ss<<"#CHILD";
	if(sig&0x1<<2) ss<<"#SIG_2";
	if(sig&0x1<<3) ss<<"#SIG_3";
	if(sig&0x1<<4) ss<<"#SINGLE";
	if(sig&0x1<<5) ss<<"#INTUITION";
	if(sig&0x1<<6) ss<<"#NET";
	if(sig&0x1<<7) ss<<"#DOS";
	if (sig & 0x1 << 8) ss << "#SIG_8";
	if (sig & 0x1 << 9) ss << "#SIG_9";
	if (sig & 0x1 << 10) ss << "#SIG_10";
	if (sig & 0x1 << 11) ss << "#SIG_11";
	if(sig&0x1<<12) ss<<"#CTRL_C";
	if(sig&0x1<<13) ss<<"#CTRL_D";
	if(sig&0x1<<14) ss<<"#CTRL_E";
	if(sig&0x1<<15) ss<<"#CTRL_F";
	if (sig & 0xffffff00) {
		ss << "#SIGH_" << std::to_string(sig);
	}
	return ss.str();
}
std::string str_tolower(std::string s)
{
	std::transform(s.begin(), s.end(), s.begin(),
		[](unsigned char c) { return std::tolower(c); }
	);
	return s;
}

// strips \n

//void systemLog(const char* a, std::stringstream &ss) {
//	systemLog(a, ss.str());
//}

void flushLog() {
#ifdef LOG_COUT
	for (auto it : machineLog) {
		std::cout << addressString(it.first) << " " << it.second << std::endl;
	}
#endif
	machineLog.clear();
}

#include "loadiff.h"

// vscode F11 - focus

#include <cassert>

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


struct heap {
};

// address is 24 bit 6hexdigit

// for validation purposes only
#ifdef KICKSTART
rom16 kickstart(0xf80000, 0xff80000, "../../media/kick.rom", 524288 ); // 512K
#endif

ram16 chipmem(0x000000, 0xfe00000, 0x100000);	// 2MB

chipset16 chipset(0xdff000, 0xffff000, 0x100); // 256 16 bit registers dff000..dff1fe 
interface8 cia_a(0xbfe000, 0xffff000, 0x1000); // 256 16 bit registers dff000..dff1fe 
interface8 cia_b(0xbfd000, 0xffff000, 0x1000); // 256 16 bit registers dff000..dff1fe 

amiga16 mig(0x800000, 0xff00000, 0x100000);

// chinnamasta soc


#include <filesystem>

char fileSeparator = std::filesystem::path::preferred_separator;

extern "C" {
#include "musashi/m68k.h"
#include "musashi/m68kcpu.h"
#include "musashi/m68kops.h"
#include "musashi/sim.h"
#include "musashi/mmu.h"
#include "musashi/m68kops.h"
}

struct Stream {
	std::stringstream out;

	void writeLong(int b) {
		out << std::setfill('0') << std::setw(8) << std::right << std::hex << b << std::dec;
	}
	void writeString(std::string s) {
		out << s;
	}
	void writeSpace() {
		out << " ";
	}
	void writeChar(int c) {
		out << (char)c;
	}
	void writeAddress(int b) {
		out << std::setfill('0') << std::setw(6) << std::right << std::hex << (b & 0xffffff) << std::dec;
	}
	void writeWord(int b) {
		out << std::setfill('0') << std::setw(4) << std::right << std::hex << (b & 0xffff) << std::dec;
	}
	void writeByte(int b) {
		out << std::setfill('0') << std::setw(2) << std::right << std::hex << (b & 0xff) << std::dec;
	}
	std::string flush() {
		std::string s = out.str();
		out.str("");
		return s;
	}
	void clear() {
		out.str("");
	}
};

const char readwrite[] = { 'R','W' };
const char intshortbyte[] = { 'l','w','b','?' };

typedef std::map<std::string, std::string> Dictionary;
typedef std::map<int, std::string> AddressMap;

/*


CIAA Address Map
----------------
 Byte    Register               Data bits
Address    Name    7     6     5    4    3     2     1    0
------------------------------------------------------------------
BFE001    pra      /FIR1 /FIR0 /RDY /TK0 /WPRO /CHNG /LED OVL
BFE101    prb      Parallel port
BFE201    ddra     Direction for port A (BFE001);1 output (set to 0x03)
BFE301    ddrb     Direction for port B (BFE101);1 output (can be in/out)
BFE401    talo     CIAA timer A low byte (.715909 Mhz NTSC; .709379 Mhz PAL)
BFE501    tahi     CIAA timer A high byte
BFE601    tblo     CIAA timer B low byte (.715909 Mhz NTSC; .709379 Mhz PAL)
BFE701    tbhi     CIAA timer B high byte
BFE801    todlo    50/60 Hz event counter bits  7-0 (VSync or line tick)
BFE901    todmid   50/60 Hz event counter bits 15-8
BFEA01    todhi    50/60 Hz event counter bits 23-16
BFEB01             not used
BFEC01    sdr      CIAA serial data register (connected to keyboard)
BFED01    icr      CIAA interrupt control register
BFEE01    cra      CIAA control register A
BFEF01    crb      CIAA control register B

Note: CIAA can generate interrupt INT2.

CIAB Address Map
----------------
Byte Register Data bits
Address   Name     7    6     5     4     3     2     1    0
-------------------------------------------------------------------
BFD000    pra      /DTR /RTS  /CD   /CTS  /DSR  SEL   POUT BUSY
BFD100    prb      /MTR /SEL3 /SEL2 /SEL1 /SEL0 /SIDE DIR  /STEP
BFD200    ddra     Direction for Port A (BFD000);1 = output (set to 0xFF)
BFD300    ddrb     Direction for Port B (BFD100);1 - output (set to 0xFF)
BFD400    talo     CIAB timer A low byte (.715909 Mhz NTSC; .709379 Mhz PAL)
BFD500    tahi     CIAB timer A high byte
BFD600    tblo     CIAB timer B low byte (.715909 Mhz NTSC; .709379 Mhz PAL)
BFD700    tbhi     CIAB timer B high byte
BFD800    todlo    Horizontal sync event counter bits 7-0
BFD900    todmid   Horizontal sync event counter bits 15-8
BFDA00    todhi    Horizontal sync event counter bits 23-16
BFDB00             not used
BFDC00    dr       CIAB serial data register (unused)
BFDD00    icr      CIAB interrupt control register
BFDE00    cra      CIAB Control register A
BFDF00    crb      CIAB Control register B

Note: CIAB can generate INT6.


[mem] 00008490 R w dff01c  0000
[mem] 0017d9c2 R b dff01f  00
[mem] 00008490 W w dff042  0000
[mem] 00008490 W w dff096  0400
[mem] 00008490 W w dff096  000f
[mem] 00008490 W w dff096  8068
[mem] 0017d9c2 W w dff098  1041
[mem] 00008490 W w dff09c  3fff
[mem] 00008490 W w dff09a  3fff
[mem] 00008490 W w dff09a  c008
[mem] 0017d9c2 W w dff09a  0020
[mem] 0017d9c2 W w dff09c  0020
[mem] 00008490 W l dff0d0  00023ec0
[mem] 00008490 W w dff0d4  03a2
[mem] 00008490 W w dff0d6  027f
[mem] 00008490 W w dff0d8  0000


*/

AddressMap addressMap = {
	{0,"ZERO"},
	{0xdff180,"COLOR0"},

	{0xbfe401,"TALO-A"},
	{0xbfe501,"TAHI-A"},
	{0xbfed01,"ICR-A"},
	{0xbfee01,"CRA-A"},

	{0xbfd100,"PRB-B"},
	{0xbfdd00,"ICR-B" },

	{0xdff002,"DMACONR"},

	{0xdff01c,"INTENAR"},
	{0xdff01e,"INTREQ"},
	{0xdff01f,"INTREQ1"},

	{0xdff036,"JOYTEST"},

	{0xdff040,"BLTCON0"},
	{0xdff042,"BLTCON1"},

	{0xdff044,"BLTAFWM"},
	{0xdff046,"BLTALWM"},

	{0xdff080,"COP1LCH"},
	{0xdff082,"COP1LCL"},

	{0xdff088,"COPJMP1"},
	{0xdff08a,"COPJMP2"},

	{0xdff092,"DDFSTRT"},
	{0xdff094,"DDFSTOP"},

	{0xdff096,"DMACON"},
	{0xdff098,"CLXCON"},
	{0xdff09c,"INTREQR"},
	{0xdff09a,"POTINP"},

	{0xdff0a0,"AUD0HI"},
	{0xdff0a2,"AUD0LO"},
	{0xdff0a4,"AUD0LEN"},
	{0xdff0a6,"AUD0PER"},
	{0xdff0a8,"AUD0VOL"},

	{0xdff0b0,"AUD1HI"},
	{0xdff0b2,"AUD1LO"},
	{0xdff0b4,"AUD1LEN"},
	{0xdff0b6,"AUD1PER"},
	{0xdff0b8,"AUD1VOL"},

	{0xdff0c0,"AUD2HI"},
	{0xdff0c2,"AUD2LO"},
	{0xdff0c4,"AUD2LEN"},
	{0xdff0c6,"AUD2PER"},
	{0xdff0c8,"AUD2VOL"},

	{0xdff0d0,"AUD3HI"},
	{0xdff0d2,"AUD3LO"},
	{0xdff0d4,"AUD3LEN"},
	{0xdff0d6,"AUD3PER"},
	{0xdff0d8,"AUD3VOL"},

	{0xDFF144,"SPR0DATA"},//	Sprite 0 image data register A
	{0xDFF146,"SPR0DATB"},//	Sprite 0 image data register B
	{0xDFF14C,"SPR1DATA"},//	Sprite 1 image data register A
	{0xDFF14E,"SPR1DATB"},//	Sprite 1 image data register B
	{0xDFF154,"SPR2DATA"},//	Sprite 2 image data register A
	{0xDFF156,"SPR2DATB"},//	Sprite 2 image data register B
	{0xDFF15C,"SPR3DATA"},//	Sprite 3 image data register A
	{0xDFF15E,"SPR3DATB"},//	Sprite 3 image data register B
	{0xDFF164,"SPR4DATA"},//	Sprite 4 image data register A
	{0xDFF166,"SPR4DATB"},//	Sprite 4 image data register B
	{0xDFF16C,"SPR5DATA"},//	Sprite 5 image data register A
	{0xDFF16E,"SPR5DATB"},//	Sprite 5 image data register B
	{0xDFF174,"SPR6DATA"},//	Sprite 6 image data register A
	{0xDFF176,"SPR6DATB"},//	Sprite 6 image data register B
	{0xDFF17C,"SPR7DATA"},//	Sprite 7 image data register A
	{0xDFF17E,"SPR7DATB"},//	Sprite 7 image data register B
};

struct MemEvent : Stream {
	int time;
	int address; // bits 31:star  30-29:R,W,F 28-27 - long,short,byte
	int data;
	int pc;
	std::string label;

	MemEvent(const MemEvent&other) {
		time = other.time;
		address = other.address;
		data = other.data;
		pc = other.pc;
		label = other.label;
	}
	MemEvent(int t32, int a32, int d32, int pc32, std::string label0) :time(t32), address(a32), data(d32), pc(pc32), label(label0) {
	}

	std::string toString() {

		out.clear();

		int t32 = time;
		int a32 = address;
		int d32 = data;
		int pc32 = pc;
		
		int star = (a32 >> 31) & 1;
		int rw = (a32 >> 29) & 3;
		int opsize = (a32 >> 27) & 3;
		int a24 = a32 & 0xffffff;

		// tick: R/W l/s/b address data 

		writeLong(t32);
		writeSpace();
		writeChar(readwrite[rw]);
		writeSpace();
		writeChar(intshortbyte[opsize]);
		writeSpace();
		writeAddress(a24);
		writeSpace();

		switch (opsize) {
		case 0:
			writeLong(d32);
			break;
		case 1:
			writeWord(d32);
			break;
		case 2:
			writeByte(d32);
			break;
		}
		if (star) {
			writeSpace();
			writeChar('*');
			writeAddress(pc32);
		}

		writeSpace();
		writeString(label);
		writeEOL();
		return flush();
	}
};

typedef std::vector<MemEvent> MemEvents;

//int m68k_execute(int num_cycles)

const int DumpLimit=5000;

enum Signals {
	DOS_BIT = 0x0080
};

struct acid68000 : acidmicro {

	int step = 0;
	int cycle = 0;

	int memoryError = 0;

	// the active memory pointer is enabled during address decode 
	memory32* mem;
	MemEvents memlog;

	std::vector<std::pair<int, int>> history;
	int prevPC = 0;
	int prevTick = 0;

	int heapPointer = HEAP_START; // 0x40000;

	std::set<std::uint32_t> breakpoints;

	std::string homePath;

	void programCounter(int physicalAddress) {

		int ppc = readRegister(M68K_REG_PPC); //m68k_get_reg(NULL, M68K_REG_PPC);

		//		int pc = readRegister(16);
		//		int tick = readCounter();
		if (ppc != prevPC) {
			int clock = readClock();
			history.push_back({ ppc,clock });
			prevPC = ppc;
		}
	}

	int previousPC(int age) {
		int n = (int)history.size();
		if (n > age) {
			return history[n - age].first;
		}
		return 0;
	}

	// note to ami - fetchString reply is not null terminated
	// todo - log when a1 parameter is null

	std::string fetchString(int a1) {
		std::stringstream ss;
		while (a1) {
			int byte = read8(a1++);
			if (byte == 0) break;
			ss << (char)(byte & 255);
		}
		return ss.str();
	}

	void setHome(std::string path) {
		homePath = path + fileSeparator;
	}

	// check case sensitive file system support
	// check caller empty string result exception handling

	std::string fetchPath(int a1) {
		std::string s = fetchString(a1);
		//		s = str_tolower(s);
		if (fileSeparator == '\\') {
			std::replace(s.begin(), s.end(), '/', '\\');
		}
		else {
			std::replace(s.begin(), s.end(), '\\', '/');
		}
		int t = s.find_first_of(':');


		int p = s.find_first_of(':');
		if (p < 0) {
			s = homePath + s;
		}else{
			std::string device=s.substr(0,p);
			s=s.substr(p+1);
			if (device == "t" || device == "T") {
				s = "temp/" + s;
			}
			else {
				// return error
			}
		}
		std::string ss = CasedFileName(s);
		//		std::cout << "fetchPath " << s << " => " << ss << std::endl;
		if (ss.length()) {
			s = ss;
		}
		return s;
	}

	std::vector<u8> fetchBytes(int a1, int d0) {
		int n = d0;
		std::vector<u8> result(n);
		for (int i = 0; i < n; i++) {
			result[i] = read8(a1++);
		}
		return result;
	}

	std::vector<u16> fetchShorts(int a1, int d0) {
		int n = (d0 + 1) / 2;
		std::vector<u16> result(n);
		for (int i = 0; i < n; i++) {
			result[i] = read16(a1);
			a1 += 2;
		}
		return result;
	}


	// http://amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_2._guide/node0332.html
	// TODO - round size to page boundary

	int allocate(int size, int bits) {
		if (bits & MEMF_FAST) return 0;
//		size = (size + 3) & -4;
		size = (size + 31) & -32;
		int p = heapPointer;
		heapPointer += size;
		if (bits & 1) {
			for (int i = 0; i < size; i += 4) {
				qwrite32(p + i, 0);
			}
		}
		return p;
	}

	int sigbits = 0;

	int setSignalBits(int newbits, int setbits) {
		sigbits &= ~setbits;
		newbits &= setbits;
		sigbits |= newbits;
		return sigbits;
	}

	// input address is 24 bit physical with qbit signals in high bits

	void log_bus(int readwritefetch, int byteshortint, int address, int value) {
		bool enable = (readwritefetch == 1) ? (mem->flags & 2) : (mem->flags & 1);
		int star = (mem->flags & 4) ? 1 : 0;
		int err = (address == memoryError) ? 1 : 0;
		if (enable || err) {
			// low 24 bits are physical address
			int a32 = ((star | err) << 31) | (readwritefetch << 29) | (byteshortint << 27) | (address & 0xffffff);
			int pc = readRegister(16);

			std::string label;
			if (addressMap.count(address)) {
				label = addressMap[address];
			}

			memlog.emplace_back(cycle, a32, value, pc, label);

			std::string s = memlog.back().toString();
			systemLog("mem", s);
		}
	}

	//	void dumpEvent(std:stringstream & out, MemEvent& e) {
	//	}

	void writeTrace(std::string path) {
		std::ofstream fout(path);


		auto it = machineLog.begin();

		//		std::vector<std::pair<int, logline>> machineLog;

		int n = history.size();
		for (int i = 0; i < n; i++) {

			int pc = history[i].first;
			int clock = history[i].second;

			while (it != machineLog.end() && clock > it->first) {
				int clk = it->first;
				std::string line = it->second;
				fout << addressString(clk) << " " << line << std::endl;
				it++;
			}

			std::string dis = disassembleLine(pc);
			fout << addressString(clock) << " " << dis << std::endl;
		}
		fout.close();
	}

	void writeLog(std::string path) {
		std::ofstream fout(path);
		for (auto m : machineLog) {
			fout << addressString(m.first) << " " << m.second << std::endl;
		}
		fout.close();
	}

	void dumplog(int max) {
		int n = memlog.size();
		if (max == 0) max = (n < DumpLimit) ? n : DumpLimit;
		int begin = n - max;//5;
		if (begin < 0) begin = 0;
		for (int i = begin; i < n; i++) {
			MemEvent& e = memlog[i];
			int t32 = e.time;
			int a32 = e.address;
			int d32 = e.data;
			int pc32 = e.pc;
			int star = (a32 >> 31) & 1;
			int rw = (a32 >> 30) & 1;
			int opsize = (a32 >> 28) & 3;
			int a24 = a32 & 0xffffff;

			// tick: R/W l/s/b address data 

			writeData32(t32);
			writeSpace();
			writeChar(readwrite[rw]);
			writeSpace();
			writeChar(intshortbyte[opsize]);
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
			if (star) {
				writeSpace();
				writeAddress(pc32);
			}
			writeEOL();
		}

	}

	void resume() {
		memoryError = 0;
		m68k_clear_halt();
	}

	void call(int subroutine) {

	}

	void writeRegister(int reg, int value) {
		m68k_set_reg((m68k_register_t)reg, (unsigned int)value);
	}

	int readRegister(int reg) {
		void* context = 0;
		unsigned int value = m68k_get_reg(context, (m68k_register_t)reg);
		return (int)value;
	}

	// sets ::mem

	int decode(int physicalAddress) {
#ifdef KICKSTART		
		if ((physicalAddress & kickstart.mask) == kickstart.physical) {
			mem = &kickstart;
			return physicalAddress & (~kickstart.mask);
		}
#endif		
		if ((physicalAddress & mig.mask) == mig.physical) {
			mem = &mig;
			return physicalAddress & (~mig.mask);
		}
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
		m68k_pulse_halt();

		//		m68k_pulse_bus_error();
		//		writeAddress(physicalAddress);

		return -1;
	}

	int read8(int a32) {
		int qbits = a32 & 0xc0000000;
		if (qbits) {
			if ((a32 & 0xff000000) == 0xff000000) {
				// link address here
//				physicalAddress = 0;
				a32 = 0;
			}
		}
		int physicalAddress = a32 & 0xffffff;
		int address = decode(physicalAddress);
		if (address < 0) {
			log_bus(0, 2, physicalAddress, 0);
			return 0; // free pass hackers are us
		}
		int value = mem->read8(address);
		log_bus(0, 2, physicalAddress, value);
		return value;
	}

	// only cpu pc fetch has bit 31 of a32 set
	// only debugger fetch has bit 30 of a32 set 

	int read16(int a32) {
		int qbits = a32 & 0xc0000000;	// instruction or debugger fetch
		int physicalAddress = a32 & 0xffffff;

		if (qbits & QBIT) { // 0x80000000 pc is on the bus, check breakpoints
			if ((a32 & 0xff000000)== 0xff000000){
				// link address here
				physicalAddress = 0;
			}
			if (breakpoints.count(a32)) {
				memoryError = physicalAddress;
				m68k_pulse_halt();
				return 0;
			}
			programCounter(physicalAddress);
		}
		int address = decode(physicalAddress);
		if (address < 0) {
			log_bus(qbits ? 2 : 0, 1, physicalAddress, 0);
			return 0; // free pass hackers are us
		}
		int value = mem->read16(address, qbits);

		if (machineError) {
			// TODO - status = machineStatus;
			memoryError = machineError;
			m68k_pulse_halt();
			// TODO - emit message
			systemLog("acid", machineState + std::to_string(qbits) + std::to_string(machineError));
			machineError = 0;
			//			flushLog();
			//			log_bus(qbits?2:0, 1, physicalAddress, 0);
		}

		if (qbits == 0) {
			log_bus(0, 1, physicalAddress, value);
		}

		return value;
	}

	int read32(int a32) {
		int qbits = a32 & 0xc0000000;
		if (qbits) {
			if ((a32 & 0xff000000) == 0xff000000) {
				// link address here
//				physicalAddress = 0;
				a32 = 0;
			}
		}
		int physicalAddress = a32 & 0xffffff;
		int address = decode(physicalAddress);
		if (address < 0) {
			log_bus(0, 0, physicalAddress, 0);
			return 0; // free pass hackers are us
		}
		int value = mem->read32(address);
		if (qbits == 0) log_bus(0, 0, physicalAddress, value);
		return value;
	}

	void write8(int physicalAddress, int value) {
		int address = decode(physicalAddress);
		log_bus(1, 2, physicalAddress, value);
		if (!memoryError) {
			mem->write8(address, value);
		}
	}

	void writeChunk(int physicalAddress, const Chunk& chunk) {
		int n = chunk.size();
		for (int i = 0; i < n; i++) {
			qwrite16(physicalAddress + i * 2, chunk[i]);
		}
	}

	void push(int physicalAddress) {
		int sp = readRegister(15) - 4;
		write32(sp, physicalAddress);
		writeRegister(15, sp);
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

	int writes(int physicalAddress, std::string s, int maxlen) {
		int address = decode(physicalAddress);
		int count = 0;
		for (auto c : s) {
			mem->write8(address++, c);
			count++;
			if (count >= maxlen) break;
		}
		mem->write8(address++, 0);
		return count;
	}

	void qwrite32(int physicalAddress, int value) {
		int address = decode(physicalAddress);
		mem->write32(address, value);
	}

	void qwrite16(int physicalAddress, int value) {
		int address = decode(physicalAddress);
		mem->write16(address, value);
	}

	void breakpoint(int address) {
		breakpoints.insert(address | QBIT);// 0x80000000);
	}
};

acid68000 acid500;

int clockReads = 0;
extern int readClock(){
	return acid500.cycle+(clockReads++);
}

class acidbench : public IBench {
	acid68000* cpu0;
public:
	acidbench(acid68000* cpu) {
		cpu0 = cpu;
	}
	void closeWorkBench() {
		cpu0->writeRegister(0, 1); // return true for success
	}
};

class acidnonvolatile : public INonVolatile {
	std::stringstream nvlog;
	acid68000* cpu0;

	void emit() {
		std::string s = nvlog.str();
		systemLog("nv", s);
		nvlog.str(std::string());
	}


public:
	acidnonvolatile(acid68000* cpu) {
		cpu0 = cpu;
	}
	//appName, itemName, killRequesters a0,a1,d1
	void getCopy() {
		int a0 = cpu0->readRegister(8);
		int a1 = cpu0->readRegister(9);
		int d1 = cpu0->readRegister(1);
		std::string app= cpu0->fetchString(a0);
		std::string item= cpu0->fetchString(a1);
		int data = 0;
		cpu0->writeRegister(0, data);
		nvlog << "getCopy " << app << "," << item << "," << d1;
		emit();
	}

};


class acidgraphics : public IGraphics {
	std::stringstream gfxlog;
	acid68000* cpu0;

	void emit() {
		std::string s = gfxlog.str();
		systemLog("gfx", s);
		gfxlog.str(std::string());
	}

public:
	acidgraphics(acid68000* cpu) {
		cpu0 = cpu;
	}
	void textLength() {

	}
	void loadView() {
		int a1 = cpu0->readRegister(9);
		gfxlog << "loadview " << a1; emit();
	}
	void waitTOF() {
		gfxlog << "waitTOF "; emit();
	}
	void ownBlitter() {
		gfxlog << "ownBlitter"; emit();
	}
	void disownBlitter() {
		gfxlog << "disownBlitter"; emit();
	}
};

class acidfastmath : public IFFPMath {
	std::stringstream mathlog;
	void emit() {
		std::string s = mathlog.str();
		systemLog("math", s);
		mathlog.str(std::string());
	}

public:

	acid68000* cpu0;
	acidfastmath(acid68000* cpu) {
		cpu0 = cpu;
	}


};

// musashi entry points to acid cpu address bus

unsigned int mmu_read_byte(unsigned int address){
	int v8=acid500.read8(address);
//	if (v8&128) 
//		v8 |= -256;
	v8 &= 0xff;
	return v8;
}
unsigned int mmu_read_word(unsigned int address){
	int v16=acid500.read16(address);
	v16 &= 0xffff;
//	if (v16 & 0x8000) v16 |= -65536;
	return v16;
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
	return acid500.read16(address | 0x40000000);
}

unsigned int cpu_read_long_dasm(unsigned int address)
{
	return acid500.read32(address | 0x40000000);
}

// write to acid500

void loadHunk(std::string path,int physical) {
	writeString("Reading Hunk from ");
	writeString(path);
	writeEOL();

	filesystem::path p(path);
	std::string filename=p.filename().string();

	filedecoder fd(path);

	if (fd.f==0) {
		writeString("error");
		writeEOL();
		exit(1);
		return;
	}

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
//	writeNamedInt("total words", totalWords);
//	writeEOL();
	Chunk chunk(totalWords);
//	writeNamedInt("hunk count", n);
//	writeEOL();
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
//			std::cout << "HUNK_CODE" << std::endl;
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
//			std::cout << "HUNK_DATA" << std::endl;
			int target = offsetWords[index];
			u32 count = fd.readBigInt();
			for (int i = 0; i < count; i++) {
				int l32 = fd.readBigInt();
				chunk[target+i*2+0] = (l32 >> 16);
				chunk[target+i*2+1] = (l32 & 0xffff);
			}
			break;
		}
		case 1003: // HUNK__BSS
		{
			std::cout << "HUNK_BSS" << std::endl;
			int target = offsetWords[index];
			u32 count = fd.readBigInt();
			for (int i = 0; i < count; i++) {
				chunk[target+i*2+0] = 0;
				chunk[target + i * 2 + 1] = 0;
			}
			break;
		}
		case 1004:	//RELOC32
		{
			std::cout << "HUNK_RELOC32" << std::endl;
			while (true) {
				u32 count = fd.readBigInt();
				if (count == 0)
					break;
				int current = offsetWords[index];
				u32 index32 = fd.readBigInt();
				int target = offsetWords[index32];
				u32 reloc32 = physical + target * 2;
				for (int i = 0; i < count; i++) {
					u32 offset = fd.readBigInt();
					u32 word = current + offset / 2;
					u32 loc32=(chunk[word] << 16) | (chunk[word + 1] & 0xffff);
					loc32 = loc32 + reloc32;
					chunk[word] = (loc32 >> 16);
					chunk[word + 1] = loc32 & 0xffff;
				}
			}
			break;
		}
		case 1010:	//HUNK__END
		{
//			std::cout << "HUNK_END" << std::endl;
			index++;
			if (index == n) {
				parseHunk = false;
			}
			break;
		}
		default:
		{
			//std::cout << "type " << std::hex << type << " not supported " << std::endl;
			std::cout << "type " << type << " not supported " << std::endl;
			//			assert(false);
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
	std::stringstream ss;
	ss<<filename<<" start:"<<addressString(physical)<<" end:" << addressString(physical+totalWords*2);
	systemLog("hunk", ss.str());
//	writeString("hunks parsed");
//	writeEOL();
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

std::string disassembleLine(int pc) {
	char buff[100];
	char buff2[100];
	char buff3[100];
//	std::stringstream ss;
	int instr_size = m68k_disassemble(buff, pc, M68K_CPU_TYPE_68000);
	make_hex(buff2, pc, instr_size);
	int n=snprintf(buff3,100,"%06x: %-20s: %s", pc, buff2, buff);
	return std::string(buff3, n);
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
}

const char* title = "       ☰☰☰☰☰☰☰ ☰☰☰☰☰☰☰ ☰☰☰☰☰☰☰ 🟠 ACID 500";

const char* help = "[s]tep [o]ver [c]ont [pause] [r]eset [h]ome [q]uit";

const int SP_START = 0x1e00;
const int ROM_START = 0x2000;

//const int SP_START = 0x5400;
//const int ROM_START = 0x6000;

const int ARGS_START = 0x200;

void displayLogLines(int count) {
	int n = machineLog.size();
	for (int i = n - count; i < n; i++) {
		if (i >= 0) {
			std::cout << machineLog[i].second;
		}
		writeEOL();
	}
}

void initMig(){
	acidexec *bass=new acidexec(&acid500);
	aciddos* sub = new aciddos(&acid500);
	acidbench* bench = new acidbench(&acid500);
	acidgraphics* gfx = new acidgraphics(&acid500);
	acidnonvolatile* nvram = new acidnonvolatile(&acid500);
	acidfastmath* math = new acidfastmath(&acid500);
	
	mig.setExec((struct IExec*)bass);
	mig.setDos((struct IDos*)sub);
	mig.setBench((struct IBench*)bench);
	mig.setNonVolatile((struct INonVolatile*)nvram);
	mig.setGraphics((struct IGraphics*)gfx);
	mig.setMath((struct IFFPMath*)math);
}

int runRom(int pc24,const char *name,const char *args,const char *home) {
	initMig();
	acid500.setHome(home);
	systemLog("args", args);
	systemLog("home", home);
	acid500.qwrite32(0, SP_START); //sp
	acid500.qwrite32(4, 0x801000); //exec
	acid500.qwrite32(8, pc24); //pc
	int pc = pc24;//acid500.readRegister(16);
//	writeClear();
	m68k_init();
	m68k_set_cpu_type(acid500_cpu);
	m68k_pulse_reset();
	int arglen = 0;
	if (args) {
		arglen=acid500.writes(ARGS_START, args, 0x100);
	}
	// command line mode starts D0,A0 with arglen,args
	acid500.writeRegister(0, arglen);
	acid500.writeRegister(8, ARGS_START);
	int err = 0;
	int run=1;
	while (run && err==0) {
		int n=RUN_CYCLES_PER_TICK;
		int cycles=m68k_execute(n);
		acid500.step++;
		acid500.cycle+=cycles;
		if (acid500.memoryError) {
			run = false;
			err = acid500.memoryError;
		}
		flushLog();
	}
	std::cout << "cycles: " << acid500.cycle << std::endl;
	acid500.writeLog("skidtool.log");
	return err;
}

void debugRom(int pc24,const char *name,const char *args,const char *home) {

	initMig();

	int key = 0;
	int run = 0;
	int err = 0;
	bool refresh=true;
	
	const char* status = name;

	acid500.setHome(home);

	systemLog("args", args);
	systemLog("home", home);

	acid500.qwrite32(0, SP_START); //sp
	acid500.qwrite32(4, 0x801000); //exec
	acid500.qwrite32(8, pc24); //pc

// top of stack hack
//	acid500.qwrite32(0x1400, 0x807000); //pc
#ifdef HAS_NOPS
	while (*nops) {
		int a = *nops++;
		acid500.qwrite16(a, 0x4e75); //pc
	}
#endif
//	acid500.writeRegister(16, pc24);

	int pc = pc24;//acid500.readRegister(16);

	writeClear();

	m68k_init();
	m68k_set_cpu_type(acid500_cpu);
	m68k_pulse_reset();

	int arglen = 0;
	if (args) {
		arglen=acid500.writes(ARGS_START, args, 0x100);
	}

	// command line mode starts D0,A0 with arglen,args
	acid500.writeRegister(0, arglen);
	acid500.writeRegister(8, ARGS_START);

	// refresh has 20 milli second sanity delay
	int drawtime=millis();
	while (true) {
		int t=millis();
		int elapsed=t-drawtime;

//		std::cout << "elapsed:" << elapsed << std::endl;

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
			writeNamedInt("step", acid500.step);
			writeEOL();
			writeNamedInt("cycle", acid500.cycle);
			writeEOL();
			writeNamedString("status", status);
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
#ifdef HISTORY
			for (int i = 0; i < PREV_LINES; i++) {
				int index = (PREV_LINES - i);
				int pc2 = acid500.previousPC(index);
				if (pc2) {
					disassemble(pc2, 1);
				}
				else {
					writeEOL();
				}

			}
			writeEOL();
#endif
//			disassemble(pc, ASM_LINES);
//			writeEOL();

			displayLogLines(LOG_LINES);

//			acid500.dumplog(5);

			writeString(help);
//			writeEOL();
//			writeEOL();

			drawtime=millis();
			refresh=false;
		}

		key=getChar();

		if (key == 'q') break;

		if(key=='h'){
			writeClear();
			refresh=true;
		}

		if (key == 's') {
			status = "step";
			int cycles=m68k_execute(1);
			acid500.step++;
			acid500.cycle+=cycles;
			refresh=true;
		}

		if (key == 'o') {
			acid500.breakpoint(pc+4);
			status = "running";
			run = 1;
		}

//		if (key == 'r') {
//			run = 1 - run;
//		}

		if(key=='c'){
			status = "running";
			run=1;
		}

		if(key=='p'){
			status = "paused";
			run=0;
		}

		if (run) {
			int n=RUN_CYCLES_PER_TICK;
			int cycles=m68k_execute(n);
			acid500.step++;
			acid500.cycle+=cycles;
			refresh=true;
			if (acid500.memoryError) {
				run = false;
				err = acid500.memoryError;
				status = "memory error";
				// TODO emit message - who will unhalt the processor
				acid500.resume();
			}
		}
		pc = acid500.readRegister(16);

//		usleep(1000);
	}

#ifdef trace_log
	std::cout << std::endl << std::endl << "Write skidtool.log to disk? (y/N) " << std::flush;
	int charcode=waitChar();
//	std::cout << "charcode " << charcode << std::flush;
//	acid500.writeLog("skidtool.log");
	if(charcode=='y'){
		acid500.writeTrace("trace.log");
	}
#endif

	std::cout << "done" << std::endl;
	uninitConsole();
//	acid500.dumplog(0);
}

int convertFiles() {
//	const char* iff = "../../archive/amigademogfx/skid.iff";
//	const char* iff = "../../archive/amigademogfx/impact.iff";
	const char* iff = "../../archive/amigademogfx/engine.iff";
//	loadILBM(iff);
	loadSVX(iff);
	return 0;
}

// main entry point for acid500 skidkick

int main() {
	writeClear();
//	std::cout << "  ☰☰ ACID 500 🟠" << std::endl;
	std::cout << "skidtool nitrokick 0.7" << std::endl;
/*
	COORD rect;
	HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleDisplayMode(out, CONSOLE_FULLSCREEN_MODE, &rect);
*/
#ifdef HAS_STEAM
	int loggedOn = OpenSteam(768030);
	std::cout << "Steam " << ((loggedOn) ? "Logged In" : "Offline") << std::endl;

	if (loggedOn>0) {

#ifdef STATS
		while (true) {
			const char* s = ReadSteam();

			if (s && *s) {
				std::cout << "[steam]" << s << std::endl;

				std::string line(s);
				if (line == "stats received!") {
					int games_played = GetSteamStat("games_played");
					std::cout << "games_played=" << games_played << std::endl;
				}
			}

			sleep(100);
		}
#endif
	}
#endif

	int rows, cols;
	screenSize(rows, cols);
//	mouseOn();
	std::cout << "screenSize rows:" << rows << " cols:" << cols << std::endl;
	initConsole();

#ifdef test_genam
	const char* amiga_binary = "../archive/genam";
	const char* amiga_args = "blitz2skid.s\n";
	const char* amiga_home = "blitz2src";
#endif

//#define test_guard

#ifdef test_guard
	//  const char* amiga_binary = "../../archive/guardian";
	//	const char* amiga_binary = "../archive/virus";
	//	const char* amiga_binary = "../archive/oblivion/oblivion";
	//	const char* amiga_binary = "../archive/skidpower/skidmarks";

	const char* amiga_binary = "skidpow30/Skid";
	const char* amiga_home = "skidpow30";
	const char* amiga_args = "";
#endif

//#define test_avail
#ifdef test_avail
	const char* amiga_binary = "C/Avail";	//waiting readargs support
	const char* amiga_args = "\n\0";
	const char* amiga_home = ".";
#endif

// amiga_binary
// 
//	const char* amiga_binary = "../archive/blitz2/blitz2";
//	const char* args = "-c test.bb\n";

//	const char* amiga_binary = "../archive/genam";
//	const char* args = "test.s -S -P\n";

#define test_lha
#ifdef test_lha
	const char* amiga_home = ".";
	const char* amiga_binary = "../archive/lha";
//	const char* amiga_args= "e cv.lha\n";
	const char* amiga_args = "e skid.lha\n";
#endif
//	const char* amiga_binary = "C/Avail";
//	const char* amiga_args= "";

//	const int nops[] = {0x63d6, 0};

//	const char* amiga_binary = "blitz2src/blitz2skid";
//	const char* amiga_args = "\n";
//	const char* amiga_home = ".";
	//	const char* amiga_args = "blitz2.s -S -P\n";

//	const char* amiga_binary = "C/Avail";	//waiting readargs support
//	const char* amiga_binary = "C/Dir";		//waiting readargs support
//	const char* amiga_binary = "C/Date";	//needs Utility library
//	const char* amiga_args = "\n\0";
//	const char* amiga_home = ".";
//
//	const char* amiga_binary = "skidaf/skid";
//	const char* amiga_home = "skidaf";
//	const char* amiga_args = "\n";

//	const char* amiga_binary = "oblivion/oblivion";
//	const char* amiga_home = "oblivion";

	const int nops[] = { 0 };

	loadHunk(amiga_binary,ROM_START);

//#define pause
#ifdef pause
	writeString("enter to continue");
	writeEOL();
	waitChar();
#endif

//	const char* name = "lha @ ROM_START";

	std::string name = std::string("hunk:")+amiga_binary+" args:"+amiga_args;

//	debugRom(ROM_START, name.c_str(), amiga_args, amiga_home);

	runRom(ROM_START, name.c_str(), amiga_args, amiga_home);

//  kickstart sanity test
//	debugCode(0xf800d2);

	return 0;
};
