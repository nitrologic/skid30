#pragma once

#include "monitor.h"
#include "filedecoder.h"

// physical address and mask == physical address
// big endian byte order
// flag 1 - logs reads
// flag 2 - logs writes
// flag 4 - star

struct memory32 {
	u32 physical;
	u32 mask;
	u32 flags;

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
			std::cout << "unexpected eof fail for rom16 file" << std::endl;
		}
	}
	virtual int read16(int address) {
		return shorts[address>>1];
	}
};

struct ram16 : memory32 {
	std::vector<u16> shorts;
	ram16(u32 p,u32 m, int wordCount) : memory32(p,m), shorts(wordCount) {
		flags=2;
	}
	virtual void write16(int address,int value) {
		if(address<0||(address>>1)>=shorts.size()){
			return;
		}
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
		flags=3;
	}
	virtual void write16(int address, int value) {
		shorts[address >> 1] = value;
	}
	virtual int read16(int address) {
		return shorts[address >> 1];
	}
};
