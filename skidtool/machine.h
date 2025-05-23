#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <sstream>

#include "exec.h"
#include "monitor.h"
#include "filedecoder.h"

void flattenString(std::string &s);

extern int readClock();

//#define STOP_ON_WRITE

// last allo ffe00

enum ami_mem_map {
	HEAP_START = 0x20000,
//	ARGS_BASE = 0x200,
	AMI_BASE = 0x800000,
	EXEC_BASE = 0x801000,
	DOS_BASE = 0x802000,
	INTUITION_BASE = 0x803000,
	NONVOLATILE_BASE = 0x804000,
	GRAPHICS_BASE = 0x805000,
	MATHFFP_BASE = 0x806000,
	DISKFONT_BASE = 0x807000,
	MATHTRANS_BASE = 0x808000,
	WORKBENCH_BASE = 0x80c000,
	TASK_BASE = 0x80e000,
	BAD_BASE = 0x80f000,
	SAD_BASE = 0x810000,	// not implemented decode error such as WORKBENCH_BASE + 0xac
	ACID_BASE = 0x900000,	// native bridge for acidstructs
};

std::string addressString(int b);
std::string hexValue32(int b);
std::string hexValue16(int b);
std::string hexValue8(int b);

const int ChipsetFlags = 3;

typedef std::string logline;

extern std::vector<std::pair<int,logline>> machineLog;

void systemLog(const char* tag, std::string s);

class acidlog : public std::stringstream {

public:
	void clr() {
		str(std::string());
		*this << std::hex;
	}
};


Chunk16 loadPhysicalChunk(std::string path, int physical);


class IEvent {
public:
	virtual std::string toString() = 0;
};

// physical address and mask == physical address
// big endian byte order
// flag 1 - logs reads
// flag 2 - logs writes
// flag 4 - star

struct MachineEvent {
	int time;
	std::string detail;
};

extern int machineError;

extern std::string machineState;

struct memory32 {
	u32 physical;
	u32 mask;
	u32 flags;

	memory32(u32 phys, u32 bits) : physical(phys), mask(bits), flags(0) {
	}

	// address interface is local with physical page bits and qbits already removed

	virtual int read16(int address, int bits) {
		return 0;
	}

	virtual void write16(int address, int value) {
	}

	// little endian helpers

	virtual int read8(int address) {
		int odd = address & 1;
		int word = read16(address - odd,0);
		word >>= 8 * (1-odd);
		return word & 0xff;
	}

	virtual int read8a(int address) {
		int odd = address & 1;
		int word = read16(address - odd, 0);
		word >>= 8 * odd;
		return word & 0xff;
	}

	virtual int read32(int address) {
		int w0 = read16(address,0);
		int w1 = read16(address+2,0);
//		return (w1 << 16) | (w0 & 0xffff);
		return (w0 << 16) | (w1 & 0xffff);
	}

	virtual void write8(int address, int value) {
		value &= 0xff;
		int odd = address & 1;
		address -= odd;
		int word = read16(address,0);
		if (odd) {
			word = (word & 0xff00) | value;
		}
		else 
		{
			word = (word & 0xff) | (value << 8);
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
			std::cout << "unexpected eof fail for rom16 file" << std::endl;
		}
	}
	virtual int read16(int address,int flags) {
		return shorts[address>>1];
	}
};

struct ram16 : memory32 {
	std::vector<u16> shorts;
	ram16(u32 p,u32 m, int wordCount) : memory32(p,m), shorts(wordCount) {
		flags=0;
	}
	virtual void write16(int address,int value) {
		if(address<0||(address>>1)>=shorts.size()){
			machineError = address;
			return;
		}
		shorts[address>>1]=value;
	}
	virtual int read16(int address,int flags) {
		return shorts[address>>1];
	}
};

// a headless paula denise agnus chipset

struct chipset16 : memory32 {
	std::vector<u16> shorts;
	chipset16(u32 p, u32 m, int wordCount) : memory32(p, m), shorts(wordCount) {
		flags = ChipsetFlags;
		shorts[15]=0x20;//oblivion waits for DSKBLK to be set
	}
	virtual void write16(int address, int value) {
		shorts[address >> 1] = value;
	}
	virtual int read16(int address,int flags) {
		return shorts[address >> 1];
	}
};

struct interface8 : memory32 {
	std::vector<u8> bytes;
	interface8(u32 p, u32 m, int byteCount) : memory32(p, m), bytes(byteCount) {
		flags = 7;
	}
	virtual void write8(int address, int value) {
		bytes[address] = value;
	}
	virtual int read8(int address) {
		return bytes[address];
	}
};

// execbase breakpoint platform 
// common origin is 0x800000
// execbase=

