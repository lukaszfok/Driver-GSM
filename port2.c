#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>     
#include <sys/types.h>
#include <sys/stat.h>

enum STATUS_GSM{	
	NONE,
	OK,
	ERROR,
	CME_ERROR,
	CMS_ERROR,};
	
enum STATUS_GSM resp = ERROR;

enum gsm_state{
	check_comunication,
	wfr_comunication,
	check_amount_of_pin,
	wfr_amount_of_pin,
	ask_for_pin,
	enter_pin,
	wfr_enter_pin,
};

enum gsm_state current_state = check_comunication;

enum STATUS_GSM check_response(char *buf, ssize_t size)
{
	if (size == -1)
		return NONE;
	if (strstr(buf, "OK"))
		return OK;
	else if (strstr(buf, "ERROR"))
		return ERROR;
	return NONE;
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
    if(fd<0)
    {
	printf("IS NOT CONNECTION\n");
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
	tty.c_lflag     =   ICANON;          // no signaling chars, no echo, 												canonical processing
	tty.c_oflag     =   0;                  // no remapping, no delays
	tty.c_cc[VMIN]      =   0;              // read doesn't block
	tty.c_cc[VTIME]     =   15;             // 1.5 seconds read timeout

	tty.c_cflag     |=  CREAD | CLOCAL;     // turn on READ & ignore ctrl 							   						lines
	tty.c_iflag     &=  ~(IXON | IXOFF | IXANY);// turn off s/w flow ctrl
	tty.c_lflag     &=  ~(ECHO | ECHOE | ISIG); // make raw
	tty.c_oflag     &=  ~OPOST;              	// make raw

	tcsetattr(fd, TCSANOW, &tty);
	
	while(1){		
	char buf[128];
	int retval;
	int amount;
	char pin[7];
		printf("Mam stan rowny: %d\n",current_state);
		switch(current_state){
		
	 		case check_comunication:
					my_write(fd, "ate0\r\n"); 
					current_state = wfr_comunication;
					break;
				
			case wfr_comunication:	 	
					memset(buf, 0, sizeof(buf));
					retval = read(fd, buf, sizeof(buf));
					resp = check_response(buf, retval);
					if(resp == OK)
						current_state = check_amount_of_pin;
					printf("ANSWER: %d %s\n", resp, buf);	
					break;
					
			case check_amount_of_pin:
					my_write(fd, "AT#PCT\r\n");
					current_state = wfr_amount_of_pin;	
					break;
			
			case wfr_amount_of_pin:
					memset(buf, 0, sizeof(buf));
					retval = read(fd, buf, sizeof(buf)); 
					resp = check_response(buf, retval);
					if(strstr(buf,"#PCT:")){
					sscanf(buf,"#PCT: %d",&amount);
					printf("You have: %d chance yet!\n", amount);
					}	
					if(resp == OK)
							current_state = ask_for_pin;
						printf("ANSWER: %d %s\n", resp, buf);		
					break;		
			case ask_for_pin:
					printf("Enter PIN: \n");
					scanf("%s",pin);
					snprintf(buf,sizeof(buf),"AT+CPIN=\"%d\"\r\n);
							current_state = wfr_enter_pin;
					printf("ANSWER: %d %s\n", resp, buf);	
					break;
			case wfr_enter_pin:
					memset(buf, 0, sizeof(buf));
					retval = read(fd, buf, sizeof(buf));
					resp = check_response(buf, retval);
					if(resp == OK)
						printf("Your Pin is correct");
					if(resp == ERROR)
						current_state = check_amount_of_pin;		
					break;
			default:
				printf("ERROR\n");
				exit(-6);
	    }
	    sleep(1);
    }
}


