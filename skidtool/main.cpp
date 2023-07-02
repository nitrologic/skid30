#include <iostream>
#include <sstream>

typedef uint16_t u16;
typedef uint32_t u32;

// ctrl shift period

struct filedecoder {

    FILE *f;

	filedecoder(std::string s) {
		f = fopen(s.c_str(), "rb");
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

	bool eof() {
		return feof(f);
	}

};

const std::string bitplane16(u16 s){
	std::stringstream ss;
	for (int i = 0; i < 16; i++) {
		char c = (s & 1) ? '#' : ' ';
		ss << c;
		s >>= 1;
	}
	return ss.str();
}

const std::string bitplane32big(u32 s) {
	std::stringstream ss;
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 8; j++) {
			int bit = (1 << (7-j)) << (i * 4);
			char c = (s & bit) ? '#' : ' ';
			ss << c;
		}
	}
	return ss.str();
}
const std::string bitplane32(u32 s) {
	std::stringstream ss;
	for (int i = 0; i < 32; i++) {
		char c = (s & 1) ? '#' : ' ';
		ss << c;
		s >>= 1;
	}
	return ss.str();
}

int main(){
	std::cout << "skidtool 0.0" << std::endl;

//	std::string src("C:\\nitrologic\\skid30\\archive\\agacars\\mini.aga");
	std::string src("C:\\nitrologic\\skid30\\vehicles\\mini.aga");

	filedecoder fd(src);

	u16 s0=fd.readShort();
	u16 s1 = fd.readShort();
	u16 s2 = fd.readShort();
	u16 s3 = 0;// fd.readShort();

	std::cout << s0 << "," << s1 << "," << s2 << "," << s3 << std::endl;

	for (int i = 0; i < 1280; i++) {

		u32 i0 = fd.readInt();
		std::cout << "[" << i << "] " <<  bitplane32big(i0) << std::endl;


//		u16 s0 = fd.readShort();
//		std::cout << "[" << i << "] " <<  bitplane16(s0) << std::endl;

		if (fd.eof()) 
			break;

	}

	return 0;

}
