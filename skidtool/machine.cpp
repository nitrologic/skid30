#include "machine.h"
#include "m68k.h"

//#define LOG_VERBOSE

int machineError;

std::string machineState = "";

std::vector<std::pair<int,logline>> machineLog;

void flattenString(std::string &s){
	std::replace(s.begin(), s.end(), '\n', '_');
	std::replace(s.begin(), s.end(), '\r', '_');
	std::replace(s.begin(), s.end(), '\0', '|');
}

void systemLog(const char* tag, std::string s) {
	std::replace(s.begin(), s.end(), '\n', '_');
	std::replace(s.begin(), s.end(), '\0', '|');
	std::stringstream ss;
	void* context = 0;
	int clock = readClock();
//	ss << addressString(clock) << " [" << tag << "] " << s;
	ss << "[" << tag << "] " << s;
	std::string line = ss.str();
	machineLog.push_back({ clock, line });
// if logging to console
#ifdef LOG_VERBOSE
	std::cout << line;// << std::endl;
#endif
}

std::string addressString(int b) {
	std::stringstream ss;
	ss << std::setfill('0') << std::setw(6) << std::right << std::hex << (b & 0xffffff) << std::dec;
	return ss.str();
}

std::string hexValue32(int v32) {
	std::stringstream ss;
	ss << std::setfill('0') << std::setw(8) << std::right << std::hex << v32 << std::dec;
	return ss.str();
}
std::string hexValue16(int v16) {
	std::stringstream ss;
	ss << std::setfill('0') << std::setw(4) << std::right << std::hex << (v16&0xffff) << std::dec;
	return ss.str();
}
std::string hexValue8(int v8) {
	std::stringstream ss;
	ss << std::setfill('0') << std::setw(2) << std::right << std::hex << (v8&0xff) << std::dec;
	return ss.str();
}

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
	std::cout << std::setfill('0') << std::setw(6) << std::right << std::hex << (b & 0xffffff) << std::dec;
}
void writeData32(int b) {
	std::cout << std::setfill('0') << std::setw(8) << std::right << std::hex << b << std::dec;
}
void writeData16(int b) {
	std::cout << std::setfill('0') << std::setw(4) << std::right << std::hex << (b & 0xffff) << std::dec;
}
void writeData8(int b) {
	std::cout << std::setfill('0') << std::setw(2) << std::right << std::hex << (b & 0xff) << std::dec;
}

void writeHome() {
//	std::cout << "\033[0;0f" << std::flush;
//	std::cout << "\033[0;0H" << std::flush;
	std::cout << "\033[H" << std::flush;
//	std::cout << std::flush;
}

void writeClear() {
	std::cout << "\033[2J" << "\033[H" << std::flush;
//	std::cout << "<clr>" << std::flush;
}

void writeEOL() {
	// clear to end of line
	std::cout << "\033[K" << std::endl;
//	std::cout << "<eol>" << std::endl;
//	std::cout << std::endl;
}
void writeSpace() {
	std::cout << " ";
}
void writeChar(int c) {
//	std::cout << "%" << c;
	std::cout << (char)c;
}
void writeString(std::string s) {
	std::cout << s;
}
void writeNamedString(std::string n, std::string s) {
	std::cout << n << " " << s;
}
void writeIndex(int i) {
	std::cout << i;
}
void writeCC4Big(int tag) {
	writeData32(tag);
	std::cout << " ";
	for (int i = 0; i < 4; i++) {
		int b = (tag >> ((3 - i) * 8)) & 0xff;
		if (b < 32 || b>127)
			b = '#';
		std::cout << (char)(b);
	}
}

Chunk16 loadPhysicalChunk(std::string path,int physical) {

	std::replace(path.begin(),path.end(),':','\\');

	writeString("Reading Hunk from ");
	writeString(path);
	writeEOL();

	filedecoder fd(path);
	if (fd.f == 0) {
		writeString("file failure");
		writeEOL();
		return Chunk16();
	}
	// page 256 hunk__header

	u16 h0 = fd.readBigShort();
	u16 h1 = fd.readBigShort();
	bool magic = (h0 == 0) && (h1 == 1011);

	if (!magic) {
		writeString("loadChunk fail magic");
		writeEOL();
		return Chunk16();
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
	Chunk16 chunk(totalWords);
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
			std::cout << "type " << std::dec << type << " not supported " << std::endl;
//			std::cout << "type " << std::hex << type << " not supported " << std::endl;
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

//	writeString("hunks parsed");
//	writeEOL();
	return chunk;
}

