#include <iostream>
#include <sstream>

#include "libpng/png.h"
#include <zlib.h>



typedef uint16_t u16;
typedef uint32_t u32;


//int palette[] = { 0,0xff000000,0xffffffff,0xff0000ff };
int palette[] = { 0,0xffffffff,0xff000000,0xff0000ff };

// cars are 64x32
// 800 frames is 32x25
// png is 2048 x 800

struct Image {

	int width;
	int height;
	int bitdepth;
	int channels;
	int span;	//int words per line
	size_t size;

	int* pixels = nullptr;

	Image(int w, int h, int d, int c) {
		setSize(w, h, d, c);
	}

	void plot(int x, int y, int c) {
		pixels[y * span + x] = c;
	}

	void hlin32x2(int celx,int cely,u32 s0, u32 s1) {
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 8; j++) {
				int bit = (1 << (7 - j)) << (i * 8);
				int bits = ((s0 & bit) ? 2 : 0) + ((s1 & bit) ? 1 : 0);
				if(bits) plot(celx, cely, palette[bits]);
				celx++;
			}
		}
	}

	void setSize(int w, int h, int d, int c) {
		if (pixels) {
			delete[]pixels;
		}
		width = w;
		height = h;
		bitdepth = d;
		channels = c;
		int bits = bitdepth * channels;
		if (bits == 24) bits = 32; // RGB -> xRGB
		span = ((width * bits) + 31) / 32;
		size = span * height;
		pixels = new int[size];
		if (pixels == 0) {
			std::cout << "pixel allocation failure" << std::endl;
			// throw here
		}
		zero();
	}

	void zero() {
		memset(pixels, 0x00, 4 * span * height);
	}

	bool writePNG(png_structp& png, png_charp license = NULL) {

		int color_type;
		switch (channels) {
		case 4:
			color_type = PNG_COLOR_TYPE_RGB_ALPHA;
			break;
		case 3:
			color_type = PNG_COLOR_TYPE_RGB;
			break;
		case 2:
			color_type = PNG_COLOR_TYPE_GRAY_ALPHA;
			break;
		case 1:
			color_type = PNG_COLOR_TYPE_GRAY;
			break;
		default:
			return false;
		}

		int bits = channels * bitdepth;
		int bit_depth = bitdepth;

		int interlace_method = PNG_INTERLACE_NONE;
		int compression_method = PNG_COMPRESSION_TYPE_DEFAULT;
		int filter_method = PNG_FILTER_TYPE_DEFAULT;

		png_infop info = png_create_info_struct(png);
		png_set_IHDR(png, info, width, height, bit_depth, color_type, interlace_method, compression_method, filter_method);

		if (license) {
			//			int numText = 1;
			//			png_charp License = "License";
			//			png_text text = { -1,License,license,strlen(license),0 };
			//			png_set_text(png, info, &text, 1);
						/*
					{
						int  compression;       // compression value: -1: tEXt, none 0: zTXt, deflate 1: iTXt, none 2: iTXt, deflate
						png_charp key;          // keyword, 1-79 character description of "text"
						png_charp text;         // comment, may be an empty string (ie "") or a NULL pointer
						png_size_t text_length; // length of the text string
						png_size_t itxt_length; // length of the itxt string
						png_charp lang;         // language code, 0-79 characters or a NULL pointer
						png_charp lang_key;     // keyword translated UTF-8 string, 0 or more chars or a NULL pointer
					} png_text;
					*/
		}
		png_write_info(png, info);	//png_set_bgr(png);

		png_bytep* rows = new png_bytep[height];
		for (int y = 0; y < height; y++) {
			rows[y] = (png_bytep)(pixels + y * span);
		}
		png_write_rows(png, rows, height);
		delete[]rows;

		png_write_end(png, info);
		png_destroy_write_struct(&png, &info);

		return true;
	}

	bool savePNG(const char* path, const char* license = NULL) {
		FILE* fp = fopen(path, "wb");
		if (!fp) return false;
		png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
		png_init_io(png, fp);
		bool result = writePNG(png, (char*)license);
		int closed = fclose(fp);
		if (closed == EOF) {
			result = false;
		}
		return result;
	}

};

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
		return feof(f)!=0;
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


