#pragma once

#include <termios.h>
#include <stdio.h>
#include <unistd.h>

//"/home/skid/simon/skid30/";

// https://stackoverflow.com/questions/312185/kbhit-on-macos-to-detect-keypress
int tty_getch() {
    char ch;
    int error;
    static struct termios Otty, Ntty;

    fflush(stdout);
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

    Ntty.c_cc[VMIN]  = CMIN;    /* minimum chars to wait for */
    Ntty.c_cc[VTIME] = CTIME;   /* minimum wait time    */

#if 1
    /*
    * use this to flush the input buffer before blocking for new input
    */
    #define FLAG TCSAFLUSH
#else
    /*
    * use this to return a char from the current input buffer, or block if
    * no input is waiting.
    */
    #define FLAG TCSANOW

#endif

    if ((error = tcsetattr(0, FLAG, &Ntty)) == 0) {
        error  = read(0, &ch, 1 );        /* get char from stdin */
        error += tcsetattr(0, FLAG, &Otty);   /* restore old settings */
    }

    return (error == 1 ? (int) ch : -1 );
}
