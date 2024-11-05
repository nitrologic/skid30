#include "skidnative.h"
#include "exec.h"
#include "machine.h"

#include <map>
#include <set>
#include <filesystem>
#ifdef _WIN32
#include <direct.h>
#endif
#include <sys/stat.h>

#define LOG_DOS_OPEN_FAIL

#define ZERO_PAD_FREAD

enum {
	MEMF_ANY = 0,
	MEMF_PUBLIC = 1,
	MEMF_CHIP = 2,
	MEMF_FAST = 4,
	MEMF_LOCAL = 256,
	MEMF_DMA = 512,
	MEMF_KICK = 1024,
	MEMF_CLEAR = 0x10000,
	MEMF_LARGEST = 0x20000,
	MEMF_REVERSE = 0x40000,
	MEMF_TOTAL = 0x80000
};

struct acidmicro {

	virtual int setSignalBits(int newbits, int setbits) = 0;

	virtual int allocate(int size, int bits) = 0;
	virtual void push(int physicalAddress) = 0;

	virtual std::string fetchString(int a1) = 0;
	virtual std::string fetchPath(int a1) = 0;
	virtual std::vector<u8> fetchBytes(int a1, int d0) = 0;
	virtual std::vector<u16> fetchShorts(int a1, int d0) = 0;

	virtual int readRegister(int reg) = 0;
	virtual void writeRegister(int reg, int value) = 0;

	virtual int read8(int a32) = 0;
	virtual int read16(int a32) = 0;
	virtual int read32(int a32) = 0;

	virtual void write8(int physicalAddress, int value) = 0;
	virtual void write16(int physicalAddress, int value) = 0;
	virtual void write32(int physicalAddress, int value) = 0;

	virtual void writeChunk(int physicalAddress, const Chunk &chunk) = 0;

	void writeEndianMem(int physicalAddress, void* src, size_t size) {
		int n = (size + 3) / 4;
		uint32_t* l = (uint32_t*)src;
		for (int i = 0; i < n; i++) {
			write32(physicalAddress + i * 4, l[i]);
		}
	}

};

std::string decodeFormattedString(acidmicro *cpu0, std::string fmt, int args) {
	int a1 = args;
	//		systemLog("fmt", fmt);
	std::stringstream ss;

	for (auto i = 0; i < fmt.length(); i++) {
		char c = fmt[i];
		if (c == '%') {
			int len = 2;
			int word = 0;
			int decode = 1;
			int dec = 0;
			char pad = 0;
			std::string s;
			while (decode) {
				char d = fmt[++i];
				if (d >= '0' && d <= '9') {
					if (d == 0) pad = '0';
					dec = dec * 10 + (int)(d - '0');
					continue;
				}
				switch (d) {	//b=bstr d=decimal u=unsigned x=hex s=str
				case '%':
					ss << '%';
					decode = 0;
					break;
				case 's': {
					int p = cpu0->read32(a1);
					s = cpu0->fetchString(p);
					a1 += 4;
					ss << s;
					decode = 0;
				}
						break;
				case 'l':
					len = 4;
					break;
				case 'd':
					if (len == 2) {
						word = cpu0->read16(a1);
						a1 += 2;
					}
					else {
						word = cpu0->read32(a1);
						a1 += 4;
					}
					{
						std::string w = std::to_string(word);
						size_t pad = (dec > w.length()) ? dec - w.length() : 0;
						ss << std::string(pad, ' ');
						ss << (int)word;
					}
					decode = 0;
					break;
				case 'c':
					if (len == 2) {
						d = cpu0->read16(a1);
						a1 += 2;
					}
					else {
						d = cpu0->read32(a1);
						a1 += 4;
					}
					ss << (char)d;
					decode = 0;
					break;
				default:
					break;
				}
			}
		}
		else {
			ss << (char)c;
		}

	}
	ss << '\0';
	return ss.str();
}

#include <string.h>

void pokeString(std::string s, char* dest, int maxlen) {
	int n = s.size();
	if (n > maxlen) n = maxlen;
	memcpy(dest, s.data(), n);
	dest[n] = 0;
}
void ekopString(std::string s, char* dest, int maxlen) {
	int n = s.size();
	if (n > maxlen) n = maxlen;
	int i = 0;
	while (i < n) {
		dest[(i & -4) | (3 - (i & 3))] = s[i];
		i++;
	}
	dest[(i & -4) | (3 - (i & 3))] = 0;
}

std::string rawString(std::vector<u8> raw, bool addhex) {
	std::stringstream ss;
	std::stringstream ss2;
	int n = (int)raw.size();
	ss << std::setfill('0') << std::right << std::hex;
	for (int i = 0; i < n; i++) {
		ss << std::setw(2) << (int)raw[i] << " ";
		char c = (char)raw[i];
		if (c > 7) {
			ss2 << (char)raw[i];
		}
	}
	if (addhex) {
		ss2 << std::endl << ss.str();// std::endl + ss2.str();
	}
	return ss2.str();

}


