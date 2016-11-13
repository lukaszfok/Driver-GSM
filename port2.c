#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>     
#include <sys/types.h>
#include <sys/stat.h>



enum STATUS_CODE {
	NONE,
	OK,
	ERROR,
	CME_ERROR,
	CMS_ERROR,
	TEST
};

enum STATUS_CODE check_answer(char *buf, ssize_t size)
{
	if (size == -1)
		return NONE;
	if (strstr(buf, "OK"))
		return OK;
	else if (strstr(buf, "ERROR"))
		return ERROR;
	
	return NONE;
}

enum ANSWER {
	FOUND,
	NOT_FOUND
};

enum ANSWER check_specific_answer(char *buf, size_t size,
		char *expected_data, size_t size2)
{
	if (strstr(buf, expected_data))
		return FOUND;
	return NOT_FOUND;
}

int my_write(int file, char *data)
{
	return write(file, data, strlen(data) + 1);
}

//File descriptor
int fd;
int main(int argc, char *argv[])
{
    fd=open("/dev/ttyUSB0",O_RDWR);
    /*printf("argumenty: %d\n", argc);
    for (int i = 0; i < argc; i++) {
    	printf("Arg %d: %s\n", i, argv[i]);
    }*/
    if (argc != 3)
    	exit(-5);
    if (fd<0)
    {
	printf("IS NOT CONNECTION");
        return 0;
    }

        struct termios tty;
	
	tcgetattr(fd, &tty);
	
	cfsetispeed(&tty, B115200);
	cfsetospeed(&tty, B115200);

/* Setting other Port Stuff */
tty.c_cflag     &=  ~PARENB;        // Make 8n1
tty.c_cflag     &=  ~CSTOPB;
tty.c_cflag     &=  ~CSIZE;
tty.c_cflag     |=  CS8;
tty.c_cflag     &=  ~CRTSCTS;       // no flow control
tty.c_lflag     =   ICANON;          // no signaling chars, no echo, no canonical processing
tty.c_oflag     =   0;                  // no remapping, no delays
tty.c_cc[VMIN]      =   0;                  // read doesn't block
tty.c_cc[VTIME]     =   15;                  // 1.5 seconds read timeout

tty.c_cflag     |=  CREAD | CLOCAL;     // turn on READ & ignore ctrl lines
tty.c_iflag     &=  ~(IXON | IXOFF | IXANY);// turn off s/w flow ctrl
tty.c_lflag     &=  ~(ECHO | ECHOE | ISIG); // make raw
tty.c_oflag     &=  ~OPOST;              // make raw

	tcsetattr(fd, TCSANOW, &tty);
 

	char buf[128];
	ssize_t retval;
	enum STATUS_CODE code;
	
	my_write(fd, "ate0\r\n");
	do{
		memset(buf, 0, sizeof(buf));
		retval = read(fd, buf, sizeof(buf));
		if (retval != -1) printf("ANSWER: <%d> %s\n", retval, buf);

	}while ((code = check_answer(buf, retval)) == NONE);
	
	if (code != OK)
		exit(-1);

	memset(buf, 0, sizeof(buf));
	
	my_write(fd, "at+cpin?\r\n");

	do{
		memset(buf, 0, sizeof(buf));
		retval = read(fd, buf, sizeof(buf));
		if (retval != -1) printf("ANSWER: <%d> %s\n", retval, buf);
	}while ((code = check_answer(buf, retval)) == NONE &&
		check_specific_answer(buf, retval, "+CPIN: ", sizeof("+CPIN:") == NOT_FOUND));
	
	if (code == ERROR)
		exit(-2);
	if (code == NONE) {
		if (strstr(buf, "SIM PIN")) {
		sprintf(buf, sizeof(buf), "AT+CPIN=\"%s\"\r\n", argv[1]);
			printf("PIN required\n");
		}
	}
    return 1;
	
}