int decodeCar(std::string src,std::string dest){
	filedecoder fd(src);
	Image cels(2048, 800, 8, 4);

	for (int i = 0; i < 800; i++) {

		// .aga frames are 512 bytes = 64 x 8 

		for (int j = 0; j < 32; j++) {
			//u16 s0 = fd.readShort();
			//std::cout << "[" << index << "] " <<  bitplane16(s0) << std::endl;
			
			u32 i0 = fd.readInt();
			u32 i1 = fd.readInt();
			u32 i2 = fd.readInt();
			u32 i3 = fd.readInt();

//			std::cout << "[" << index << "] " <<  bitplane32(i0) << bitplane32(i1)  << std::endl;
			std::cout << "[" << i << "] " << dualbitplane32(i0,i2) << dualbitplane32(i1,i3) << std::endl;

			int celx = i & 31;
			int cely = i >> 5;

			cels.hlin32x2(celx*64,cely*32+j, i0, i2);
			cels.hlin32x2(celx * 64+32, cely * 32 + j, i1, i3);
		}

		std::cout << "-------------------------------------------------" << std::endl;


		if (fd.eof()) 
			break;

	}

	cels.savePNG(dest.c_str());

	return 0;

}

// todo - support compressed aga for caravan and f1 cars







#define musashi

#ifdef musashi

extern "C" {
#include "musashi/m68k.h"
#include "musashi/m68kcpu.h"
#include "musashi/m68kops.h"
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

void disassemble_program()
{
	unsigned int pc;
	unsigned int instr_size;
	char buff[100];
	char buff2[100];

	pc = cpu_read_long_dasm(4);

	while (pc <= 0x16e)
	{
		instr_size = m68k_disassemble(buff, pc, M68K_CPU_TYPE_68000);
		make_hex(buff2, pc, instr_size);
		printf("%03x: %-20s: %s\n", pc, buff2, buff);
		pc += instr_size;
	}
	fflush(stdout);
}


int main() {
	std::cout << "skidtool 0.1" << std::endl;

//	disassemble_program();

	m68k_init();
	m68k_set_cpu_type(M68K_CPU_TYPE_68000);
	m68k_pulse_reset();

	m68k_execute(100000);

//	input_device_reset();
//	output_device_reset();
//	nmi_device_reset();
//	const char* tag = "";

	return 0;
};

#else

int main() {
	std::cout << "skidtool 0.1" << std::endl;
	//	decodeCar("C:\\nitrologic\\skid30\\vehicles\\mini.aga", "C:\\nitrologic\\skid30\\vehicles\\mini.png");
	//	decodeCar("C:\\nitrologic\\skid30\\vehicles\\vw.aga", "C:\\nitrologic\\skid30\\vehicles\\vw.png");
	//	decodeCar("C:\\nitrologic\\skid30\\vehicles\\truck.aga", "C:\\nitrologic\\skid30\\vehicles\\truck.png");
	//	decodeCar("C:\\nitrologic\\skid30\\vehicles\\porsche.aga", "C:\\nitrologic\\skid30\\vehicles\\porsche.png");

	//	decodeCar("C:\\nitrologic\\skid30\\vehicles\\camaro.aga", "C:\\nitrologic\\skid30\\vehicles\\camaro.png");
	//	decodeCar("C:\\nitrologic\\skid30\\vehicles\\cow.aga", "C:\\nitrologic\\skid30\\vehicles\\cow.png");

	//	decodeCar("C:\\nitrologic\\skid30\\vehicles\\midget.aga", "C:\\nitrologic\\skid30\\vehicles\\midget.png");
	//	decodeCar("C:\\nitrologic\\skid30\\vehicles\\police.aga", "C:\\nitrologic\\skid30\\vehicles\\police.png");

	return 0;
}

#endif