const int INPUT_STREAM = -4;
const int OUTPUT_STREAM = -8;
const int FILE_STREAM = -24;


struct DateStamp {
	int days,mins,ticks;
};

/* Returned by Examine() and ExNext(), must be on a 4 byte boundary */

struct FileInfoBlock {
	int	  fib_DiskKey;
	int	  fib_DirEntryType;  /* Type of Directory. If < 0, then a plain file. If > 0 a directory */
	char	  fib_FileName[108]; /* Null terminated. Max 30 chars used for now */
	int	  fib_Protection;    /* bit mask of protection, rwxd are 3-0.	   */
	int	  fib_EntryType;
	int	  fib_Size;	     /* Number of bytes in file */
	int	  fib_NumBlocks;     /* Number of blocks in file */
	struct DateStamp fib_Date;/* Date file last changed */
	char	  fib_Comment[80];  /* Null terminated comment associated with file */

	/* Note: the following fields are not supported by all filesystems.	*/
	/* They should be initialized to 0 sending an ACTION_EXAMINE packet.	*/
	/* When Examine() is called, these are set to 0 for you.		*/
	/* AllocDosObject() also initializes them to 0.			*/
	uint16_t fib_OwnerUID;		/* owner's UID */
	uint16_t fib_OwnerGID;		/* owner's GID */

	char	  fib_Reserved[32];
}; /* FileInfoBlock */

/* FIBB are bit definitions, FIBF are field definitions */
#define FIBB_SCRIPT    6	/* program is a script (execute) file */
#define FIBB_PURE      5	/* program is reentrant and rexecutable */
#define FIBB_ARCHIVE   4	/* cleared whenever file is changed */
#define FIBB_READ      3	/* ignored by old filesystem */
#define FIBB_WRITE     2	/* ignored by old filesystem */
#define FIBB_EXECUTE   1	/* ignored by system, used by Shell */
#define FIBB_DELETE    0	/* prevent file from being deleted */

FileInfoBlock _fib = { 0 };


typedef std::vector<uint8_t> Blob;

int fileCount=0;

struct NativeFile {
	int exclusiveLock=0;
	std::set<int> bcplLocks;
	std::string filePath;
	FILE *fileHandle=NULL;
	struct stat fileStat;
	int status;
	int isTemp;
	int isDir;
	std::filesystem::directory_iterator fileIterator;

	int nextLock() {
		int t = FILE_STREAM - (fileCount++) * 4;
		return (uint32_t)t >> 2;
	}

	std::string fileName() {
		std::filesystem::path p(filePath);
		return p.filename().string();
	}

	int addLock(bool exclusive) {
		int h = nextLock();
		if (exclusive) {
			if (bcplLocks.size() || exclusiveLock || fileHandle) {
				std::cout << "NativeFile::addLock failure " << std::endl;
				return 0;
			}
			exclusiveLock = h;
			return h;
		}
		bcplLocks.insert(h);
		return h;
	}

	void unlock(int h) {
		if (exclusiveLock == h) {
			exclusiveLock = 0;
		}else{
			bcplLocks.erase(h);
		}
	}

	// preset lock constructor for stdin

	NativeFile(int lock, std::string path) : NativeFile(path) {
		bcplLocks.insert(lock);
	}

	NativeFile(std::string path) {
		filePath = path;
		isTemp = 0;
		isDir = 0;
		if (path == "stdin") {
			fileHandle = stdin;
			fileStat = { 0 };
			status = 0;
			return;
		}
		if (path == "stdout") {
			fileHandle = stdout;
			fileStat = { 0 };
			status = 0;
			return;
		}
		if (path == "T:") {
			fileStat = { 0 };
			status = 0;
			isTemp = 1;
			isDir = 1;
			return;
		}
		statFile();
	}

	void statFile() {
		int res = stat(filePath.c_str(), &fileStat);
		if (res == 0) {
			isDir = (fileStat.st_mode & S_IFDIR) ? 1 : 0;
		}
		status=res;
	}

	NativeFile() {
	}

	NativeFile(const NativeFile&f) {
		filePath = f.filePath;
		int res = stat(filePath.c_str(), &fileStat);
		status = res;
	}

	int nextEntry() {
		if (status == 0) {
			fileIterator = std::filesystem::directory_iterator(filePath);
			status = 1;
		}
		if (status == 2) {
			fileIterator++;
			status = 1;
		}
		if(status==1){
			if (fileIterator==std::filesystem::directory_iterator()) {
//			if (fileIterator._At_end()) {
				status = 3;
				return 0;
			}
			status = 2;
			return 1;
		}
		return 0;
	}

