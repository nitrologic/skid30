#pragma once
#include <cstdio>
#include <cstdlib>

#include <deque>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>

std::mutex inputMutex;
std::mutex availableMutex;
std::condition_variable inputAvailable;
std::deque<int> inputQueue;	
bool empty=true; // shared variable
std::atomic<bool> stopReadThread=false;

void readInputThread(){
	while(!stopReadThread){
		int ch=getchar();
//		int ch=std::getc(stdin);
		if(ch>0){
			{
				std::unique_lock lock(inputMutex);
				inputQueue.push_back(ch);
				empty=false;
			}
//			inputAvailable.notify_all();
			inputAvailable.notify_one();
		}else{
			return;
		}
	}
}

int waitChar(){
	while(true)
	{
		std::unique_lock<std::mutex> lock(inputMutex);
		if(inputQueue.empty()){
			inputAvailable.wait(lock,[]{return empty;});
		}
		if(!inputQueue.empty()){
			int value=inputQueue.front();
			inputQueue.pop_front();
			return value;
		}
	}
}

int getChar() {
	int value=-1;
	if(!inputQueue.empty()){
		std::unique_lock lock(inputMutex);
		value=inputQueue.front();
		inputQueue.pop_front();
	}
	return value;
}

std::thread *readThread;

#ifdef WIN32
#include <windows.h>
#include <conio.h>
#include <synchapi.h>

HANDLE hStdin;

void initConsole()
{
	timeBeginPeriod(1);	
	SetConsoleOutputCP(CP_UTF8);
//	SetConsoleCP(CP_UTF8);
	hStdin = GetStdHandle(STD_INPUT_HANDLE);
	SetConsoleMode(hStdin, 0);
	readThread = new std::thread(readInputThread);
}

void uninitConsole(){
	stopReadThread=true;
	FreeConsole();
//	CloseHandle(hStdin);
	readThread->join();
}

void sleep(int ms)
{
	Sleep(ms);
}

int getch2() {
	if (_kbhit()) {
		return getch();
	}
	return -1;
}

SYSTEMTIME epoch = { 1978,1,0,1 };

void dayminticks(int *dmt) {
	SYSTEMTIME time;
	GetLocalTime(&time);
	double v0,v1;
	SystemTimeToVariantTime(&epoch,&v0);
	SystemTimeToVariantTime(&time,&v1);
	dmt[0] = (v1-v0);
	dmt[1] = time.wHour * 60 + time.wMinute;
	dmt[2] = time.wSecond * 50 + (time.wMilliseconds/20);
}

void usleep(int micros) {
	int millis = micros / 1e6;
	Sleep(millis);
}
int millis() {
	return GetTickCount();
}
void screenSize(int& columns, int& rows) {
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	BOOL ok = GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
	if(ok){
		columns = csbi.srWindow.Right - csbi.srWindow.Left + 1;
		rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
	}else{
		columns = 40;
		rows = 25;
	}
}

int mouseOn() {
	HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
	int fdwMode = ENABLE_EXTENDED_FLAGS;
	if (!SetConsoleMode(hStdin, fdwMode)) return -1;
	fdwMode = ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT;
	if (!SetConsoleMode(hStdin, fdwMode)) return -1;
	return 0;
}

#endif

#ifdef __linux__
#include <inttypes.h>
#include <unistd.h>

#include <ctime>
#include <sys/ioctl.h>
#include <termios.h>
#include <sys/time.h>

static struct termios old, new1;

void resetTermios(void) {
    tcsetattr(0, TCSANOW, &old);
}

void initConsole()
{
    tcgetattr(0, &old); /* grab old terminal i/o settings */
    new1 = old; /* make new settings same as old settings */
    new1.c_lflag &= ~ICANON; /* disable buffered i/o */
//    new1.c_lflag &= echo ? ECHO : ~ECHO; /* set echo mode */
    tcsetattr(0, TCSANOW, &new1); /* use these new terminal i/o settings now */
#ifdef NCURSES
	initscr();
	timeout(200);
#endif
	readThread = new std::thread(readInputThread);
}

void uninitConsole()
{
	pthread_cancel(readThread->native_handle());
//	readThread->join();
}


void screenSize(int &row,int &col){
	winsize w;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
	row=w.ws_row;
	col=w.ws_col;
}

void sleep(int ms){
	usleep(ms*1e3);
}

int getch3(){
	char c;
	scanf("%c",&c);	
	return (int)c;
}

// CLOCK_REALTIME

uint64_t millis(){
	timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
	return (spec.tv_sec*1000)+(spec.tv_nsec/1e6);
}

struct std::tm jan1978 = {0,0,0,1,1,78};

void dayminticks(int *dmt) {
	timeval t;
	gettimeofday(&t,NULL);
	std::time_t epoch = std::mktime(&jan1978);   // get time now
	std::time_t now = std::time(0);   // get time now
	std::tm* time = std::localtime(&now);
	int days=std::difftime(now, epoch) / (60 * 60 * 24);;
	int mins=time->tm_hour*60+time->tm_min;
	int ticks=t.tv_sec*50+t.tv_usec/2e4;
    dmt[0]=days;
	dmt[1]=mins;
	dmt[2]=ticks;
}



#endif

#ifdef __APPLE__

#include <ctime>
#include "tty_getch.h"
#include <sys/time.h>

void initConsole(){
	readThread = new std::thread(readInputThread);	
}


int millis(){
	uint64_t t=clock_gettime_nsec_np(CLOCK_UPTIME_RAW);
	return (int)(t/1e6);
}

void sleep(int ms){
	usleep(ms*1e3);
}

#ifdef NO_TM
struct tm {
	int	tm_sec;		/* seconds after the minute [0-60] */
	int	tm_min;		/* minutes after the hour [0-59] */
	int	tm_hour;	/* hours since midnight [0-23] */
	int	tm_mday;	/* day of the month [1-31] */
	int	tm_mon;		/* months since January [0-11] */
	int	tm_year;	/* years since 1900 */
	int	tm_wday;	/* days since Sunday [0-6] */
	int	tm_yday;	/* days since January 1 [0-365] */
	int	tm_isdst;	/* Daylight Savings Time flag */
	long	tm_gmtoff;	/* offset from UTC in seconds */
	char	*tm_zone;	/* timezone abbreviation */
};
#endif

struct std::tm jan1978 = {0,0,0,1,1,78};

void dayminticks(int *dmt) {
	timeval t;
	gettimeofday(&t,NULL);
	std::time_t epoch = std::mktime(&jan1978);   // get time now
	std::time_t now = std::time(0);   // get time now
	std::tm* time = std::localtime(&now);
	int days=std::difftime(now, epoch) / (60 * 60 * 24);;
	int mins=time->tm_hour*60+time->tm_min;
	int ticks=t.tv_sec*50+t.tv_usec/2e4;
    dmt[0]=days;
	dmt[1]=mins;
	dmt[2]=ticks;
}

#endif

