// acid500 monitor
// 
// All rights reserved 2023 Simon Armstrong
//
// https://github.com/nitrologic/skid30
//

// #define trace_log

const int PREV_LINES = 4;
const int ASM_LINES = 6;
const int LOG_LINES = 4;

#define RUN_CYCLES_PER_TICK 1024

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
	for (auto it : machineLog) {
		std::cout << addressString(it.first) << " " << it.second << std::endl;
	}
	machineLog.clear();
}

std::string rawString(std::vector<u8> raw,bool addhex) {
	std::stringstream ss;
	std::stringstream ss2;
	int n = (int)raw.size();
	ss << std::setfill('0')  << std::right << std::hex;
	for (int i = 0; i < n; i++) {
		ss << std::setw(2) << (int)raw[i] << " ";
		ss2 << (char)raw[i];
	}
	if (addhex) {
		ss2 << std::endl << ss.str();// std::endl + ss2.str();
	}
	return ss2.str();
		
}

#include "loadiff.h"

// vscode F11 - focus

#include <cassert>

#ifdef WIN32
#include <windows.h>
#include <conio.h>
#include <synchapi.h>

SYSTEMTIME epoch = { 1978,1,0,1 };

void dayminticks(int *dmt) {
	SYSTEMTIME time;
	GetLocalTime(&time);
	double v0,v1;
	SystemTimeToVariantTime(&epoch,&v0);
	SystemTimeToVariantTime(&time,&v1);
	dmt[0] = (v1-v0);
	dmt[1] = time.wHour * 60 + time.wMinute;
	dmt[2] = time.wSecond * 50 + (time.wMilliseconds/20);
}

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
void screenSize(int& columns, int& rows) {
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
	columns = csbi.srWindow.Right - csbi.srWindow.Left + 1;
	rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
}

int mouseOn() {
	HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
	int fdwMode = ENABLE_EXTENDED_FLAGS;
	if (!SetConsoleMode(hStdin, fdwMode)) return -1;
	fdwMode = ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT;
	if (!SetConsoleMode(hStdin, fdwMode)) return -1;
	return 0;
}

#else
#include <sys/ioctl.h>

#include <termios.h>
#include <sys/time.h>

void screenSize(int &row,int &col){
	winsize w;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
	row=w.ws_row;
	col=w.ws_col;
}

void Sleep(int ms){
	usleep(ms*1e3);
}
int getch2(){
	char c;
	scanf("%c",&c);	
	return (int)c;
}

#ifdef LINUX2

#include "tty_getch.h"
#include <time.h>

int millis(){
	uint64_t t=clock_gettime_nsec_np(CLOCK_UPTIME_RAW);
	return (int)(t/1e6);
}
#else

uint64_t millis(){
	timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
	return (spec.tv_sec*1000)+(spec.tv_nsec/1e6);
}


#ifdef NO_TM
struct tm {
	int	tm_sec;		/* seconds after the minute [0-60] */
	int	tm_min;		/* minutes after the hour [0-59] */
	int	tm_hour;	/* hours since midnight [0-23] */
	int	tm_mday;	/* day of the month [1-31] */
	int	tm_mon;		/* months since January [0-11] */
	int	tm_year;	/* years since 1900 */
	int	tm_wday;	/* days since Sunday [0-6] */
	int	tm_yday;	/* days since January 1 [0-365] */
	int	tm_isdst;	/* Daylight Savings Time flag */
	long	tm_gmtoff;	/* offset from UTC in seconds */
	char	*tm_zone;	/* timezone abbreviation */
};
#endif

struct std::tm jan1978 = {0,0,0,1,1,78};

void dayminticks(int *dmt) {
	timeval t;
	gettimeofday(&t,NULL);
	std::time_t epoch = std::mktime(&jan1978);   // get time now
	std::time_t now = std::time(0);   // get time now
	std::tm* time = std::localtime(&now);
	int days=std::difftime(now, epoch) / (60 * 60 * 24);;
	int mins=time->tm_hour*60+time->tm_min;
	int ticks=t.tv_sec*50+t.tv_usec/2e4;
    dmt[0]=days;
	dmt[1]=mins;
	dmt[2]=ticks;
}

#endif

#endif

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

ram16 chipmem(0x000000, 0xff00000, 0x100000);	// 2MB
chipset16 chipset(0xdff000, 0xffff000, 0x100); // 256 16 bit registers dff000..dff1fe 
interface8 cia_a(0xbfe000, 0xffff000, 0x1000); // 256 16 bit registers dff000..dff1fe 
interface8 cia_b(0xbfd000, 0xffff000, 0x1000); // 256 16 bit registers dff000..dff1fe 

amiga16 mig(0x800000, 0xff00000, 0x100000);

// chinnamasta soc

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

struct acid68000 {

	int step=0;
	int cycle=0;

	int memoryError = 0;

	// the active memory pointer is enabled during address decode 
	memory32* mem;
	MemEvents memlog;