	int open(int mode) {

		if (fileHandle) {
			return 0;
		}

//		if (status) return 0;
		const char* m;
		switch (mode) {
		case 1005://MODE_OLDFILE
//			m = "r+b";
			m = "rb";
			break;
		case 1006://MODE_NEWFILE
			m = "wb";	// was w+b
			break;
		case 1004://MODE_READWRITE
			m = "a+b";
			break;
		}
		// TODO: interpret amiga mode to fopen _Mode
		fileHandle = fopen(filePath.c_str(), m);
#ifdef LOG_DOS_OPEN_FAIL
		if(!fileHandle){
			std::cout << "fopen failure for " << filePath << " mode " << m << std::endl;
		}
#endif
//		statFile();
		status = (fileHandle) ? 0 : -1;
		return fileHandle?1:0;
	}

	void close() {
		if (fileHandle) {
			fclose(fileHandle);
			fileHandle = 0;
		}
		else {
			//todo - lha needs help
		}
	}

//	+		fileHandle	0x0000018aa38c3f80 {_Placeholder=0x0000018aa38ba3d0 }	_iobuf *

	Blob read(int length) {
		Blob blob;
		uint8_t c;
		for (int i = 0; i < length; i++) {
			int n = fread(&c, 1, 1, fileHandle);
			if (n == 0) 
				break;
			if (n < 0) {
				break;
			}
			blob.push_back(c);
		}
		return blob;
	}

	int write(Blob blob) {
		if (fileHandle == 0) {
			return -1;
		}
		if (exclusiveLock == 0) {
			return -2;
		}
		int size = blob.size();
		uint8_t* p = blob.data();
		int n = 0;
		while (n < size) {
			int count = fwrite(p, 1, size, fileHandle);
			if (count <= 0) break;
			n += count;
			p += count;
		}
		return n;
	}

	int seek(int offset, int mode) {
		int oldpos = ftell(fileHandle);
//		int origin = (mode == -1) ? SEEK_CUR : ((mode == 0) ? SEEK_SET : SEEK_END);
		int origin = (mode == -1) ? SEEK_SET : ((mode == 0) ? SEEK_CUR : SEEK_END);
		int res = fseek(fileHandle, offset, origin);
		if (res) {
// simon come here as this crashes lha
//			return -1;
		}
		int currentpos = ftell(fileHandle);
		return (int)currentpos;
//		return oldpos;
	}
};


typedef std::map<std::string, NativeFile> FileMap;	// never removed
typedef std::map<int, NativeFile*> FileLocks;  // sometimes collected

std::map<int, std::string> modeNames = { {1005,"OLDFILE"},{1006,"NEWFILE"},{1004,"READWRITE"} };
std::map<int, std::string> lockNames = { {-1,"EXCLUSIVE"},{-2,"SHARED"} };
std::map<int, std::string> seekNames = { {-1,"BEGINNING"},{0,"CURRENT"},{1,"END"} };

class aciddos : public IDos {
	FileMap fileMap;
	FileLocks fileLocks;
	acidlog doslog;

	enum modes {
		MODE_OLDFILE = 1005,   // Open existing file read/write positioned at beginning of file. 
		MODE_NEWFILE = 1006,   // Open freshly created file (delete old file) read/write, exclusive lock. 
		MODE_READWRITE = 1004   // Open old file w/shared lock, creates file if doesn't exist. 
	};

	enum locks {
		SHARED_LOCK=-2,	    // File is readable by others
		EXCLUSIVE_LOCK=-1   // No other access allowed
	};

	void emit() {
		std::string s = doslog.str();
		systemLog("dos", s);
		doslog.clr();
	}

public:
	
	acidmicro* cpu0;
	int emptyArgs;

	aciddos(acidmicro* micro) {
		cpu0 = micro;
		doslog.clr();

		fileMap["stdin"] = NativeFile(INPUT_STREAM, "stdin");
		fileMap["stdout"] = NativeFile(OUTPUT_STREAM, "stdout");

		fileLocks[INPUT_STREAM] = &fileMap["stdin"];
		fileLocks[OUTPUT_STREAM] = &fileMap["stdout"];

		emptyArgs = micro->allocate(12 + 24,1);
	}

