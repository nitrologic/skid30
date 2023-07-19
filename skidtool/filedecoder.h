#pragma once

#include <cstdio>
#include <string>
#include "monitor.h"

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

	void readChars(int n, void *buffer) {
		fread(buffer, n, 1, f);
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
