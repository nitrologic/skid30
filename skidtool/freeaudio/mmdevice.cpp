// mmdevice.cpp
// windows mediadevice version 2

#include "freeaudio.h"

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <mmsystem.h>
#include <malloc.h>
#include <stdio.h>
#include <cstdint>
#include <profileapi.h>

extern "C" audiodevice *OpenMultiMediaDevice();

static void __stdcall audioTimerProc( UINT id,UINT msg,DWORD user,DWORD u1,DWORD u2 );

int now() {
	LARGE_INTEGER freq;
	LARGE_INTEGER count;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&count);
	return 0;
}

struct pcmsetting
{
	WAVEFORMATEX	wf; 

	pcmsetting(int freq,int chan,int bits)
	{
		wf.wFormatTag=WAVE_FORMAT_PCM; 
		wf.nChannels=chan; 
		wf.nSamplesPerSec=freq;
		wf.nBlockAlign=chan*bits/8;
		wf.nAvgBytesPerSec=freq*wf.nBlockAlign;
		wf.wBitsPerSample=bits;
		wf.cbSize=0;
	}
};

struct abuffer {
	LPWAVEHDR	hdr;
	u8			*data;
	int			id;
	int handle;

	void init(HWAVEOUT device,int size,int n) {
		hdr=(LPWAVEHDR)malloc(sizeof(WAVEHDR));
		data=(u8*)calloc(size,1);
		id=n;
		hdr->lpData=(LPSTR)data;
		hdr->dwBufferLength=size;
		hdr->dwUser=(DWORD_PTR)this;
		hdr->dwFlags=WHDR_BEGINLOOP|WHDR_ENDLOOP;
		hdr->dwLoops=0x7fffffff;
		waveOutPrepareHeader(device,hdr,sizeof(WAVEHDR));
	}

	void play(HWAVEOUT device) {
		if (waveOutWrite(device,hdr,sizeof(WAVEHDR))) {
			printf("waveOutWrite error\n");
		}
	}

	void finit(HWAVEOUT device) {
		waveOutUnprepareHeader(device,hdr,sizeof(WAVEHDR)); 
		free(data);data=0;
		free(hdr);hdr=0;
	}
};

struct winmmdevice:audiodevice {
	HWAVEOUT	device; 
	int 		buffersize;
	int			samplesize;		//1=8 bit 2=16bit
	abuffer 	buffer;
	int 		bnum;
	int 		mode;			//0=8bit 1=16bit
	int 		playing;
	int			timer;
	int			playpos,mixpos;
	int			timeout;
	int			lagbuffers;
	
	int iswin98()
	{
		OSVERSIONINFO	osinfo;
		osinfo.dwOSVersionInfoSize=sizeof(OSVERSIONINFO);
		if (GetVersionEx(&osinfo))
		{
			if
			(
				(osinfo.dwPlatformId==VER_PLATFORM_WIN32_WINDOWS) && (osinfo.dwMajorVersion>4)
				//((osinfo.dwMajorVersion>4)||((osinfo.dwMajorVersion==4)&&(osinfo.dwMinorVersion>0)))
			)	//osversion=1;
				return 0;
			if (osinfo.dwPlatformId==VER_PLATFORM_WIN32_NT) return 0;//osversion=2;
		}
		return 1;
	}

	int reset()
	{
		int sz=(iswin98())?2048:1024;	//10ms fragment size 20ms for windows98				
		mix=new mixer(sz);
		device=0;
		buffersize=0;
		bnum=0;
		mode=0;
		playing=0;
		timer=0;
		timeout=0;
		playpos=0;
		mixpos=0;
		return init(44100,2,16,sz);
	}

	int close()
	{
		playing=0;
		if (timer) timeKillEvent( timer );
		timer=0;
		mix->releaseall();
		if (device)
		{
			waveOutReset(device);
			buffer.finit(device); 
			waveOutClose(device);
		}
		return 0;
	}
		
	int init(int freq,int channels,int bits,int size)
	{
		pcmsetting	pcm(freq,channels,bits);
		if (bits==8) {mode=0;samplesize=1;} else {mode=1;samplesize=2;}

		DWORD_PTR instance = (DWORD_PTR)this;
		if (waveOutOpen(&device,WAVE_MAPPER,&pcm.wf,0,instance,0)) return 1;
		buffersize=size;
		bnum=0;
		mix->freq=freq;
		mix->channels=channels;
		buffer.init(device,size*samplesize*32,0);
		timeSetEvent(5,5,(LPTIMECALLBACK)audioTimerProc,(DWORD_PTR)this,TIME_ONESHOT);//PERIODIC );
		timeout=0;
		playing=1;
		lagbuffers=6;
		flip();
		buffer.play(device);
		return 0;
	}

	void flip()
	{
		MMTIME	time;
		int		err;

		if (playing==0) return;
		if (timeout)
		{
			if (--timeout) return;
			buffer.play(device);
		}
		memset(&time,0,sizeof(MMTIME));
   		time.wType=TIME_BYTES;
		err=waveOutGetPosition(device,&time,sizeof(MMTIME));		

		if (time.wType!=TIME_BYTES) err=-666;else if (time.u.cb>0x10000000) err=-6667;
		if (err) {
//			printf("waveOutGetPosition failed err=%d\n",err);fflush(stdout);
			waveOutReset(device);
			mixpos=0;
			memset(buffer.data,0,buffersize*samplesize*32);
			buffer.play(device);
			return;
		}
		
		int wavepos=time.u.cb/samplesize;
		int writeahead=buffersize*lagbuffers;
		if (wavepos && wavepos+buffersize*2>mixpos)
		{
			timeout=250;
			memset(buffer.data,0,buffersize*samplesize*32);
			waveOutReset(device);
			if (lagbuffers<10) lagbuffers+=2;
			mixpos=buffersize*lagbuffers*2;
//			printf("collision");fflush(stdout);
			return;			
		}
		int seek=wavepos+writeahead;	
		int nowms = now();
		while (mixpos<seek)
		{
			int f=(mixpos/buffersize)&31;
			if (mode==0) 
			{
				mix->mix8(buffer.data+f*buffersize,0,nowms);
			}
			else
			{
				mix->mix16((short *)buffer.data+f*buffersize,0,nowms);
			}
			mixpos+=buffersize;	
		}
	}
};

static void __stdcall audioTimerProc( UINT id,UINT msg,DWORD user,DWORD u1,DWORD u2 ) {
	((winmmdevice *)user)->flip();
	LPTIMECALLBACK proc = (LPTIMECALLBACK)audioTimerProc;
	timeSetEvent(5,5,proc,(DWORD)user,TIME_ONESHOT);
}

audiodevice *OpenMultiMediaDevice() {
	return new winmmdevice();
}

#endif