    void vprintf(){
		int d1 = cpu0->readRegister(1); // format
		int d2 = cpu0->readRegister(2); // array
		std::string format = cpu0->fetchString(d1);
//		std::cout << "VPRINTF " << format << std::endl;
		std::string s = decodeFormattedString(cpu0, format, d2);
		std::cout << "{VF}" << s << std::endl;
	}

/*
* struct CSource {
	UBYTE	*CS_Buffer;
	LONG	CS_Length;
	LONG	CS_CurChr;
};
struct RDArgs {
	struct	CSource RDA_Source;	// Select input source 
	LONG	RDA_DAList;		// PRIVATE. 
	UBYTE* RDA_Buffer;		// Optional string parsing space. 
	LONG	RDA_BufSiz;		// Size of RDA_Buffer (0..n) 
	UBYTE* RDA_ExtHelp;		// Optional extended help 
	LONG	RDA_Flags;		// Flags for any required control 
};
#define RDAB_STDIN	0	// Use "STDIN" rather than "COMMAND LINE" 
#define RDAF_STDIN	1
#define RDAB_NOALLOC	1	// If set, do not allocate extra string space.
#define RDAF_NOALLOC	2
#define RDAB_NOPROMPT	2	// Disable reprompting for string input. 
#define RDAF_NOPROMPT	4
*/
//http://amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_3._guide/node01A1.html
	void readargs() {
		int d1 = cpu0->readRegister(1);	// template
		int d2 = cpu0->readRegister(2);	// array
		int d3 = cpu0->readRegister(3); // rdargs
		std::string tremplate = cpu0->fetchString(d1);
		cpu0->writeRegister(0,emptyArgs); // default failure
#ifdef LOG_VERBOSE
		doslog << "readargs " << tremplate << "," << d3 << " => " << 0;
#endif
		emit();
		return;
	}

/*

***********************************************************************
************************ PATTERN MATCHING ******************************
************************************************************************

* structure expected by MatchFirst, MatchNext.
* Allocate this structure and initialize it as follows:
*
* Set ap_BreakBits to the signal bits (CDEF) that you want to take a
* break on, or NULL, if you don't want to convenience the user.
*
* If you want to have the FULL PATH NAME of the files you found,
* allocate a buffer at the END of this structure, and put the size of
* it into ap_Strlen.  If you don't want the full path name, make sure
* you set ap_Strlen to zero.  In this case, the name of the file, and stats
* are available in the ap_Info, as per usual.
*
* Then call MatchFirst() and then afterwards, MatchNext() with this structure.
* You should check the return value each time (see below) and take the
* appropriate action, ultimately calling MatchEnd() when there are
* no more files and you are done.  You can tell when you are done by
* checking for the normal AmigaDOS return code ERROR_NO_MORE_ENTRIES.
*
*/
#ifdef EMBED_AMIGA_INCLUDES
	struct AnchorPath {
		struct AChain* ap_Base;	/* pointer to first anchor */
#define	ap_First ap_Base
		struct AChain* ap_Last;	/* pointer to last anchor */
#define ap_Current ap_Last
		LONG	ap_BreakBits;	/* Bits we want to break on */
		LONG	ap_FoundBreak;	/* Bits we broke on. Also returns ERROR_BREAK */
		BYTE	ap_Flags;	/* New use for extra word. */
		BYTE	ap_Reserved;
		WORD	ap_Strlen;	/* This is what ap_Length used to be */
#define	ap_Length ap_Flags	/* Old compatability for LONGWORD ap_Length */
		struct	FileInfoBlock ap_Info;
		UBYTE	ap_Buf[1];	/* Buffer for path name, allocated by user */
		/* FIX! */
	};
#endif
//http://amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_3._guide/node018D.html
	void matchfirst() {
		int d1 = cpu0->readRegister(1);	// template
		int d2 = cpu0->readRegister(2);	// path
		std::string path = cpu0->fetchString(d2);
		cpu0->writeRegister(0, 0);
	}
	void matchend() {

	}
	void freeargs() {
	}

// http://amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_3._guide/node0196.html
	void getvar() {
		int d1 = cpu0->readRegister(1);
		int d2 = cpu0->readRegister(2);
		int d3 = cpu0->readRegister(3);
		int d4 = cpu0->readRegister(4);
		std::string name = cpu0->fetchString(d1);
		cpu0->writeRegister(0,-1); // not defined
		doslog << "getvar " << name << " => " << -1;
		emit();
		return;
	}

	void open() {
		int d1 = cpu0->readRegister(1);//name
		int d2 = cpu0->readRegister(2);//mode
		std::string s = cpu0->fetchPath(d1);
		if (fileMap.count(s)==0) {
			fileMap[s] = NativeFile(s);
#ifdef LOG_FILE_OPEN
			std::cout << " NativeFile::open path : " << s << std::endl;
#endif
		}
		NativeFile* f = &fileMap[s];
		bool exclusive = (d2 == MODE_NEWFILE);
		int lock = f->addLock(exclusive);
		int success = 0;
		if (lock) {
			success = f->open(d2);
			if (success) {
				fileLocks[lock] = f;
			}
			else 
			{
//				std::cout << "::open lock failure " << s << std::endl;
				f->unlock(lock);
			}
		}else{
			std::cout << "::open no lock failure " << s << std::endl;
		}
		int result = success ? lock : 0;
		cpu0->writeRegister(0, result);
//		doslog << "open " << s << " " << std::dec << d2 << " => " << std::hex <<result;
		doslog << "open " << s << " " << modeNames[d2] << " => " << result;
		emit();
	}

