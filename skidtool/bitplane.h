#pragma once

#include <sstream>
#include "monitor.h"

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


