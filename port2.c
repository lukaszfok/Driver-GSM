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
	check_sim_state,
	wfr_spin_respons,
	check_amount_of_pin_or_puk,
	wfr_amount_of_pin,
	wfr_amount_of_puk,
	ask_for_pin,
	ask_for_puk,
	wfr_enter_pin,
	wfr_enter_puk,
	end,
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
	char puk[11];
	char lck_type[7];
	int subcurrent_state = wfr_comunication;
		printf("Mam stan rowny: %d\n",current_state);
		switch(current_state){
		
	 		case check_comunication: /* 0 */
					my_write(fd, "ate0\r\n"); 
					current_state = wfr_comunication;
					break;
				
			case wfr_comunication:	 /* 1 */	
					memset(buf, 0, sizeof(buf));
					retval = read(fd, buf, sizeof(buf));
					resp = check_response(buf, retval);
					if(resp == OK)
						current_state = check_sim_state;
					printf("ANSWER: %d %s\n", resp, buf);	
					break;
			case check_sim_state: /* 2 */
					my_write(fd, "AT+CPIN?\r\n");
					current_state = wfr_spin_respons;	
					break;
			case wfr_spin_respons:   /* 3 */
					memset(buf, 0, sizeof(buf));
					retval = read(fd, buf, sizeof(buf)); 
					resp = check_response(buf, retval);
					if(strstr(buf,"SIM PIN")){
						sscanf(buf,"+CPIN: SIM %s", lck_type);
						printf("You must write a %s!", lck_type);
						
						subcurrent_state = check_amount_of_pin_or_puk;
					}else if(strstr(buf,"SIM PUK")){
						sscanf(buf,"+CPIN: SIM %s", lck_type);
						printf("You must write a %s!", lck_type);
						
						subcurrent_state = check_amount_of_pin_or_puk;
					}else if(strstr(buf,"READY")){
							subcurrent_state = end; /* end for a test*/
					}else subcurrent_state = end;
					 if (resp == OK)
						current_state = subcurrent_state;
						
							printf("ANSWER: %d %s\n", resp, buf);		
					break;		
			case check_amount_of_pin_or_puk:		/* 4 */
					my_write(fd, "AT#PCT\r\n");
					current_state = wfr_amount_of_pin;	
					break;
			
			case wfr_amount_of_pin:		/* 5 */
					memset(buf, 0, sizeof(buf));
					retval = read(fd, buf, sizeof(buf)); 
					resp = check_response(buf, retval);
					if(strstr(buf,"#PCT:")){
					sscanf(buf,"#PCT: %d",&amount);
					printf("You have: %d chance yet! Enter %s\n", amount, lck_type);
					}	
					if(resp == OK)
							current_state = ask_for_pin;
					if(amount == 1){
							printf("WARRNING! YOU HAVE LAST CHANCE!");
					}
							printf("ANSWER: %d %s\n", resp, buf);		
					break;
			case wfr_amount_of_puk:  /* 6 */
					memset(buf, 0, sizeof(buf));
					resp = check_response(buf,retval);
					if(strstr(buf,"#PCT:")){
					sscanf(buf,"#PCT: %d",&amount);
					printf("You have: %d chance yet!\n", amount);
					}	
					if(resp == OK)
							current_state = ask_for_puk;
					if(amount == 1){
							printf("WARRNING! YOU HAVE LAST CHANCE!");
					}
						printf("ANSWER: %d %s\n", resp, buf);
					break;		
			case ask_for_pin:		/* 7 */
					printf("Enter PIN: \n");
					scanf("%s",pin);
					snprintf(buf,sizeof(buf),"AT+CPIN=\"%s\"\r\n",pin);
					my_write(fd,buf);
							current_state = wfr_enter_pin;
					printf("ANSWER: %d %s\n", resp, buf);	
					break;
			case ask_for_puk:		/* 8 */
					printf("Enter PUK: \n");
					scanf("%s",puk);
					snprintf(buf,sizeof(buf),"AT+CPIN=\"%s\"\r\n",puk);
					my_write(fd,buf);
							current_state = wfr_enter_pin;
					printf("ANSWER: %d %s\n", resp, buf);
					break;		
			case wfr_enter_pin:		/* 9 */		
					memset(buf, 0, sizeof(buf));
					retval = read(fd, buf, sizeof(buf));
					resp = check_response(buf, retval);
					if(resp == OK)
						printf("Your Pin is correct\n");
					if(resp == ERROR)
						current_state = check_amount_of_pin_or_puk;		
					break;
			case wfr_enter_puk:		/* 10 */		
					memset(buf, 0, sizeof(buf));
					retval = read(fd, buf, sizeof(buf));
					resp = check_response(buf, retval);
					if(resp == OK)
						printf("Your PUK is correct\n");
					if(resp == ERROR)
						current_state = check_amount_of_pin_or_puk;		
					break;
			case end:
				printf("THE END\n");
			break;
			default:
				printf("ERROR\n");
				exit(-6);
	    }
	    sleep(1);
    }
}