	void close(){
		int d1 = cpu0->readRegister(1); //file
		NativeFile* f = fileLocks[d1];
		f->unlock(d1);
		f->close();
		cpu0->writeRegister(0, 0);	//RETURN_OK
		doslog << "close " << d1 << " => 0";
		emit();
	}

	void read(){
		int d1 = cpu0->readRegister(1); //file
		int d2 = cpu0->readRegister(2); //buffer physicalAddress
		int d3 = cpu0->readRegister(3); //length
		int result = d3;
		if (d1) {
			NativeFile* f = fileLocks[d1];
			Blob blob = f->read(d3);
			int n = blob.size();
			for (int i = 0; i < n; i++) {
				cpu0->write8(d2 + i, blob[i]);
			}
#ifdef ZERO_PAD_FREAD			
			for (int i = n; i < d3; i++) {
				cpu0->write8(d2 + i, 0);
			}
#endif
#ifdef WRITE_RESULT_WRITTEN
#endif
			result = n;
		}
		cpu0->writeRegister(0, result);

		doslog << "read " << d1 << " , " << d2 << " , " << d3 << " => " << result;
		emit();
	}

// http://amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_3._guide/node01D1.html
// 
	void write(){
		int d1 = cpu0->readRegister(1);
		int d2 = cpu0->readRegister(2);
		int d3 = cpu0->readRegister(3);
// TODO: handle odd address in d2 and odd length in d3
// or consider a fetchBytes command, little endians go sleep now

// std::vector<u16> raw = cpu0->fetchShorts(d2, d3);
		std::vector<u8> raw = cpu0->fetchBytes(d2, d3);
// file,buffer,length
		switch (d1) {
		case OUTPUT_STREAM: {
			std::string s = rawString(raw,false);
			systemLog("write", s);
			std::cout << s;// << std::endl;
			break;
		}
		default: {
			NativeFile* f = fileLocks[d1];
			d3=f->write(raw);
			systemLog("write", "blob");
			break;
		}
		}
		cpu0->writeRegister(0, d3);
	}
	void input(){
		cpu0->writeRegister(0, INPUT_STREAM);
		doslog << "input";
		emit();
	}
	void output(){
		cpu0->writeRegister(0, OUTPUT_STREAM);
		doslog << "output";
		emit();
	}

	void seek(){
		int d1 = cpu0->readRegister(1);//file
		int d2 = cpu0->readRegister(2);//position	
		int d3 = cpu0->readRegister(3);//mode start,current,end  -1,0,1
		NativeFile* f = fileLocks[d1];
		int pos=f->seek(d2, d3);
		cpu0->writeRegister(0, pos);
//		doslog << "seek " << d1 << "," << d2 << "," << d3 << " => " << pos;
		doslog << "seek " << d1 << "," << d2 << "," << seekNames[d3] << " => " << pos;
		emit();
	}

	void deletefile(){
		doslog << "deletefile";emit();
	}
	void rename(){
		doslog << "rename"; emit();
	}
	//http://amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_3._guide/node0186.html

	void datestamp() {
		int d1 = cpu0->readRegister(1);//{day,min,ticks}

		int dmt[3];
		dayminticks(dmt);

		cpu0->write32(d1 + 0, dmt[0]);
		cpu0->write32(d1 + 4, dmt[1]);
		cpu0->write32(d1 + 8, dmt[2]);

		cpu0->writeRegister(0, d1);

		doslog << "datestamp " << d1;
		emit();
	}

	void delay() {
		int d1 = cpu0->readRegister(1);//{ticks}
		int ms = d1 * 20;
		sleep(ms);
		doslog << "delay " << d1; 
		emit();
	}

	void unloadseg() {
		doslog << "unloadseg "; emit();
	}

	void loadseg() {
		int d1 = cpu0->readRegister(1);
		std::string segname = cpu0->fetchPath(d1);
		int seglist = 0;
		int physical = 0x050000;
		Chunk chunk = loadPhysicalChunk(segname, physical);
		cpu0->writeChunk(physical, chunk);
		int n = chunk.size();
		seglist = (n==0)?0:physical >> 2;
		cpu0->writeRegister(0, seglist);
		doslog << "loadseg " << segname << " => " << seglist;
		emit();
	}

	void setfiledate(){
		
	}

	void currentdir() {
		int d1 = cpu0->readRegister(1);	//name
		NativeFile* f = fileLocks[d1];
		// d1=lock return d0=oldlock
		// all paths lead to root - wtf LhA???
		cpu0->writeRegister(0, 0);
		doslog << "currentDir " << d1;
		emit();
	}

#define SHARED_LOCK	     -2	    // File is readable by others 
#define ACCESS_READ	     -2	    // Synonym 
#define EXCLUSIVE_LOCK	     -1	    // No other access allowed	  
#define ACCESS_WRITE	     -1	    // Synonym 