const int QBIT = 0x80000000;
const int DBIT = 0x40000000;

const std::string execNames[] = {"ReplyMsg","WaitPort"}; // just guessing here, please step slowly

enum enum_exec {

	FORBID = -132,
	PERMIT = -138,

	SUPERSTATE = -150,
	USERSTATE = -156,

	SETINTVECTOR = -162,
	ADDINTSERVER = -168,

	ALLOCATE = -186,
	DEALLOCATE = -192,
	ALLOCMEM = -198,
	FREEMEM=-210,
	AVAILMEM=-216,

	FINDTASK=-294,
	SETTASKPRI=-300,
	SETSIGNAL=-306,
	SETEXCEPT=-312,
	WAIT=-318,

	ALLOCSIGNAL=-330,

	REMPORT=-360,
	PUTMSG=-366,
	GETMSG=-372,
	REPLY=-378,
	WAITPORT=-384,

	OLDOPENLIBRARY=-408,
	CLOSELIBRARY=-414,

	OPENDEVICE=-444,

	DOIO=-456,

	RAWDOFMT=-522,
	OPENLIBRARY=-552,
	

	COPYMEM=-624,

	ALLOCVEC = -684,
};

enum enum_intuition {
	INTUITION_CLOSEWORKBENCH = -78,
	INTUITION_GETSCREENDATA=-426
};

enum enum_mathffp {
	SP_FIX=-30,
	SP_FLOAT = -36,
	SP_MUL = -42,
	SP_DIV = -48,
	SP_ADD = -54,
	SP_SUB = -60,
	SP_CMP = -66,
	SP_TST = -72,
	SP_ABS = -78,
	SP_NEG = -84,
	SP_FLOOR = -90,
	SP_CEIL = -96
};

enum enum_nonvolatile {
	NV_GETCOPY = -30,
	NV_FREEDATA = -36,
	NV_STORE = -42,
	NV_DELETE = -48,
	NV_GETINFO = -54,
	NV_GETLIST = -60,
	NV_SETPROTECTION = -66
};

enum enum_graphics {
	GFX_TEXTLENGTH = -54,
	GFX_LOADVIEW = -222,
	GFX_WAITTOF = -270,
	GFX_OWNBLITTER = -456,
	GFX_DISOWNBLITTER = -462
};

enum enum_dos {
	DOS_OPEN = -30,
	DOS_CLOSE = -36,
	DOS_READ = -42,
	DOS_WRITE = -48,
	DOS_INPUT = -54,
	DOS_OUTPUT = -60,
	DOS_SEEK = -66,
	DOS_LOCK = -84,
	DOS_UNLOCK = -90,
	DOS_DUPLOCK = -96,
	DOS_EXAMINE = -102,
	DOS_EXNEXT = -108,
	DOS_CREATEDIR = -120,
	DOS_CURRENTDIR = -126,
	DOS_IOERR = -132,
	DOS_LOADSEG = -150,
	DOS_UNLOADSEG = -156,
	DOS_DATESTAMP = -192,
	DOS_DELAY = -198,
	DOS_ISINTERACTIVE = -216,
	DOS_SETFILEDATE = -396,
	DOS_PRINTFAULT = -474,
	DOS_READARGS = -798,
	DOS_MATCHFIRST = -822,
	DOS_MATCHEND = -834,
	DOS_FREEARGS = -858,
	DOS_GETVAR = -906,
	DOS_VPRINTF = -954
};

// address is 0x800000
// 
// underlying 0x100000 shorts to be removed
//
// ACID_BASE 0x900000 wired to acidstruct::pysical 

struct acidstruct {
	const int blocksize = 4096;
	int physical;
	Chunk16 chunk;
	acidstruct(int index,int words):chunk(words) {
		physical = ACID_BASE + index * blocksize;
	}
};


struct amiga16 : memory32{
	std::vector<u16> shorts;	// deprecate me
	IExec* exec;
	IDos* dos;
	IBench* bench;
	INonVolatile* nvram;
	IGraphics* gfx;
	IFFPMath* math;

	std::vector<acidstruct> native;

	amiga16(u32 p, u32 m, int wordCount) : memory32(p, m), shorts(wordCount) {
		flags=0;
		dos = NULL;
		exec = NULL;
		bench = NULL;
		nvram = NULL;
		gfx = NULL;
	}
	int allocNative(int bytes) {
		int nativeCount = (int)native.size();
		native.emplace_back(nativeCount,bytes);
	}
	void setBench(IBench* work) {
		bench = work;
	}
	void setExec(IExec *bass) {
		exec = bass;
	}
	void setDos(IDos* sub) {
		dos = sub;
	}
	void setNonVolatile(INonVolatile* nv) {
		nvram = nv;
	}
	void setGraphics(IGraphics* graphics) {
		gfx = graphics;
	}
	void setMath(IFFPMath * ffpMath){
		math = ffpMath;
	}

