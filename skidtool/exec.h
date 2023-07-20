#pragma once

class IExec{
public:
	virtual void waitPort() = 0;
	virtual void replyMsg() = 0;
};