	void lock(){
		int d1 = cpu0->readRegister(1);//name
		int d2 = cpu0->readRegister(2);//type
		std::string s = cpu0->fetchPath(d1);
		
		if (fileMap.count(s) == 0) {
			fileMap[s] = NativeFile(s);
		}
		else {
			fileMap[s].statFile();
		}
		NativeFile* file = &fileMap[s];

		int success = (file->status == 0) || (d2 == -1);
		int lock = 0;

		if (success) {
			bool exclusive = (d2 == -1);
			lock = file->addLock(exclusive);
			if (lock) {
				fileLocks[lock] = file;
				// yeh nah what?
			}
		}
		else {
			std::cout << "no scuccess lock path : " << s << std::endl;
		}


		int result = success ? lock : 0;

		cpu0->writeRegister(0, result);
//		doslog << "lock " << s << "," << d2 << " => " << result;
		doslog << "lock " << s << " " << lockNames[d2] << " => " << result;
		emit();
	}
	void unLock(){
		int d1 = cpu0->readRegister(1);//lock
		NativeFile* f = fileLocks[d1];
		f->unlock(d1);
		doslog << "unlock " << d1;
		emit();
	}
	void dupLock(){
		int d1 = cpu0->readRegister(1);//lock
		if (d1 == 0) {
			cpu0->writeRegister(0, 0);
			return;
		}
		NativeFile* f = fileLocks[d1];

		int lock = f->addLock(false);
		fileLocks[lock] = f;	// count first??

//		NativeFile(f);
		int result = (f->status == 0) ? lock : 0;
		cpu0->writeRegister(0, result);
		doslog << "duplock"; emit();
	}

	void exnext() {
		int d1 = cpu0->readRegister(1);//lock
		int d2 = cpu0->readRegister(2);//fileinfo
		NativeFile* f = fileLocks[d1];
		int success = 0;
		if (f->nextEntry()) {

			const auto& entry = *(f->fileIterator);
			std::filesystem::path p = entry.path();

			std::string s = p.filename().string();
			//			filesystem::directory_entry &entry = fileIterator;

			uint64_t size = entry.file_size();
			bool isdir = entry.is_directory();

			_fib.fib_Size = (int)size;
			_fib.fib_NumBlocks = (size + 1023) / 1024;
			_fib.fib_Protection = 0xdd00;	//rwxd
			_fib.fib_DirEntryType = isdir ? 1 : -1;
			//			pokeString(f.filePath,_fib.fib_FileName,108);
//			ekopString(f->filePath, _fib.fib_FileName, 108);
			ekopString(s, _fib.fib_FileName, 108);
			cpu0->writeEndianMem(d2, &_fib, sizeof(_fib));

			success = 1;
		}
		cpu0->writeRegister(0, success);
		doslog << "exnext " << d1 << "," << d2 << " => " << success;
		emit();
	}

	void examine() {
		int d1 = cpu0->readRegister(1);//lock
		int d2 = cpu0->readRegister(2);//fileinfo
		NativeFile* f = fileLocks[d1];

		memset(&_fib, 0, sizeof(FileInfoBlock));
		int success = 0;
		if (f->status == 0) {
			int n = f->fileStat.st_size;
			int mode = f->fileStat.st_mode & 7;
//			_fib.fib_DiskKey = ;
			_fib.fib_Size = n;
			_fib.fib_NumBlocks = 0;// (n + 1023) / 1024;
			_fib.fib_Protection = 0x01;		// 0x0f;
//			_fib.fib_DirEntryType = (f->fileStat.st_mode& _S_IFDIR ) ? 1 : -1 ;
			_fib.fib_DirEntryType = (f->fileStat.st_mode& S_IFDIR ) ? 1 : -1 ;
//			pokeString(f.filePath,_fib.fib_FileName,108);

			std::string filename = f->fileName();
			ekopString(filename, _fib.fib_FileName, 108);
			cpu0->writeEndianMem(d2, &_fib, sizeof(_fib));
			success = 1;
		}
		cpu0->writeRegister(0, success);
		doslog<<"examine("<<d1<<") <= " << success;
		emit();
	}
	void info() {
		doslog << "info"; emit();
	}

