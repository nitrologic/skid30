#pragma once
#include "monitor.h"
#include "filedecoder.h"
// EA DPAINT IFF ILBM FILE FORMAT
// http://www.etwright.org/lwsdk/docs/filefmts/ilbm.html

const int FORM=0x464f524d;
const int ILBM=0x494c424d;
const int BMHD=0x424d4844;
const int BODY=0x424f4459;
const int CMAP=0x434d4150;
const int CRNG=0x43524e47;
const int CAMG=0x43414d47;
const int DPPS=0x44505053;

#define CAMG_HAM 0x800   /* hold and modify */
#define CAMG_EHB 0x80    /* extra halfbrite */

void loadIFF(std::string path){
	writeString("Reading IFF from ");
	writeString(path);
	writeEOL();
	filedecoder fd(path);

	int form = fd.readBigInt();
	if (form != FORM){
		std::cout << "expecting FORM at start of IFF ILBM" << std::endl;
		return;
	}
	int fsize=fd.readBigInt();
	int ilbm=fd.readBigInt();
	if(ilbm!=ILBM){
		writeCC4Big(ilbm);
		std::cout << "expecting ILBM at start of IFF ILBM" << std::endl;
		return;
	}
	int w = 0;
	int h = 0;
	int planes = 0;
	int mask = 0;
	int comp = 0;
	int xorg = 0;
	int yorg = 0;
	int trans = 0;
	int xasp = 0;
	int yasp = 0;
	int pagew = 0;
	int pageh = 0;
	fsize -= 4;

	while (fsize > 0) {
		int token= fd.readBigInt();
		int tsize= fd.readBigInt();

		switch (token) {

		case BODY:{

			int depth = planes;
			int c = 0; 
			int pos = 0; 
			int wid = ((w + 15) >> 4) << 1; 
			int bm = 0;
			if (comp == 0)
			{
				for (int y = 0; y < h; y++)
				{
					int val = 0x10000; 
					if (depth <= 8) val = 1;
					for (int d = 0; d < depth; d++)
					{
						for (int x = 0; x < wid; x++)
						{
							int b = fd.readByte();
							for (int j = 128; j > 0; j = j >> 1)
							{
//								if ((j & b) == j) can.pix[pos] |= val;
								pos++;
							}
						}
						pos -= wid * 8;
						val = val << 1;
						if (val == 0x1000000) { val = 0x100; }
						if (val == 0x10000) { val = 1; }
					}
//					pos += can.modulo;
				}
				break;
			}

			for (int y = 0; y < h; y++)
			{
				int val = 0x10000; 
				if (depth <= 8) val = 1;
				for (int d = 0; d < depth; d++)
				{
					for (int x = 0; x < wid;)
					{
						int b = fd.readByte();				//*iff++
						if (b == -128) continue;
						if (b < 0)
						{
							b = 1 - b;
							c = fd.readByte();
							for (int i = 0; i < b; i++) {
								for (int j = 128; j > 0; j = j >> 1) {
									if ((j & c) == j) {
//										can.pix[pos] |= val; 
										pos++;
									}
								}
							}
							x += b;
						}
						else
						{
							b = 1 + b;
							for (int i = 0; i < b; i++){
								c = fd.readByte();
								for (int j = 128; j > 0; j = j >> 1) { 
//									if ((j & c) == j) can.pix[pos] |= val; 
									pos++; 
								}
							}
							x += b;
						}
					}
					pos -= wid * 8;
					val = val << 1;
					if (val == 0x1000000) { val = 0x100; }
					if (val == 0x10000) { val = 1; }
				}
//				pos += can.modulo;
			}

//			fd.skip(tsize);
		}
			break;
		case DPPS:
			fd.skip(tsize);
			break;
		case CRNG: {
			int pad = fd.readBigShort();
			int rate = fd.readBigShort();
			int flags = fd.readBigShort();
			int low = fd.readByte();
			int high = fd.readByte();
			fd.skip(tsize-8);
		}
			break;
		case CMAP: {
			int n = 1 << planes;
			for (int i = 0; i < n; i++) {
				int r = fd.readByte();
				int g = fd.readByte();
				int b = fd.readByte();
			}
			fd.skip(tsize-3*n);
		}
			break;
		case CAMG: {
			int camg = fd.readBigInt();
			fd.skip(tsize-4);
		}
			 break;
		case BMHD: {
//			fd.skip(16);
			w = fd.readBigShort();
			h = fd.readBigShort();

			xorg = fd.readBigShort();
			yorg = fd.readBigShort();
				
			planes = fd.readByte();
			mask = fd.readByte();
			
			comp = fd.readByte();
			fd.skip(1);

			trans = fd.readBigShort();
			xasp = fd.readByte();
			yasp = fd.readByte();

			pagew = fd.readBigShort();
			pageh = fd.readBigShort();

			//can = new canvas(); can.init(w, h);

			fd.skip(tsize - 20);
		}
			break;
		default:
//			writeString("FORM ILBM TOKEN ");
			writeCC4Big(token);
			writeEOL();
			fd.skip(tsize);
		}
		fsize -= (tsize + 8);
	}

	for (int i = 0; i < 64; i++) {
		int x = fd.readByte();
		std::cout << x << " " << (char)x << std::endl;
	}

	u16 h0 = fd.readBigShort();
	u16 h1 = fd.readBigShort();
	bool magic = (h0 == 0) && (h1 == 1011);

}