	// read only memory, writing causes machineError

	virtual void write16(int offset, int value) {
		int address = AMI_BASE | offset;
		systemLog("write16", addressString(address)+","+hexValue16(value));
		// simon come here
//		machineError = address;
	}

	// pc has arrived with a negative offset from lib
	// 
	// low 12 bits are offset into 6 byte per entry jump table
	// 
	// library index is next few bits, see ami_mem_map
	//
	// exec supports positive offsets for system vars MaxLocMem AttnFlags

	virtual int read16(int address, int flags) {

		int lib = (address + 2047) >> 12;	//execbase+128=SoftVer
//		int lib = (address + 4095) >> 12;
//		int offset = address | (-1 << 12);
		int offset = address - (lib << 12);

		if (flags == 0) {
			std::stringstream ss;
			ss << addressString(AMI_BASE | address) << " flags:0 <= -1";
			// << " offset:" << offset << " lib:" << lib
			systemLog("read16", ss.str());
			return -1;
		}

		if (flags & QBIT) {
			//		log_bus(0, 1, physicalAddress, 0);
		}
		if (flags & DBIT) {
			return 0x4e75;
		}
		switch (lib) {
		case 1:
			if (offset > 0) {
				switch (offset) {
					case 0x128: // 296	MaxLocMemHi
						return 8;	//top of chip mem 512K = 8 * 64K					
					case 0x14:// 20 AttnFlagsHi
						return 0;
					default:
						return 0;
				}
			}
			machineError = callExec(offset);
			break;
		case 2:
			machineError = callDos(offset);
			break;
		case 3:
			machineError = callIntuition(offset);
			break;
		case 4:
			machineError = callNonVolatile(offset);
			break;
		case 5:
			machineError = callGraphics(offset);
			break;
		case 6:
			machineError = callMath(offset);
			break;
		}
		if (machineError) {
			return 0x4e75;
		}
		return 0x4e75;
	}

	int callMath(int offset) {
		switch (offset) {
		default:
			machineState = std::to_string(offset) + "(ffpmathBase) un supported";
			return offset;
		}
		return 0;
	}

	int callGraphics(int offset) {
		switch (offset) {
		case GFX_TEXTLENGTH:
			gfx->textLength();
			break;
		case GFX_LOADVIEW:
			gfx->loadView();
			break;
		case GFX_WAITTOF:
			gfx->waitTOF();
			break;
		case GFX_OWNBLITTER:
			gfx->ownBlitter();
			break;
		case GFX_DISOWNBLITTER:
			gfx->disownBlitter();
			break;
		default:
			machineState = std::to_string(offset) + "(graphicsBase) un supported";
			return offset;
		}
		return 0;
	}


	int callNonVolatile(int offset) {
		switch (offset) {
		case NV_GETCOPY:
			nvram->getCopy();
			break;
		default:
			machineState = std::to_string(offset) + "(nonvolatileBase) un supported";
			return offset;
		}
		return 0;
	}

	int callIntuition(int offset) {
		switch (offset) {
		case INTUITION_CLOSEWORKBENCH:
			bench->closeWorkBench();
			break;
		case INTUITION_GETSCREENDATA:
			bench->getScreenData();
			break;
		default:
			machineState = std::to_string(offset) + "(intuitionBase) un supported";
			return offset;
		}
		return 0;
	}

	int callMathFFP(int offset) {

	}