	std::vector<std::pair<int,int>> history;
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
			return history[n-age].first;
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
			ss << (char)(byte&255);
		}
		return ss.str();
	}

	void setHome(std::string path) {
		homePath = path + "\\";
	}

	std::string fetchPath(int a1) {
		std::string s=fetchString(a1);
//		s = str_tolower(s);
		std::replace(s.begin(),s.end(),'/','\\');
		int p = s.find_first_of(':');
		if (p<0) {
			s = homePath + s;
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

	int sigbits=0;

	int setSignal(int newbits,int setbits){
		sigbits &= ~setbits;
		newbits &= setbits;
		sigbits |= newbits;
		return sigbits;
	}

	// input address is 24 bit physical with qbit signals in high bits

	void log_bus(int readwritefetch, int byteshortint, int address, int value) {
		bool enable=(readwritefetch==1)?(mem->flags&2):(mem->flags&1);
		int star=(mem->flags&4)?1:0;
		int err = (address == memoryError)?1:0;
		if(enable||err){
			// low 24 bits are physical address
			int a32 = ((star|err) << 31) | (readwritefetch << 29) | (byteshortint << 27) | (address & 0xffffff);
			int pc=readRegister(16);

			std::string label;
			if (addressMap.count(address)) {
				label = addressMap[address];
			}

			memlog.emplace_back(cycle, a32, value, pc, label);

			std::string s=memlog.back().toString();
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
				std::string line=it->second;
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
			if(star) {
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

	void writeEndianMem(int physicalAddress, void *src, size_t size) {
		int n = (size + 3) / 4;
		uint32_t* l = (uint32_t *)src;
		for (int i = 0; i < n; i++) {
			write32(physicalAddress+i*4, l[i]);
		}
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

	// only cpu pc fetch has bit 31 of a32 set
	// only debugger fetch has bit 30 of a32 set 

	int read16(int a32) {
		int qbits = a32 & 0xc0000000;	// instruction or debugger fetch
		int physicalAddress = a32 & 0xffffff;

		if (qbits & QBIT) { // 0x80000000 pc is on the bus, check breakpoints
			if (breakpoints.count(a32)) {
				memoryError = physicalAddress;
				m68k_pulse_halt();
				return 0;
			}
			programCounter(physicalAddress);
		}
		int address = decode(physicalAddress);
		if (address < 0) {
			log_bus(qbits?2:0, 1, physicalAddress, 0);
			return 0; // free pass hackers are us
		}
		int value = mem->read16(address,qbits);

		if (machineError) {
			// TODO - status = machineStatus;
			memoryError = machineError;
			m68k_pulse_halt();
			// TODO - emit message
//			systemLog("acid",machineState + std::to_string(qbits) + std::to_string(machineError));
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
		int physicalAddress = a32 & 0xffffff;
		int address = decode(physicalAddress);
		if (address < 0) {
			log_bus(0, 0, physicalAddress, 0);
			return 0; // free pass hackers are us
		}
		int value = mem->read32(address);
		if(qbits==0) log_bus(0, 0, physicalAddress, value);
		return value;
	}

	void write8(int physicalAddress, int value) {
		int address = decode(physicalAddress);
		log_bus(1, 2, physicalAddress, value);
		if (!memoryError) {
			mem->write8(address, value);
		}
	}

	void push(int physicalAddress) {
		int sp=readRegister(15)-4;
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

	int writes(int physicalAddress, std::string s,int maxlen) {
		int address = decode(physicalAddress);
		int count = 0;
		for(auto c:s){
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


const int INPUT_STREAM = -4;
const int OUTPUT_STREAM = -8;
const int FILE_STREAM = -24;


struct DateStamp {
	int days,mins,ticks;
};

/* Returned by Examine() and ExNext(), must be on a 4 byte boundary */

struct FileInfoBlock {
	int	  fib_DiskKey;
	int	  fib_DirEntryType;  /* Type of Directory. If < 0, then a plain file. If > 0 a directory */
	char	  fib_FileName[108]; /* Null terminated. Max 30 chars used for now */
	int	  fib_Protection;    /* bit mask of protection, rwxd are 3-0.	   */
	int	  fib_EntryType;
	int	  fib_Size;	     /* Number of bytes in file */
	int	  fib_NumBlocks;     /* Number of blocks in file */
	struct DateStamp fib_Date;/* Date file last changed */
	char	  fib_Comment[80];  /* Null terminated comment associated with file */

	/* Note: the following fields are not supported by all filesystems.	*/
	/* They should be initialized to 0 sending an ACTION_EXAMINE packet.	*/
	/* When Examine() is called, these are set to 0 for you.		*/
	/* AllocDosObject() also initializes them to 0.			*/
	uint16_t fib_OwnerUID;		/* owner's UID */
	uint16_t fib_OwnerGID;		/* owner's GID */

	char	  fib_Reserved[32];
}; /* FileInfoBlock */

/* FIBB are bit definitions, FIBF are field definitions */
#define FIBB_SCRIPT    6	/* program is a script (execute) file */
#define FIBB_PURE      5	/* program is reentrant and rexecutable */
#define FIBB_ARCHIVE   4	/* cleared whenever file is changed */
#define FIBB_READ      3	/* ignored by old filesystem */
#define FIBB_WRITE     2	/* ignored by old filesystem */
#define FIBB_EXECUTE   1	/* ignored by system, used by Shell */
#define FIBB_DELETE    0	/* prevent file from being deleted */

FileInfoBlock _fib = { 0 };

#include <sys/stat.h>
#include <string.h>

void pokeString(std::string s, char* dest, int maxlen) {
	int n = s.size();
	if (n > maxlen) n = maxlen;
	memcpy(dest, s.data(), n);
	dest[n] = 0;
}
void ekopString(std::string s, char* dest, int maxlen) {
	int n = s.size();
	if (n > maxlen) n = maxlen;
	int i = 0;
	while (i < n) {
		dest[(i & -4) | (3 - (i & 3))] = s[i];
		i++;
	}
	dest[(i & -4) | (3 - (i & 3))] = 0;
}

#include <filesystem>

typedef std::vector<uint8_t> Blob;

int fileCount=0;

struct NativeFile {
	int exclusiveLock=0;
	std::set<int> bcplLocks;
	std::string filePath;
	FILE *fileHandle=NULL;
	struct stat fileStat;
	int status;
	int isTemp;
	int isDir;
	std::filesystem::directory_iterator fileIterator;

	int nextLock() {
		return FILE_STREAM - (fileCount++) * 4;
	}

	std::string fileName() {
		std::filesystem::path p(filePath);
		return p.filename().string();
	}

	int addLock(bool exclusive) {
		int h = nextLock();
		if (exclusive) {
			if (bcplLocks.size() || exclusiveLock || fileHandle) {
				return 0;
			}
			exclusiveLock = h;
			return h;
		}
		bcplLocks.insert(h);
		return h;
	}

	void unlock(int h) {
		if (exclusiveLock == h) {
			exclusiveLock = 0;
		}else{
			bcplLocks.erase(h);
		}
	}

	// preset lock constructor for stdin

	NativeFile(int lock, std::string path) : NativeFile(path) {
		bcplLocks.insert(lock);
	}

	NativeFile(std::string path) {
		filePath = path;
		isTemp = 0;
		isDir = 0;
		if (path == "stdin") {
			fileHandle = stdin;
			fileStat = { 0 };
			status = 0;
			return;
		}
		if (path == "stdout") {
			fileHandle = stdout;
			fileStat = { 0 };
			status = 0;
			return;
		}
		if (path == "T:") {
			fileStat = { 0 };
			status = 0;
			isTemp = 1;
			isDir = 1;
			return;
		}
		statFile();
	}

	void statFile() {
		int res = stat(filePath.c_str(), &fileStat);
		if (res == 0) {
			isDir = (fileStat.st_mode & S_IFDIR) ? 1 : 0;
		}
		status=res;
	}

	NativeFile() {
	}

	NativeFile(const NativeFile&f) {
		filePath = f.filePath;
		int res = stat(filePath.c_str(), &fileStat);
		status = res;
	}

	int nextEntry() {
		if (status == 0) {
			fileIterator = std::filesystem::directory_iterator(filePath);
			status = 1;
		}
		if (status == 2) {
			fileIterator++;
			status = 1;
		}
		if(status==1){
			if (fileIterator==std::filesystem::directory_iterator()) {
//			if (fileIterator._At_end()) {
				status = 3;
				return 0;
			}
			status = 2;
			return 1;
		}
		return 0;
	}

	int open(int mode) {

		if (fileHandle) {
			return 0;
		}

//		if (status) return 0;
		const char* m;
		switch (mode) {
		case 1005://MODE_OLDFILE
//			m = "r+b";
			m = "rb";
			break;
		case 1006://MODE_NEWFILE
			m = "wb";	// was w+b
			break;
		case 1004://MODE_READWRITE
			m = "a+b";
			break;
		}
		// TODO: interpret amiga mode to fopen _Mode
		fileHandle = fopen(filePath.c_str(), m);
		statFile();
		status = (fileHandle) ? 0 : -1;
		return fileHandle?1:0;
	}

	void close() {
		if (fileHandle) {
			fclose(fileHandle);
			fileHandle = 0;
		}
		else {
			//todo - lha needs help
		}
	}

	Blob read(int length) {
		Blob blob;
		uint8_t c;
		for (int i = 0; i < length; i++) {
			int n = fread(&c, 1, 1, fileHandle);
			if (n == 0) 
				break;
			if (n < 0) {
				break;
			}
			blob.push_back(c);
		}
		return blob;
	}

	int write(Blob blob) {
		if (fileHandle == 0) {
			return -1;
		}
		int size = blob.size();
		int n = fwrite(blob.data(), 1, size, fileHandle);
		return n;
	}

	int seek(int offset, int mode) {
		int oldpos = ftell(fileHandle);
//		int origin = (mode == -1) ? SEEK_CUR : ((mode == 0) ? SEEK_SET : SEEK_END);
		int origin = (mode == -1) ? SEEK_SET : ((mode == 0) ? SEEK_CUR : SEEK_END);
		int res = fseek(fileHandle, offset, origin);
		if (res) {
// simon come here as this crashes lha
//			return -1;
		}
		int currentpos = ftell(fileHandle);
		return (int)currentpos;
//		return oldpos;
	}
};

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

#include <map>

typedef std::map<std::string, NativeFile> FileMap;	// never removed
typedef std::map<int, NativeFile*> FileLocks;  // sometimes collected


/*

*/


std::map<int, std::string> modeNames = { {1005,"OLDFILE"},{1006,"NEWFILE"},{1004,"READWRITE"} };
std::map<int, std::string> lockNames = { {-1,"EXCLUSIVE"},{-2,"SHARED"} };
std::map<int, std::string> seekNames = { {-1,"BEGINNING"},{0,"CURRENT"},{1,"END"} };

class aciddos : public IDos {
	FileMap fileMap;
	FileLocks fileLocks;
	acidlog doslog;

	enum modes {
		MODE_OLDFILE = 1005,   // Open existing file read/write positioned at beginning of file. 
		MODE_NEWFILE = 1006,   // Open freshly created file (delete old file) read/write, exclusive lock. 
		MODE_READWRITE = 1004   // Open old file w/shared lock, creates file if doesn't exist. 
	};

	enum locks {
		SHARED_LOCK=-2,	    // File is readable by others
		EXCLUSIVE_LOCK=-1   // No other access allowed
	};

	void emit() {
		std::string s = doslog.str();
		systemLog("dos", s);
		doslog.clr();
	}

public:
	
	acid68000* cpu0;

	aciddos(acid68000* cpu) {
		cpu0 = cpu;
		doslog.clr();

		fileMap["stdin"] = NativeFile(INPUT_STREAM, "stdin");
		fileMap["stdout"] = NativeFile(OUTPUT_STREAM, "stdout");

		fileLocks[INPUT_STREAM] = &fileMap["stdin"];
		fileLocks[OUTPUT_STREAM] = &fileMap["stdout"];
	}

// http://amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_3._guide/node0196.html
	void getvar() {
		int d1 = cpu0->readRegister(1);
		int d2 = cpu0->readRegister(2);
		int d3 = cpu0->readRegister(3);
		int d4 = cpu0->readRegister(4);
		std::string name = cpu0->fetchString(d1);
		cpu0->writeRegister(0,-1); // not defined
		doslog << "getvar " << name << " => " << -1;
		emit();
		return;
	}

	void open() {
		int d1 = cpu0->readRegister(1);//name
		int d2 = cpu0->readRegister(2);//mode
		std::string s = cpu0->fetchPath(d1);
		if (fileMap.count(s)==0) {
			fileMap[s] = NativeFile(s);
		}
		NativeFile* f = &fileMap[s];
		bool exclusive = (d2 == MODE_NEWFILE);
		int lock = f->addLock(exclusive);
		int success = 0;
		if (lock) {
			success = f->open(d2);
			if (success) {
				fileLocks[lock] = f;
			}
			else 
			{
				f->unlock(lock);
			}
		}
		int result = success ? lock : 0;
		cpu0->writeRegister(0, result);

//		doslog << "open " << s << " " << std::dec << d2 << " => " << std::hex <<result;
		doslog << "open " << s << " " << modeNames[d2] << " => " << result;
		emit();
	}

	void close(){
		int d1 = cpu0->readRegister(1); //file
		NativeFile* f = fileLocks[d1];
		f->unlock(d1);
		f->close();
		cpu0->writeRegister(0, 0);	//RETURN_OK
		doslog << "close " << d1 << " => 0";
		emit();
	}

	void read(){
		int d1 = cpu0->readRegister(1); //file
		int d2 = cpu0->readRegister(2); //buffer physicalAddress
		int d3 = cpu0->readRegister(3); //length
		int result = 0;
		if (d1) {
			NativeFile* f = fileLocks[d1];
			Blob blob = f->read(d3);
			int n = blob.size();
			for (int i = 0; i < n; i++) {
				cpu0->write8(d2 + i, blob[i]);
			}
			for (int i = n; i < d3; i++) {
				cpu0->write8(d2 + i, 0);
			}
			result = n;
		}
		cpu0->writeRegister(0, result);

		doslog << "read " << d1 << " , " << d2 << " , " << d3 << " => " << result;
		emit();
	}

// http://amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_3._guide/node01D1.html
// 
	void write(){
		int d1 = cpu0->readRegister(1);
		int d2 = cpu0->readRegister(2);
		int d3 = cpu0->readRegister(3);
// TODO: handle odd address in d2 and odd length in d3
// or consider a fetchBytes command, little endians go sleep now

// std::vector<u16> raw = cpu0->fetchShorts(d2, d3);
		std::vector<u8> raw = cpu0->fetchBytes(d2, d3);
// file,buffer,length
		switch (d1) {
		case OUTPUT_STREAM: {
			std::string s = rawString(raw,false);
			systemLog("write", s);
			break;
		}
		default: {
			NativeFile* f = fileLocks[d1];
			d3=f->write(raw);
			systemLog("write", "blob");
			break;
		}
		}
		cpu0->writeRegister(0, d3);
	}
	void input(){
		cpu0->writeRegister(0, INPUT_STREAM);
		doslog << "input";
		emit();
	}
	void output(){
		cpu0->writeRegister(0, OUTPUT_STREAM);
		doslog << "output";
		emit();
	}

	void seek(){
		int d1 = cpu0->readRegister(1);//file
		int d2 = cpu0->readRegister(2);//position	
		int d3 = cpu0->readRegister(3);//mode start,current,end  -1,0,1
		NativeFile* f = fileLocks[d1];
		int pos=f->seek(d2, d3);
		cpu0->writeRegister(0, pos);
//		doslog << "seek " << d1 << "," << d2 << "," << d3 << " => " << pos;
		doslog << "seek " << d1 << "," << d2 << "," << seekNames[d3] << " => " << pos;
		emit();
	}

	void deletefile(){
		doslog << "deletefile";emit();
	}
	void rename(){
		doslog << "rename"; emit();
	}
	//http://amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_3._guide/node0186.html

	void datestamp() {
		int d1 = cpu0->readRegister(1);//{day,min,ticks}

		int dmt[3];
		dayminticks(dmt);

		cpu0->write32(d1 + 0, dmt[0]);
		cpu0->write32(d1 + 4, dmt[1]);
		cpu0->write32(d1 + 8, dmt[2]);

		cpu0->writeRegister(0, d1);

		doslog << "datestamp " << d1;
		emit();
	}
	void unloadseg() {
		doslog << "unloadseg "; emit();
	}
	void delay() {
		int d1 = cpu0->readRegister(1);//{ticks}

		int ms = d1 * 20;
		Sleep(ms);

		doslog << "delay " << d1; emit();
	}

	void loadseg() {
		int d1 = cpu0->readRegister(1);
		std::string segname = cpu0->fetchPath(d1);
		int seglist = 0;
		int physical = 0x050000;
		Chunk chunk = loadChunk(segname, physical);
		int n = chunk.size();
		for (int i = 0; i < n; i++) {
			acid500.qwrite16(physical + i * 2, chunk[i]);
		}
		seglist = (n==0)?0:physical >> 2;
		cpu0->writeRegister(0, seglist);

		doslog << "loadseg " << segname << " => " << seglist;
		emit();
	}

	void currentdir() {
		int d1 = cpu0->readRegister(1);	//name
		NativeFile* f = fileLocks[d1];
		// d1=lock return d0=oldlock
		// all paths lead to root - wtf LhA???
		cpu0->writeRegister(0, 0);
		doslog << "currentDir " << d1;
		emit();
	}

	void lock(){
		int d1 = cpu0->readRegister(1);//name
		int d2 = cpu0->readRegister(2);//type
		std::string s = cpu0->fetchPath(d1);
		if (fileMap.count(s) == 0) {
			fileMap[s] = NativeFile(s);
		}
		NativeFile* file = &fileMap[s];

		int success = (file->status == 0) || (d2 == -1);
		int lock = 0;

		if (success) {
			bool exclusive = (d2 == -1);
			lock = file->addLock(exclusive);
			if (lock) {
				fileLocks[lock] = file;
				// yeh nah what?
			}
		}
		
		int result = success ? lock : 0;

		cpu0->writeRegister(0, result);
//		doslog << "lock " << s << "," << d2 << " => " << result;
		doslog << "lock " << s << " " << lockNames[d2] << " => " << result;
		emit();
	}
	void unLock(){
		int d1 = cpu0->readRegister(1);//lock
		NativeFile* f = fileLocks[d1];
		f->unlock(d1);
		doslog << "unlock " << d1;
		emit();
	}
	void dupLock(){
		int d1 = cpu0->readRegister(1);//lock
		if (d1 == 0) {
			cpu0->writeRegister(0, 0);
			return;
		}
		NativeFile* f = fileLocks[d1];

		int lock = f->addLock(false);
		fileLocks[lock] = f;	// count first??

//		NativeFile(f);
		int result = (f->status == 0) ? lock : 0;
		cpu0->writeRegister(0, result);
		doslog << "duplock"; emit();
	}

	void exnext() {
		int d1 = cpu0->readRegister(1);//lock
		int d2 = cpu0->readRegister(2);//fileinfo
		NativeFile* f = fileLocks[d1];
		int success = 0;
		if (f->nextEntry()) {

			const auto& entry = *(f->fileIterator);
			std::filesystem::path p = entry.path();

			std::string s = p.filename().string();
			//			std::filesystem::directory_entry &entry = fileIterator;

			uint64_t size = entry.file_size();
			bool isdir = entry.is_directory();

			_fib.fib_Size = (int)size;
			_fib.fib_NumBlocks = (size + 1023) / 1024;
			_fib.fib_Protection = 0xdd00;	//rwxd
			_fib.fib_DirEntryType = isdir ? 1 : -1;
			//			pokeString(f.filePath,_fib.fib_FileName,108);
//			ekopString(f->filePath, _fib.fib_FileName, 108);
			ekopString(s, _fib.fib_FileName, 108);
			cpu0->writeEndianMem(d2, &_fib, sizeof(_fib));

			success = 1;
		}
		cpu0->writeRegister(0, success);
		doslog << "exnext " << d1 << "," << d2 << " => " << success;
		emit();
	}

	void examine() {
		int d1 = cpu0->readRegister(1);//lock
		int d2 = cpu0->readRegister(2);//fileinfo
		NativeFile* f = fileLocks[d1];

		memset(&_fib, 0, sizeof(FileInfoBlock));
		int success = 0;
		if (f->status == 0) {
			int n = f->fileStat.st_size;
			int mode = f->fileStat.st_mode & 7;
//			_fib.fib_DiskKey = ;
			_fib.fib_Size = n;
			_fib.fib_NumBlocks = (n+1023)/1024;
			_fib.fib_Protection = 0x01;		// 0x0f;
//			_fib.fib_DirEntryType = (f->fileStat.st_mode& _S_IFDIR ) ? 1 : -1 ;
			_fib.fib_DirEntryType = (f->fileStat.st_mode& S_IFDIR ) ? 1 : -1 ;
//			pokeString(f.filePath,_fib.fib_FileName,108);

			std::string filename = f->fileName();
			ekopString(filename, _fib.fib_FileName, 108);
			cpu0->writeEndianMem(d2, &_fib, sizeof(_fib));
			success = 1;
		}
		cpu0->writeRegister(0, success);
		doslog<<"examine("<<d1<<") <= " << success;
		emit();
	}
	void info() {
		doslog << "info"; emit();
	}

	void createdir() {
		int d1 = cpu0->readRegister(1);	//name
		std::string s = cpu0->fetchPath(d1);
#ifdef _WIN32	
		int result=mkdir(s.c_str());
#else
		int result=mkdir(s.c_str(),0777);	//fuck octal
#endif
		int lock = 0;
		if (result == 0) {
			if (fileMap.count(s)==0){
				fileMap[s] = NativeFile(s);
			}
			NativeFile* f = &fileMap[s];
			f->statFile();
			lock = f->addLock(true);
			fileLocks[lock] = f;
		}
		cpu0->writeRegister(0, lock);
		doslog << "createDir " << s << " => " << lock; emit();
	}
	void ioerr() {
		doslog << "ioErr"; emit();
	}
	void createproc() {
		doslog << "createProc"; emit();

	}
	void exit() {
		doslog << "exit"; emit();
	}

	void isinteractive() {
		int d0 = 0;
		int d1 = cpu0->readRegister(1);
		// d1=lock
		switch (d1) {
		case -4:
			d0 = 1;
			break;
		}
		cpu0->writeRegister(0, d0);
		doslog << "isInteractive"; emit();
	}
};

// notes copied from public web for reference purposes only

/*

addintserver

intNum - the Paula interrupt bit number (0 through 14). Processor
			 level seven interrupts (NMI) are encoded as intNum 15.
			 The PORTS, COPER, VERTB, EXTER and NMI interrupts are
			 set up as server chains.
	interrupt - pointer to an Interrupt structure.
			 By convention, the LN_NAME of the interrupt structure must
			 point a descriptive string so that other users may
			 identify who currently has control of the interrupt.

BUGS
	The graphics library's VBLANK server, and some user code, currently
	assume that address register A0 will contain a pointer to the custom
	chips. If you add a server at a priority of 10 or greater, you must
	compensate for this by providing the expected value ($DFF000).

*/

class acidexec : public IExec {
public:
	acid68000* cpu0;
	acidlog execlog;

	void emit() {
		std::string s = execlog.str();
		systemLog("exec", s);
		execlog.clr();
	}

	acidexec(acid68000* cpu) {
		cpu0 = cpu;
		execlog.clr();
	}

	void forbid(){
		execlog << "forbid";emit();
	}
	void permit() {
		execlog << "permit";emit();
	}
	void waitMsg() {
		execlog << "waitmsg";emit();
	}

// http://amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_3._guide/node0222.html

	void openLibrary() {
		int a1 = cpu0->readRegister(9);
		std::string s = cpu0->fetchString(a1);
		int r = 0;
		if (s == "dos.library") {
			r = DOS_BASE;
		}
		else if (s=="intuition.library"){
			r = INTUITION_BASE;			
		}
		else if (s == "nonvolatile.library") {
			r = NONVOLATILE_BASE;
		}
		else if (s == "graphics.library") {
			r = GRAPHICS_BASE;
		}
		else if (s == "mathffp.library") {
			r = MATHFFP_BASE;
		}
		else {
			// todo: build a named map
			r = 0;
			machineError = -1;
		}
		cpu0->writeRegister(0, r);
		
		execlog << "openlibrary " << s << "," << r;
		emit();
	}

	void setSignal() {
		int d0 = cpu0->readRegister(0);//newbits
		int d1 = cpu0->readRegister(1);//mask
		int bits=cpu0->setSignal(d0,d1);
		cpu0->writeRegister(0, bits);
#ifdef LOG_SETSIGNAL
		execlog << "setSignal " << d0 << "," << d1 << " <= " << bits;
		execlog << " ; "<< sigbits(d0) << " , " << sigbits(d1) << " <= " << sigbits(bits);
		emit();
#endif
	}

	int nextSignal = 16;

	void allocSignal() {
		int d0 = cpu0->readRegister(0);//preferebce
		int signum=(d0>0)?d0:nextSignal++;
//		bits=1<<
		cpu0->writeRegister(0, signum);
		execlog << "allocSignal " << d0 << " <= " << signum;
		emit();
	}

	void closeLibrary() {
		execlog << "closeLibrary";emit();
	}

// http://www.amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_2._guide/node036C.html

	// emit some code to writechar the result and push it on the stack
	//
	// moveq #c0,d0
	// jsr (a2)
	// moveq #c1,d0
	// jsr (a2)
	// 
	// moveq #00,d0
	// jsr (a2)
	// 
	// rts

	void doIO() {
		int a1 = cpu0->readRegister(9);//ioreq
		int r = 0;
		cpu0->writeRegister(0, r);
		execlog << "doIO " << a1 << " <= " << r; emit();
	}
	void openDevice() {
		int a0 = cpu0->readRegister(8);//name
		int d0 = cpu0->readRegister(0);//unit
		int a1 = cpu0->readRegister(9);//ioreq
		int d1 = cpu0->readRegister(1);//flags
		// TODO: use with fetchPath
		std::string devname = cpu0->fetchString(a0);
		int r = -1;
		cpu0->writeRegister(0, r);
		execlog << "openDevice " << devname << "," << d0 << "," << a1 << "," << d1 << " <= " << r; emit();
	}

	void rawDoFmt() {
		int a0 = cpu0->readRegister(8);//fmt
		int a1 = cpu0->readRegister(9);//args
		int a2 = cpu0->readRegister(10);//putchproc
		int a3 = cpu0->readRegister(11);//putchdata

		std::string fmt = cpu0->fetchString(a0);
//		systemLog("fmt", fmt);
		std::stringstream ss;

		for (auto i = 0; i < fmt.length(); i++) {
			char c = fmt[i];
			if (c == '%') {
				int len = 2;
				int word = 0;
				int decode = 1;
				int dec = 0;
				char pad = 0;
				std::string s;
				while(decode){
					char d = fmt[++i];
					if (d >= '0' && d <= '9') {
						if (d == 0) pad = '0';
						dec = dec * 10 + (int)(d - '0');
						continue;
					}
					switch (d) {	//b=bstr d=decimal u=unsigned x=hex s=str
						case '%':
							ss << '%';
							decode = 0;
							break;
						case 's': {
							int p = cpu0->read32(a1);
							s = cpu0->fetchString(p);
							a1 += 4;
							ss << s;
							decode = 0; }
							break;
						case 'l':
							len = 4;
							break;
						case 'd':
							if (len == 2) {
								word = cpu0->read16(a1);
								a1 += 2;
							}
							else {
								word = cpu0->read32(a1);
								a1 += 4;
							}
							ss << (int)word;
							decode = 0;
							break;
						case 'c':
							if (len == 2) {
								d = cpu0->read16(a1); 
								a1 += 2;
							}
							else {
								d = cpu0->read32(a1);
								a1 += 4;
							}
							ss << (char)d;
							decode = 0;
							break;
						default:
							break;
					}
				}
			}
			else {
				ss << (char)c;
			}

		}

		ss << '\0';
		std::string s = ss.str();
		int n = s.length();

		// todo: check null terminated

		int scratch = cpu0->allocate(n*4+2,0);
		for (int i = 0; i < n; i++) {
//			int moveqvald0 = 0xe000 | (255 & s[i]);
			int moveqvald0 = 0x7000 | (255 & s[i]);
			cpu0->write16(scratch + i * 4 + 0,moveqvald0);
			cpu0->write16(scratch + i * 4 + 2,0x4e92);	//jsr(a2)
		}
		cpu0->write16(scratch + n * 4, 0x4e75);

		cpu0->push(scratch);

		execlog << "fmt " << fmt << " => " << s;
		emit();

//		machineError = scratch;
//		cpu0->memoryError = scratch;

// TODO: interpret datastream from the docs, generate instructionlist
// TODO: return args ptr (a1) at new pos
	}

// TODO heap symantics
// http://amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_2._guide/node0332.html

	void availMem() {
		int d1 = cpu0->readRegister(1);	//attributes
		int bytes = 0x400000;
		cpu0->writeRegister(0, bytes);
		execlog << "availMem " << d1 << " => " << bytes; emit();
	}

	void freeMem() {
		int a1 = cpu0->readRegister(9);
		int d0 = cpu0->readRegister(0);
		// a1,d0
		execlog << "freemem";emit();
	}

	void allocMem() {
		int d0 = cpu0->readRegister(0);
		int d1 = cpu0->readRegister(1);
		int r = cpu0->allocate(d0, d1);
		cpu0->writeRegister(0, r);
		execlog << "allocMem " << d0 << "," << d1 << " => " << r;
		emit();
	}

	void waitPort() {
		int a0 = cpu0->readRegister(8);
		cpu0->writeRegister(0, 0);	// no message available
		execlog << "waitPort";emit();
	}
	void copyMem() {
		int a0 = cpu0->readRegister(8); //src
		int a1 = cpu0->readRegister(9); //dest
		int d0 = cpu0->readRegister(0); //size
		for (int i = 0; i < d0; i++) {
			cpu0->write8(a1 + i, cpu0->read8(a0 + i));
		}
		execlog << "copyMem " << d0;
		emit();
	}
	void replyMsg() {
		int a1 = cpu0->readRegister(9);
		std::string s = cpu0->fetchString(a1);
		execlog << "replyMsg";emit();
	}
	void fakeTask() {
		// to trap $ac(task) oblivion and friends are looking for workbench pointers
//		cpu0->writeRegister(0, 0x801000);
		int task = TASK_BASE;	// 0x80c000;
		cpu0->writeRegister(0, task);
		execlog << "findTask <= " << task; emit();
	}
	void getMsg() {
		int a0 = cpu0->readRegister(8);
		cpu0->writeRegister(0, 0);	// no message available
		execlog << "getMsg";emit();
	}
	void putMsg() {
		execlog << "putMsg"; emit();
	}
};
// b8 184
#ifdef _structcode
// structcode in which 
// long offsets are capitalized 
// word offsets are underscored
structcode taskStruct({
// list:
	{"Head,Tail,Pred,type,pad",{0,4,8,12,13},
// node: 
	{"Succ,Pred,type,pri,Name",{0,4,8,9,10}},
// task: 
	{"Node,flags,state,id,td, Alloc,Wait,Recvd,Except,",{0,14,15,16,17, 18,22,26,30}},
	{"_trapAlloc,_trapEnable,ExceptData,ExceptCode,TrapData,TrapCode,",{34,36, 38,42,46,50}},
	{"StackPointer,StackFloor,StackCeil,Switch,Launch",{54,58,62,66,70}},
	{"MemList,User",{74,88}
});
#endif

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

	std::filesystem::path p(path);
	std::string filename=p.filename().string();

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

const char* title = "☰☰☰☰☰☰☰☰☰☰ 🟠 ACID500 monitor";
const char* help = "[s]tep [o]ver [c]ontinue [pause] [r]eset [h]ome [q]uit";

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

void debugRom(int pc24,const char *name,const char *args,const char *home) {

	acidexec *bass=new acidexec(&acid500);
	aciddos* sub = new aciddos(&acid500);
	acidbench* bench = new acidbench(&acid500);
	acidgraphics* gfx = new acidgraphics(&acid500);
	acidnonvolatile* nvram = new acidnonvolatile(&acid500);
	acidfastmath* math = new acidfastmath(&acid500);
	
	mig.setExec(bass);
	mig.setDos(sub);
	mig.setBench(bench);
	mig.setNonVolatile(nvram);
	mig.setGraphics(gfx);
	mig.setMath(math);
	
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
	m68k_set_cpu_type(M68K_CPU_TYPE_68000);
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

			disassemble(pc, ASM_LINES);
			writeEOL();

			displayLogLines(LOG_LINES);

//			acid500.dumplog(5);

			writeString(help);
			writeEOL();
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


	std::cout << std::endl << std::endl << "Write log to disk? (y/N)";

	acid500.writeLog("skidtool.log");

#ifdef trace_log
	acid500.writeTrace("trace.log");
#endif

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


// main entry point


int main() {

	std::cout << "ACID500 skidtool 0.3" << std::endl;
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

			Sleep(100);
		}
#endif
	}
#endif

	int rows, cols;
	screenSize(rows, cols);
//	mouseOn();
	std::cout << "rows:" << rows << " cols:" << cols << std::endl;

#ifdef WIN32
	SetConsoleOutputCP(CP_UTF8);
//	SetConsoleCP(CP_UTF8);
#endif

#ifdef NCURSES
	initscr();
	timeout(200);
#endif

// amiga_binary
// 
//	const char* amiga_binary = "../archive/blitz2/blitz2";
//	const char* args = "-c test.bb\n";

//	const char* amiga_binary = "../archive/genam";
//	const char* args = "test.s -S -P\n";

//	const char* amiga_binary = "../archive/lha";
//	const char* amiga_args= "e cv.lha\n";
//	const char* amiga_args = "e skid.lha\n";
//	const char* amiga_home = ".";
//	const char* args = "l skid.lha\n";
//	const char* args = "e cv.lha\n";

//	const char* amiga_binary = "../archive/guardian";
//	const char* amiga_binary = "../archive/virus";
//	const char* amiga_binary = "../archive/oblivion/oblivion";

//	const int nops[] = {0x63d6, 0};

	const char* amiga_binary = "../archive/genam";
	const char* amiga_args = "blitz2.s -S -P\n";
	const char* amiga_home = "blitz2\\src";

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
	getchar();
#endif

//	const char* name = "lha @ ROM_START";

	std::string name = std::string("hunk:")+amiga_binary+" args:"+amiga_args;

	debugRom(ROM_START, name.c_str(), amiga_args, amiga_home);

//  kickstart sanity test
//	debugCode(0xf800d2);

	return 0;
};
