// acid500 
// 
// monitor aka skidtool by simon
// 
// all rights reserved 2023

#include <assert.h>
#include <sstream>
#include <vector>
#include <set>
#include <algorithm>
#include <fstream>

#include "machine.h"

#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif

const int ASM_LINES = 6;
const int LOG_LINES = 24;

std::vector<logline> machineLog;

// strips \n

//void systemLog(const char* a, std::stringstream &ss) {
//	systemLog(a, ss.str());
//}

void flushLog() {
	for (auto it : machineLog) {
		std::cout << it << std::endl;
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

#define RUN_CYCLES_PER_TICK 1024

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

#include "tty_getch.h"
#include <time.h>

#ifdef LINUX2

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
const char intshortbyte[] = { 'l','s','b','?' };

struct MemEvent : Stream {
	int time;
	int address; // bit31 - R=0 W=1 bit 30-29 - byte,short,int 
	int data;
	int pc;

	MemEvent(int t32,int a32, int d32, int pc32) :time(t32), address(a32), data(d32), pc(pc32) {}

	std::string toString() {

		out.clear();

		int t32 = time;
		int a32 = address;
		int d32 = data;
		int pc32 = pc;
		
		int star = (a32 >> 31) & 1;
		int rw = (a32 >> 30) & 1;
		int opsize = (a32 >> 28) & 3;
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
		writeEOL();

		return flush();
	}
};

typedef std::vector<MemEvent> MemEvents;

//int m68k_execute(int num_cycles)

const int DumpLimit=5000;

struct acid68000 {

	int step=0;
	int cycle=0;

	int memoryError = 0;

	// the active memory pointer is enabled during address decode 
	memory32* mem;
	MemEvents memlog;

	int heapPointer = 0x60000;

	std::set<std::uint32_t> breakpoints;

	std::string fetchString(int a1) {
		std::stringstream ss;
		while (a1) {
			int byte = read8(a1++);
			if (byte == 0) break;
			ss << (char)byte;
		}
		return ss.str();
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
		size = (size + 3) & -4;
		int p = heapPointer;
		heapPointer += size;
		if (bits & 1) {
			for (int i = 0; i < size; i += 4) {
				qwrite32(p + i, 0);
			}
		}
		return p;
	}

	// address is 24 bit physical with qbit signals in high bits

	void log_bus(int readwritefetch, int byteshortint, int address, int value) {
		bool enable=(readwritefetch==1)?(mem->flags&2):(mem->flags&1);
		int star=(mem->flags&4)?1:0;
		int err = (address == memoryError)?1:0;
		if(enable||err){
			// low 24 bits are physical address
			int a32 = ((star|err) << 31) | (readwritefetch << 29) | (byteshortint << 27) | (address & 0xffffff);
			int pc=readRegister(16);
			memlog.emplace_back(cycle, a32, value, pc);

			std::string s=memlog.back().toString();
			systemLog("mem", s);
		}
	}

//	void dumpEvent(std:stringstream & out, MemEvent& e) {
//	}

	void writeLog(std::string path) {
		std::ofstream fout(path);
		for (auto m : machineLog) {
			fout << m << std::endl;
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

	void writeMem(int physicalAddress, void *src, size_t size) {
		int n = size / 4;
		uint32_t* l = (uint32_t *)src;
		for (int i = 0; i < n; i++) {
			write32(physicalAddress+i*4, l[i]); // good endian?
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

		if (qbits & 0x80000000) {	// pc is on the bus, check breakpoints
			if (breakpoints.count(a32)) {
				memoryError = physicalAddress;
				m68k_pulse_halt();
				return 0;
			}
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
		if(!memoryError) mem->write8(address, value);
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
//		address|=QBIT;
		mem->write32(address, value);
	}
	void qwrite16(int physicalAddress, int value) {
		int address = decode(physicalAddress);
		mem->write16(address, value);
	}

	void breakpoint(int address) {
		breakpoints.insert(address|0x80000000);
	}
};

acid68000 acid500;

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

struct NativeFile {
	std::vector<int> bcplLocks;
	std::vector<int> bcplUnlocks;
	std::string filePath;
	FILE *fileHandle;
	struct stat fileStat;
	int status;
	int isTemp;
	int isDir;
	std::filesystem::directory_iterator fileIterator;

	void addLock(int h) {
		bcplLocks.push_back(h);
	}

	void newLock(int h, std::string path) {
		// assert path==filePath
		bcplLocks.push_back(h);
		int res = stat(path.c_str(), &fileStat);
		status = res;
		if (res == 0) {
			isDir = (fileStat.st_mode & _S_IFDIR) ? 1 : 0;
		}
	}

	NativeFile(int h, std::string path) {
		addLock(h);
		filePath = path;
		fileHandle = 0;
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
		int res = stat(path.c_str(), &fileStat);
		status = res;
		if (res == 0) {
			isDir = ( fileStat.st_mode & _S_IFDIR )? 1:0;
		}

	}

	NativeFile() {

	}

	NativeFile(NativeFile&f) {
		filePath = f.filePath;
		fileHandle = 0;
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
			if (fileIterator._At_end()) {
				status = 3;
				return 0;
			}
			status = 2;
			return 1;
		}
		return 0;
	}

	void unlock(int lock) {
		bcplUnlocks.emplace_back(lock);
//		status = -1;
	}

	int open(int mode) {
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
		status = (fileHandle) ? 0 : -1;
		return fileHandle?1:0;
	}

	void close() {
		if (fileHandle) {
			fclose(fileHandle);
		}
		else {
			//todo - lha needs help
		}
		fileHandle = 0;
	}

	Blob read(int length) {
		Blob blob;
		uint8_t c;
		for (int i = 0; i < length; i++) {
			int n = fread(&c, 1, 1, fileHandle);
			if (n == 0) 
				break;
			blob.push_back(c);
		}
		return blob;
	}

	void write(Blob blob) {
		int size = blob.size();
		int n = fwrite(blob.data(), 1, size, fileHandle);
	}

	int seek(int offset, int mode) {
		int oldpos = ftell(fileHandle);
		int origin = (mode == -1) ? SEEK_SET : (mode == 0) ? SEEK_CUR : SEEK_END;
		fseek(fileHandle, offset, origin);
		int currentpos = ftell(fileHandle);
//		return (int)currentpos;
		return oldpos;
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

class aciddos : public IDos {
	int fileCount = 0;
	FileMap fileMap;
	FileLocks fileLocks;
	acidlog doslog;

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

		std::string s = cpu0->fetchString(d1);
		std::replace(s.begin(),s.end(),'/','\\');

		int lock = nextLock();

		if (fileMap.count(s)) {
			fileMap[s].newLock(lock,s);
		}
		else {
			fileMap[s] = NativeFile(lock, s);
		}
		NativeFile* f = &fileMap[s];
		fileLocks[lock] = f;

		int success=f->open(d2);
		int result = success ? lock : 0;
		cpu0->writeRegister(0, result);

		doslog << "open "<<s<<" => "<<result;
		emit();
	}

	void close(){
		int d1 = cpu0->readRegister(1); //file
		NativeFile* f = fileLocks[d1];
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
			result = n;
		}
		cpu0->writeRegister(0, result);
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
			f->write(raw);
			systemLog("write", "blob");
			break;
		}
		}
		cpu0->writeRegister(0, d3);
	}
	void input(){
		cpu0->writeRegister(0, INPUT_STREAM);
	}
	void output(){
		cpu0->writeRegister(0, OUTPUT_STREAM);
	}

	void seek(){
		int d1 = cpu0->readRegister(1);//file
		int d2 = cpu0->readRegister(2);//position	
		int d3 = cpu0->readRegister(3);//mode start,current,end  -1,0,1

		NativeFile* f = fileLocks[d1];
		int pos=f->seek(d2, d3);
		cpu0->writeRegister(0, pos);
		doslog << "seek " << d2 << "," << d3 << " => " << pos;
		emit();
	}
	void deletefile(){
	}
	void rename(){
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
		std::string segname = cpu0->fetchString(d1);
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
		std::string s = cpu0->fetchString(d1);
		std::replace(s.begin(),s.end(),'/','\\');
		int lock = nextLock();
		if (fileMap.count(s)) {
			fileMap[s].addLock(lock);
		}
		else {
			fileMap[s] = NativeFile(lock, s);
		}
		NativeFile* file = &fileMap[s];
		fileLocks[lock] = file;
		// yeh nah what?
		int success=(file->status == 0) || (d2 == -1);
		int result = success ? lock : 0;
		cpu0->writeRegister(0, result);
		doslog << "lock " << s << "," << d2 << " => " << result;
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
		int lock = nextLock();
		f->addLock(lock);
		fileLocks[lock] = f;	// count first??

//		NativeFile(f);
		int result = (f->status == 0) ? lock : 0;
		cpu0->writeRegister(0, result);
	}

	void exnext() {
		int d1 = cpu0->readRegister(1);//lock
		int d2 = cpu0->readRegister(2);//fileinfo
		NativeFile* f = fileLocks[d1];
		int success = 0;
		if (f->nextEntry()) {

			const auto& entry = *(f->fileIterator);
			std::filesystem::path p = entry.path();
			std::string& s = p.filename().string();
			//			std::filesystem::directory_entry &entry = fileIterator;
			uint64_t size = entry.file_size();
			bool isdir = entry.is_directory();

			_fib.fib_Size = (int)size;
			_fib.fib_Protection = 0x0f;	//rwxd
			_fib.fib_DirEntryType = isdir ? 1 : -1;
			//			pokeString(f.filePath,_fib.fib_FileName,108);
//			ekopString(f->filePath, _fib.fib_FileName, 108);
			ekopString(s, _fib.fib_FileName, 108);
			cpu0->writeMem(d2, &_fib, sizeof(_fib));

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
		int success = 0;
		if (f->status == 0) {
			int n = f->fileStat.st_size;
			int mode = f->fileStat.st_mode & 7;
			_fib.fib_Size = n;
			_fib.fib_Protection = 0x0f;
			_fib.fib_DirEntryType = (f->fileStat.st_mode& _S_IFDIR ) ? 1 : -1 ;
//			pokeString(f.filePath,_fib.fib_FileName,108);
			ekopString(f->filePath, _fib.fib_FileName, 108);
			cpu0->writeMem(d2, &_fib, sizeof(_fib));
			success = 1;
		}
		cpu0->writeRegister(0, success);
		doslog<<"examine("<<d1<<")";
		emit();
	}
	void info() {

	}
	int nextLock() {
		return FILE_STREAM - (fileCount++) * 4;
	}
	void createdir() {
		int d1 = cpu0->readRegister(1);	//name
		std::string s = cpu0->fetchString(d1);
#ifdef _WIN32	
		int result=mkdir(s.c_str());
#else
		int result=mkdir(s.c_str(),0777);	//fuck octal
#endif
		int lock = 0;
		if (result == 0) {
			lock = nextLock();
			if (fileMap.count(s)) {
				fileMap[s].newLock(lock, s);
			}
			else {
				fileMap[s] = NativeFile(lock, s);
			}
			NativeFile* f = &fileMap[s];
			fileLocks[lock] = f;
		}
		cpu0->writeRegister(0, lock);
		doslog << "createDir " << s << " => " << lock; emit();
	}
	void ioerr() {

	}
	void createproc() {

	}
	void exit() {

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

	void forbid(){}
	void permit() {}
	void waitMsg() {}

// http://amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_3._guide/node0222.html

	void openLibrary() {
		int a1 = cpu0->readRegister(9);
		std::string s = cpu0->fetchString(a1);
		int r = 0;
		if (s == "dos.library") {
			r = 0x802000;
		}
		else if (s=="intuition.library"){
			r = 0x803000;			
		}
		else if (s == "nonvolatile.library") {
			r = 0x804000;
		}
		else if (s == "graphics.library") {
			r = 0x805000;
		}
		else if (s == "mathffp.library") {
			r = 0x806000;
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
		int d0 = cpu0->readRegister(8);//newbits
		int d1 = cpu0->readRegister(9);//mask
		cpu0->writeRegister(0, 0);
		execlog << "setsignal " << d0 << "," << d1;
		emit();
	}

	void closeLibrary() {
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
				int d = 0;
				int decode = 1;
				int dec = 0;
				std::string s;
				while(decode){
					char d = fmt[++i];
					if (d >= '0' && d <= '9') {
						dec = dec * 10 + (int)(d - '0');
						continue;
					}
					switch (d) {	//b=bstr d=decimal u=unsigned x=hex s=str
						case '%':
							ss << '%';
							decode = 0;
							break;
						case 's':
							if (a1) s = cpu0->fetchString(a1);
							ss << s;
							decode = 0;
							break;
						case 'l':
							len = 4;
							break;
						case 'd':
							if (len == 2) {
								d = cpu0->read16(a1);
								a1 += 2;
							}
							else {
								d = cpu0->read32(a1);
								a1 += 4;
							}
							ss << (int)d;
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
	}

	void allocMem() {
		int d0 = cpu0->readRegister(0);
		int d1 = cpu0->readRegister(1);
		int r = cpu0->allocate(d0, d1);
		cpu0->writeRegister(0, r);
		execlog << "allocMem " << d0; emit();
	}

	void waitPort() {
		int a0 = cpu0->readRegister(8);
		cpu0->writeRegister(0, 0);	// no message available
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
	}
	void fakeTask() {
		// to trap $ac(task) oblivion and friends are looking for workbench pointers
//		cpu0->writeRegister(0, 0x801000);
		cpu0->writeRegister(0, 0x803000);
		execlog << "findTask"; emit();
	}
	void getMsg() {
		int a0 = cpu0->readRegister(8);
		cpu0->writeRegister(0, 0);	// no message available
	}
	void putMsg() {
		execlog << "putMsg"; emit();
	}
};



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

const char* title = "☰☰☰☰☰☰☰☰☰☰ 🟠 ACID500 monitor";
const char* help = "[s]tep [o]ver [c]ontinue [pause] [r]eset [h]ome [q]uit";

const int SP_START = 0x1400;
const int ROM_START = 0x2000;

//const int SP_START = 0x5400;
//const int ROM_START = 0x6000;

const int ARGS_START = 0x200;

void displayLogLines(int count) {
	int n = machineLog.size();
	for (int i = n - count; i < n; i++) {
		if (i >= 0) {
			std::cout << machineLog[i];
		}
		writeEOL();
	}
}

void debugRom(int pc24,const char *name,const char *args,const int *nops) {

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

	acid500.qwrite32(0, SP_START); //sp
	acid500.qwrite32(4, 0x801000); //exec
	acid500.qwrite32(8, pc24); //pc

	acid500.qwrite32(0x1400, 0x807000); //pc

	while (*nops) {
		int a = *nops++;
		acid500.qwrite16(a, 0x4e75); //pc
	}

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


			disassemble(pc, ASM_LINES);

			displayLogLines(LOG_LINES);

//			acid500.dumplog(5);

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

int main() {
	int rows, cols;
	screenSize(rows, cols);
//	mouseOn();

#ifdef WIN32
	SetConsoleOutputCP(CP_UTF8);
//	SetConsoleCP(CP_UTF8);
#endif

#ifdef NCURSES
	initscr();
	timeout(200);
#endif

	std::cout << "skidtool 0.2" << std::endl;
	std::cout << "rows:" << rows << " cols:" << cols << std::endl;


//	const char* amiga_binary = "../archive/blitz2/blitz2";
//	const char* args = "-c test.bb\n";

//	const char* amiga_binary = "../archive/genam";
//	const char* args = "test.s -S -P\n";

//	const char* amiga_binary = "../archive/lha";
//	const char* args = "e cv.lha\n";
//	const char* args = "e SkidMarksDemo.lha\n";
//	const char* args = "l SkidMarksDemo.lha\n";
//	const char* args = "e cv.lha\n";

//	const char* amiga_binary = "../archive/game";
//	const char* amiga_binary = "../archive/virus";
	const char* amiga_binary = "../archive/oblivion/oblivion";
	const char* args = "\n";

//	const int nops[] = {0x63d6, 0};
	const int nops[] = { 0 };

	loadHunk(amiga_binary,ROM_START);

//#define pause
#ifdef pause
	writeString("enter to continue");
	writeEOL();
	getchar();
#endif

//	const char* name = "lha @ ROM_START";

	std::string name = std::string("hunk:")+amiga_binary+" args:"+args;

	debugRom(ROM_START, name.c_str(), args, nops);

//  kickstart sanity test
//	debugCode(0xf800d2);

	return 0;
};