	void createdir() {
		int d1 = cpu0->readRegister(1);	//name
		std::string s = cpu0->fetchPath(d1);
#ifdef _WIN32
		int result=mkdir(s.c_str());
#else
		int result=mkdir(s.c_str(),0777);	//fuck octal
#endif
		int lock = 0;
		if (result == 0) {
			if (fileMap.count(s)==0){
				fileMap[s] = NativeFile(s);
			}
			NativeFile* f = &fileMap[s];
			f->statFile();
			lock = f->addLock(true);
			fileLocks[lock] = f;
		}
		cpu0->writeRegister(0, lock);
		doslog << "createDir " << s << " => " << lock; emit();
	}
	void printfault(){
		int d1 = cpu0->readRegister(1);//code
		int d2 = cpu0->readRegister(2);//header
		int code=d1;
		std::string header=cpu0->fetchString(d2);
		cpu0->writeRegister(0, 0);
		doslog << "printfault " << header << " - " << code; emit();
	}
	void ioerr() {
		cpu0->writeRegister(0, 0);
		doslog << "ioErr"; emit();
	}
	void createproc() {
		doslog << "createProc"; emit();

	}
	void exit() {
		doslog << "exit"; emit();
	}

	void isinteractive() {
		int d0 = 0;
		int d1 = cpu0->readRegister(1);
		// d1=lock
		switch (d1) {
		case -4:
			d0 = 1;
			break;
		}
		cpu0->writeRegister(0, d0);
		doslog << "isInteractive"; emit();
	}
};

// notes copied from public web for reference purposes only

/*

addintserver

intNum - the Paula interrupt bit number (0 through 14). Processor
			 level seven interrupts (NMI) are encoded as intNum 15.
			 The PORTS, COPER, VERTB, EXTER and NMI interrupts are
			 set up as server chains.
	interrupt - pointer to an Interrupt structure.
			 By convention, the LN_NAME of the interrupt structure must
			 point a descriptive string so that other users may
			 identify who currently has control of the interrupt.

BUGS
	The graphics library's VBLANK server, and some user code, currently
	assume that address register A0 will contain a pointer to the custom
	chips. If you add a server at a priority of 10 or greater, you must
	compensate for this by providing the expected value ($DFF000).

*/

class acidexec : public IExec {
public:
	acidmicro* cpu0;
	acidlog execlog;

	void emit() {
		std::string s = execlog.str();
		systemLog("exec", s);
		execlog.clr();
	}

	acidexec(acidmicro* micro) {
		cpu0 = micro;
		execlog.clr();
	}

	void forbid(){
		execlog << "forbid";emit();
	}
	void permit() {
		execlog << "permit";emit();
	}
	void waitMsg() {
		execlog << "waitmsg";emit();
	}

// http://amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_3._guide/node0222.html

	void openLibrary() {
		int a1 = cpu0->readRegister(9);
		std::string s = cpu0->fetchString(a1);
		int r = 0;
		if (s == "dos.library") {
			r = DOS_BASE;
		}
		else if (s=="intuition.library"){
			r = INTUITION_BASE;			
		}
		else if (s == "nonvolatile.library") {
			r = NONVOLATILE_BASE;
		}
		else if (s == "graphics.library") {
			r = GRAPHICS_BASE;
		}
		else if (s == "mathffp.library") {
			r = MATHFFP_BASE;
		}
		else if (s == "diskfont.library") {
			r = DISKFONT_BASE;
		}
		else if (s == "mathtrans.library") {
			r = MATHTRANS_BASE;
		}
		else {
			// todo: build a named map
			r = 0;
			machineError = -1;
		}
		cpu0->writeRegister(0, r);
		
		execlog << "openlibrary " << s << "," << r;
		emit();
	}

	void setSignal() {
		int d0 = cpu0->readRegister(0);//newbits
		int d1 = cpu0->readRegister(1);//mask
		int bits=cpu0->setSignalBits(d0,d1);
		cpu0->writeRegister(0, bits);
#ifdef LOG_SETSIGNAL
		execlog << "setSignal " << d0 << "," << d1 << " <= " << bits;
		execlog << " ; "<< sigbits(d0) << " , " << sigbits(d1) << " <= " << sigbits(bits);
		emit();
#endif
	}

	int nextSignal = 16;

	void allocSignal() {
		int d0 = cpu0->readRegister(0);//preferebce
		int signum=(d0>0)?d0:nextSignal++;
//		bits=1<<
		cpu0->writeRegister(0, signum);
		execlog << "allocSignal " << d0 << " <= " << signum;
		emit();
	}

	void closeLibrary() {
		execlog << "closeLibrary";emit();
	}

// http://www.amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_2._guide/node036C.html

	// emit some code to writechar the result and push it on the stack
	//
	// moveq #c0,d0
	// jsr (a2)
	// moveq #c1,d0
	// jsr (a2)
	// 
	// moveq #00,d0
	// jsr (a2)
	// 
	// rts

	void superState() {
		cpu0->writeRegister(0, 0);	//always in superstate
		execlog << "superState "; emit();

	}

//http://amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_3._guide/node0244.html

