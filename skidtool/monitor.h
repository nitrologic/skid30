#pragma once

#include <cstdint>
#include <iostream>
#include <iomanip>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

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

