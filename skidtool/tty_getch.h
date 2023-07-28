#pragma once

#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdlib.h>

void screenSize(int &row,int &col){
	winsize w;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
	row=w.ws_row;
	col=w.ws_col;
}

void mouseOn(){

}

// termios for life

// https://stackoverflow.com/questions/312185/kbhit-on-macos-to-detect-keypress
int getch(){
	char c;
	scanf("%c",&c);	
	return (int)c;
}

int tty_getch() {

  //  int n;

//    ioctl(STDIN_FILENO, FIONREAD, &n);
//	if(n==0) return -1;

	char ch;
	int error;
	static struct termios Otty, Ntty;

//    fflush(stdout);
	tcgetattr(0, &Otty);
	Ntty = Otty;

	Ntty.c_iflag  =  0;     /* input mode       */
	Ntty.c_oflag  =  0;     /* output mode      */
	Ntty.c_lflag &= ~ICANON;    /* line settings    */

#if 1
	/* disable echoing the char as it is typed */
	Ntty.c_lflag &= ~ECHO;  /* disable echo     */
#else
	/* enable echoing the char as it is typed */
	Ntty.c_lflag |=  ECHO;  /* enable echo      */
#endif

	Ntty.c_cc[VMIN]  = 0;//CMIN;    /* minimum chars to wait for */
	Ntty.c_cc[VTIME] = 0;//CTIME;   /* minimum wait time    */
//	#define FLAG TCSAFLUSH
	if ((error = tcsetattr(0, TCSANOW, &Ntty)) == 0) {

		fd_set set;
		struct timeval timeout = {0,100};
		FD_ZERO(&set);
		FD_SET(0,&set);

		int rv = select(0 + 1, &set, NULL, NULL, &timeout);
		if(rv>0){
				error  = read(0, &ch, 1 );        /* get char from stdin */
		}

		error += tcsetattr(0, TCSANOW, &Otty);   /* restore old settings */
	}

	return (error == 1 ? (int) ch : -1 );
}


int getch2(){
	int key=tty_getch();
	if(key==-1){
//		usleep(50000);
	}
}
