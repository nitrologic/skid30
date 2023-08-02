#pragma once

#include <string>

#include "exec.h"
#include "monitor.h"
#include "filedecoder.h"

// physical address and mask == physical address
// big endian byte order
// flag 1 - logs reads
// flag 2 - logs writes
// flag 4 - star

int machineError;

std::string machineState="";

struct memory32 {
	u32 physical;
	u32 mask;
	u32 flags;

	memory32(u32 phys, u32 bits) : physical(phys), mask(bits) {
	}

	// address interface is local with physical bits already removed

	virtual int read16(int address, int flags) {
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

	virtual int read32(int address) {
		int w0 = read16(address,0);
		int w1 = read16(address+2,0);
//		return (w1 << 16) | (w0 & 0xffff);
		return (w0 << 16) | (w1 & 0xffff);
	}

	virtual void write8(int address, int value) {
		int odd = address & 1;
		address -= odd;
		int word = read16(address,0);
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
		flags=0;
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
	ALLOCATE=-2,
	DEALLOCATE=-1,
	ALLOCMEM=0,
	FINDTASK=14,
	SETTASKPRI=15,
	SETSIGNAL=16,
	SETEXCEPT=17,
	WAIT=18,

	REMPORT=27,
	PUTMSG=28,
	GETMSG=29,
	REPLY=30,
	WAITPORT=31,
	OLDOPENLIBRARY=35,
	OPENLIBRARY=59,

	DOSOPEN=118
};

// address is 0x800000
// 
// underlying 0x100000 shorts to be removed

struct amiga16 : memory32{
	std::vector<u16> shorts;	// deprecate me
	IExec* exec;
	IDos* dos;

	amiga16(u32 p, u32 m, int wordCount) : memory32(p, m), shorts(wordCount) {
		flags=0;
	}
	void setExec(IExec *bass) {
		exec = bass;
	}
	void setDos(IDos* sub) {
		dos = sub;
	}
	// pc has arrived with a negative offset from lib
	// execbase ($801000)
	// dosbase ($802000)
	// 
	virtual int read16(int address,int flags) {
		//		int base = 0x801000;
		int offset = address | -4096;
		if (flags&QBIT) {
			//		log_bus(0, 1, physicalAddress, 0);
		}
		if (flags & DBIT) {
			return 0x4e75;
		}
//		int func = -(offset/ 6) - 63;
		int func = -(offset / 6) - 33;
		switch (func) {
		case DOSOPEN:
			dos->open();
			break;
		case ALLOCMEM:
		case ALLOCATE:
			machineState = "ALLOC";
			exec->allocMem();
			break;
		case DEALLOCATE:
			break;
		case WAIT:
			break;
		case OLDOPENLIBRARY: 
		case OPENLIBRARY:
			exec->openLibrary();
//			machineError = address;
			break;
		case SETEXCEPT:
		case SETSIGNAL:
		case SETTASKPRI:
			break;
		case FINDTASK:
			exec->fakeTask();
			break;
		case GETMSG:
		case PUTMSG:
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
		default:
			machineState = std::to_string(offset) + "(execbase) un supported";
			machineError=address;
			break;
		}
		// once system implementation is done return an RTS
		return 0x4e75;
	}
	virtual int read32(int address) {		
		// trap $114(execbase) for apps looking for workbench pointers
		if (address == (0x1000 + 0x114) ) {
			return 0x801000;
		}
		return address;
//		writeData32(address);
//		machineError=address;
//		return shorts[address >> 1];
	}
};