	int callDos(int offset) {
		switch (offset) {

		case DOS_PRINTFAULT:
			dos->printfault();
			break;
		case DOS_IOERR:
			dos->ioerr();
			break;
		case DOS_OPEN:
			dos->open();			
			break;
		case DOS_CLOSE:
			dos->close();
			break;
		case DOS_READARGS:
			dos->readargs();
			break;
		case DOS_MATCHFIRST:
			dos->matchfirst();
			break;
		case DOS_MATCHEND:
			dos->matchend();
			break;
		case DOS_FREEARGS:
			dos->freeargs();
			break;
		case DOS_GETVAR:
			dos->getvar();
			break;
		case DOS_VPRINTF:
			dos->vprintf();
			break;
		case DOS_ISINTERACTIVE:
//			return offset;
			dos->isinteractive();
			break;
		case DOS_SETFILEDATE:
			dos->setfiledate();
			break;
		case DOS_CURRENTDIR:
			dos->currentdir();
			break;
		case DOS_LOADSEG:
			dos->loadseg();
//			return 1;
			break;
		case DOS_UNLOADSEG:
			dos->unloadseg();
			break;
		case DOS_DATESTAMP:
			dos->datestamp();
			break;
		case DOS_DELAY:
			dos->delay();
			break;
		case DOS_READ:
			dos->read();
			break;
		case DOS_WRITE:
			dos->write();
#ifdef STOP_ON_WRITE
			return 1;
#endif
			break;
		case DOS_INPUT:
			dos->input();
			break;
		case DOS_OUTPUT:
			dos->output();
			break;
		case DOS_SEEK:
			dos->seek();
			break;
		case DOS_LOCK:
			dos->lock();
			break;
		case DOS_UNLOCK:
			dos->unLock();
			break;
		case DOS_DUPLOCK:
			dos->dupLock();
			break;
		case DOS_EXAMINE:
			dos->examine();
//			return 1;
			break;
		case DOS_EXNEXT:
			dos->exnext();
			break;
		case DOS_CREATEDIR:
			dos->createdir();
			break;
		default:
			machineState = std::to_string(offset) + "(dosBase) un supported";
			return offset;
		}
		return 0;
	}

	int callExec(int offset){
		switch (offset) {
		case FORBID:
			systemLog("exec", "forbid");
			break;
		case PERMIT:
			systemLog("exec", "permit");
			break;
		case SUPERSTATE:
			exec->superState();
			break;
		case USERSTATE:
			exec->userState();
			break;
		case SETINTVECTOR:
			systemLog("exec", "setintvector");
			break;
		case ADDINTSERVER://d0,d1 intnum,handler
			systemLog("exec", "addintserver");
			break;
		case DOIO:
			exec->doIO();
			break;
		case OPENDEVICE:
			exec->openDevice();
			break;
		case RAWDOFMT:
			exec->rawDoFmt();
//			return 1;
			break;
		case ALLOCVEC:
		case ALLOCMEM:
		case ALLOCATE:
			exec->allocMem();
			break;
		case FREEMEM:
			exec->freeMem();
			break;
		case AVAILMEM:
			exec->availMem();
			break;
		case DEALLOCATE:
			systemLog("exec", "dellocate");
			break;
		case ALLOCSIGNAL:
//			systemLog("exec", "allocsignal");
			exec->allocSignal();
			break;
		case WAIT:
			systemLog("exec", "wait");
			break;
		case OLDOPENLIBRARY: 
		case OPENLIBRARY:
			exec->openLibrary();
			if (machineError) 
				return 1;
			break;
		case CLOSELIBRARY:
			exec->closeLibrary();
			break;
		case SETEXCEPT:
			systemLog("exec", "setExcept");
			break;
		case SETSIGNAL:
			exec->setSignal();
//			return 1;
			break;
		case SETTASKPRI:
			systemLog("exec", "setTaskPri");
			break;
		case FINDTASK:
			exec->fakeTask();
			break;
		case GETMSG:
			exec->getMsg();
			break;
		case PUTMSG:
			exec->putMsg();
			break;
		case REPLY:
			// A1=message
			machineState = "REPLY";
			exec->replyMsg();
			break;
		case WAITPORT:
			machineState = "WAITPORT";
			exec->waitPort();
			break;
		case COPYMEM:
			exec->copyMem();
			break;
		default:
			machineState = std::to_string(offset) + "(execBase) un supported";
			return offset;
		}
		return 0;
	}
	
	int readAddress(int address) {		

		// trap $114(execbase) for apps such as blitz2 and lha looking for workbench pointers
		if (address == (EXEC_BASE)) {
			return 0;
		}

		if (address == (EXEC_BASE + 0x114) ) {
			return WORKBENCH_BASE;
		}
		if (address == (WORKBENCH_BASE + 0xac)) {
			return SAD_BASE;	// offset;
		}
		if (address == (WORKBENCH_BASE + 0x98)) {
			return 0;	// SYSTEM ROOT LOCK
		}
		if (address == (DOS_BASE + 0x22)) {
			return BAD_BASE;
//			return offset;
		}
		if (address == (TASK_BASE + 0xb8)) {	//lha stores pointer before writing -1
			return 0xdeadbeef;
		}
		if (address == (TASK_BASE + 0xa4)) {	//lha
			return 0xcafeb0b0;
		}
		if (address == (TASK_BASE + 0xac)) {	//oblivion
			return 0xb00bcafe;
		}
		return address;
	}

	virtual int read32(int offset) {		
		int address=AMI_BASE|offset;
		int value32=readAddress(address);
		systemLog("read32", addressString(address)+","+hexValue32(value32));
		return value32;
	}

};
