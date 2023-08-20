#pragma once

// bespoke computer interface based on public amiga interfaces 
// (C) 2023 Simon Armstrong

class IExec{
public:
	virtual void allocMem() = 0;
	virtual void freeMem() = 0;
	virtual void availMem() = 0;
	virtual void waitPort() = 0;
	virtual void replyMsg() = 0;
	virtual void fakeTask() = 0;
	virtual void openLibrary() = 0;
	virtual void closeLibrary() = 0;
	virtual void openDevice() = 0;
	virtual void doIO() = 0;
	virtual void rawDoFmt() = 0;
	virtual void putMsg() = 0;
	virtual void getMsg() = 0;
	virtual void copyMem() = 0;
	virtual void setSignal() = 0;
	virtual void forbid() = 0;
	virtual void permit() = 0;
};
class IDos {
public:
	virtual void open()=0;
	virtual void close()=0;
	virtual void read()=0;
	virtual void write() = 0;
	virtual void input() = 0;
	virtual void output() = 0;
	virtual void seek() = 0;
	virtual void deletefile() = 0;
	virtual void rename() = 0;
	virtual void lock() = 0;
	virtual void unLock() = 0;
	virtual void dupLock() = 0;
	virtual void examine() = 0;
	virtual void exnext() = 0;
	virtual void info() = 0;
	virtual void createdir() = 0;
	virtual void currentdir() = 0;
	virtual void ioerr() = 0;
	virtual void createproc() = 0;
	virtual void exit() = 0;
	virtual void isinteractive() = 0;
	virtual void getvar() = 0;
	virtual void loadseg() = 0;
	virtual void unloadseg() = 0;
	virtual void delay() = 0;
	virtual void datestamp() = 0;
};
class IBench {
public:
	virtual void closeWorkBench() = 0;
};
class INonVolatile {
public:
	virtual void getCopy() = 0;
};
class IGraphics {
public:
	virtual void textLength() = 0;
	virtual void loadView() = 0;
	virtual void waitTOF() = 0;
	virtual void ownBlitter() = 0;
	virtual void disownBlitter() = 0;
};
class IFFPMath {
public:
};