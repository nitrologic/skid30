#pragma once

class IExec{
public:
	virtual void allocMem() = 0;
	virtual void freeMem() = 0;
	virtual void waitPort() = 0;
	virtual void replyMsg() = 0;
	virtual void fakeTask() = 0;
	virtual void openLibrary() = 0;
	virtual void closeLibrary() = 0;
	virtual void rawDoFmt() = 0;
	virtual void putMsg() = 0;
	virtual void getMsg() = 0;
	virtual void copyMem() = 0;
	virtual void setSignal() = 0;
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
};
class IBench {
public:
	virtual void closeWorkBench() = 0;
};