	void userState() {
		cpu0->writeRegister(0, 0);	//always in superstate
		execlog << "superState "; emit();

	}
	void doIO() {
		int a1 = cpu0->readRegister(9);//ioreq
		int r = 0;
		cpu0->writeRegister(0, r);
		execlog << "doIO " << a1 << " <= " << r; emit();
	}
	void openDevice() {
		int a0 = cpu0->readRegister(8);//name
		int d0 = cpu0->readRegister(0);//unit
		int a1 = cpu0->readRegister(9);//ioreq
		int d1 = cpu0->readRegister(1);//flags
		// TODO: use with fetchPath
		std::string devname = cpu0->fetchString(a0);
		int r = -1;
		cpu0->writeRegister(0, r);
		execlog << "openDevice " << devname << "," << d0 << "," << a1 << "," << d1 << " <= " << r; emit();
	}

	// todo - leaks scratch memory 
	//http://amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_2._guide/node036C.html

	void rawDoFmt() {
		int a0 = cpu0->readRegister(8);//fmt
		int a1 = cpu0->readRegister(9);//args
		int a2 = cpu0->readRegister(10);//putchproc
		int a3 = cpu0->readRegister(11);//putchdata

		std::string fmt = cpu0->fetchString(a0);

		std::string s = decodeFormattedString(cpu0,fmt,a1);
		int n = s.length();

		// todo: check null terminated

		int scratch = cpu0->allocate(n*4+2,0);
		for (int i = 0; i < n; i++) {
//			int moveqvald0 = 0xe000 | (255 & s[i]);
			int moveqvald0 = 0x7000 | (255 & s[i]);
			cpu0->write16(scratch + i * 4 + 0, moveqvald0);
			cpu0->write16(scratch + i * 4 + 2, 0x4e92);	//jsr(a2)
		}
		cpu0->write16(scratch + n * 4, 0x4e75);
		cpu0->push(scratch);
	}

// TODO heap symantics
// http://amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_2._guide/node0332.html

	void availMem() {
		int d1 = cpu0->readRegister(1);	//attributes
		int bytes = 0x400000;

		if (d1 & MEMF_FAST) bytes = 0;

		cpu0->writeRegister(0, bytes);
		execlog << "availMem " << d1 << " => " << bytes;
	}

	void freeMem() {
		int a1 = cpu0->readRegister(9);
		int d0 = cpu0->readRegister(0);
		// a1,d0
		execlog << "freemem";emit();
	}

	void allocMem() {
		int d0 = cpu0->readRegister(0);
		int d1 = cpu0->readRegister(1);
		int r = cpu0->allocate(d0, d1);
		cpu0->writeRegister(0, r);
		execlog << "allocMem " << d0 << "," << d1 << " => " << r;
		emit();
	}

	void waitPort() {
		int a0 = cpu0->readRegister(8);
		cpu0->writeRegister(0, 0);	// no message available
		execlog << "waitPort";emit();
	}
	void copyMem() {
		int a0 = cpu0->readRegister(8); //src
		int a1 = cpu0->readRegister(9); //dest
		int d0 = cpu0->readRegister(0); //size
		for (int i = 0; i < d0; i++) {
			cpu0->write8(a1 + i, cpu0->read8(a0 + i));
		}
		execlog << "copyMem " << d0;
		emit();
	}
	void replyMsg() {
		int a1 = cpu0->readRegister(9);
		std::string s = cpu0->fetchString(a1);
		execlog << "replyMsg";emit();
	}
	void fakeTask() {
		// to trap $ac(task) oblivion and friends are looking for workbench pointers
//		cpu0->writeRegister(0, 0x801000);
		int task = TASK_BASE;	// 0x80c000;
		cpu0->writeRegister(0, task);
		execlog << "findTask <= " << task; emit();
	}
	void getMsg() {
		int a0 = cpu0->readRegister(8);
		cpu0->writeRegister(0, 0);	// no message available
		execlog << "getMsg";emit();
	}
	void putMsg() {
		execlog << "putMsg"; emit();
	}
};
// b8 184
#ifdef _structcode
// structcode in which 
// long offsets are capitalized 
// word offsets are underscored
structcode taskStruct({
// list:
	{"Head,Tail,Pred,type,pad",{0,4,8,12,13},
// node: 
	{"Succ,Pred,type,pri,Name",{0,4,8,9,10}},
// task: 
	{"Node,flags,state,id,td, Alloc,Wait,Recvd,Except,",{0,14,15,16,17, 18,22,26,30}},
	{"_trapAlloc,_trapEnable,ExceptData,ExceptCode,TrapData,TrapCode,",{34,36, 38,42,46,50}},
	{"StackPointer,StackFloor,StackCeil,Switch,Launch",{54,58,62,66,70}},
	{"MemList,User",{74,88}
});
#endif

