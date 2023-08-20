#pragma once

#include <vector>
#include <cstdint>
#include <iostream>
#include <iomanip>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

typedef std::vector<u16> Chunk;

void writeByte(int b);
void writeShort(int b);
void writeNamedInt(const char* name, int b);
void writeAddress(int b);
void writeData32(int b);
void writeData16(int b);
void writeData8(int b);
void writeHome();
void writeClear();
void writeEOL();
void writeSpace();
void writeChar(int c);
void writeString(std::string s);
void writeNamedString(std::string n, std::string s);
void writeIndex(int i);
void writeCC4Big(int tag);
