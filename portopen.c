#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>     
#include <sys/types.h>
#include <sys/stat.h>
#include "gsm.h"

//File descriptor
int gsm_serial;
int main(int argc, char *argv[])
{
    gsm_serial=open("/dev/ttyUSB0",O_RDWR);
    if(gsm_serial<0)
    {
	printf("IS NOT CONNECTION\n");
        return 0;
    }
	struct termios tty;
	
	tcgetattr(gsm_serial, &tty);
	
	cfsetispeed(&tty, B115200);
	cfsetospeed(&tty, B115200);

	/* Setting other Port Stuff */
	tty.c_cflag     &=  ~PARENB;        // Make 8n1
	tty.c_cflag     &=  ~CSTOPB;
	tty.c_cflag     &=  ~CSIZE;
	tty.c_cflag     |=  CS8;
	tty.c_cflag     &=  ~CRTSCTS;       // no flow control
	tty.c_lflag     =   ICANON;          // no signaling chars, no echo, 												canonical processing
	tty.c_oflag     =   0;                  // no remapping, no delays
	tty.c_cc[VMIN]      =   0;              // read doesn't block
	tty.c_cc[VTIME]     =   15;             // 1.5 seconds read timeout

	tty.c_cflag     |=  CREAD | CLOCAL;     // turn on READ & ignore ctrl 							   						lines
	tty.c_iflag     &=  ~(IXON | IXOFF | IXANY);// turn off s/w flow ctrl
	tty.c_lflag     &=  ~(ECHO | ECHOE | ISIG); // make raw
	tty.c_oflag     &=  ~OPOST;              	// make raw

	tcsetattr(gsm_serial, TCSANOW, &tty);
	
	while(1){		
		state_machine(gsm_serial);
	    sleep(1);
    }
}


