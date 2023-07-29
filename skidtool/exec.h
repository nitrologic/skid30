#pragma once

class IExec{
public:
	virtual void allocMem() = 0;
	virtual void waitPort() = 0;
	virtual void replyMsg() = 0;
	virtual void fakeTask() = 0;
	virtual void openLibrary() = 0;
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
	virtual void deleteFile() = 0;
	virtual void rename() = 0;
	virtual void lock() = 0;
	virtual void unLock() = 0;
	virtual void dupLock() = 0;
};