#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>     
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include "gsm.h"
/*!
* \brief Enum is assigned to variabul which check state GSM respons  
*/
enum STATUS_GSM resp = ERROR;
/*!
* \brief Enum have all of machine state of GSM
*
* Enum stories state and first state check the comunication with modem next 
* verifies comunication next chceck SIM card state next verifies card state 
* response next variable check what SIM need if need PIN go to PIN stan and 
* ask user about PIN if need PUK go to PUK state and ask user for PUK next step 
* is a verifies correct of PIN/PUK.The driver can change the password and when pin is block 
* user can unblock the sim before enter the PUK after PIN. 
*/
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
	change_your_pin,
	wfr_change_pin,
	end,
};
/*! \brief Function serch in buf to map predefined status value
*	
* Function have two paramether
* \param[in] first patameter stores a bufor
* \param[in] second check a size of bufor
* \return GSM status or NONE if nothing found
* \attention If size is -1 return NONE or if string in bufor is OK return OK
* else if string on the bufor is ERROR return ERROR	
*/
enum STATUS_GSM check_response(char *buf, ssize_t size)
{	
	if (size == -1) 
		return NONE;
	if (strstr(buf, "OK"))
		return OK;
	else if (strstr(buf, "ERROR"))
		return ERROR;
	else if(strstr(buf,"READY")){
		return READY;
	}
	return NONE;
}
/*!
* \brief Function sent string to GSM 
* \param[in] first patameter stores a file
* \param[in] second paremeter stores a data
* \return bufor, comand sent to GSM 
*/
int my_write(int file, char *data)
{
       return write(file, data, strlen(data) + 1);
}
/*! 
* \brief Function have all logic of GSM Driver 
* 		
* Function switch chceck all status of GSM request and response 
* and comunication with modem by AT comand.
*
* \param[in] open comunication port bettwen modem GSM and user
*/
void state_machine(int fd){
	static clock_t endwait;
	char buf[128]; /**< Table of bufor type character size 128 */
	int retval;    /**< Read value */
	int amount;    /**< Store a scan bufor*/
	char pin[8];   /**< Table for pin size 7*/
	char newpin[8];
	char puk[20];  /**< Table for puk size 11*/
	static char lck_type[7];  /**< Table for sim lock type size 7 */
	static int subcurrent_state = wfr_comunication; /**< New stan for PUK and pin verifie*/
	/*!
	* Defines first status for check comunication with modem
	*/  
	static enum gsm_state current_state = check_comunication;
	/*!
	* Defines status to old for check differences bettwen a current 
	*/
	static enum gsm_state old_state = end;
	
	
		if(current_state!=old_state){
		
			old_state=current_state;
			printf("Mam stan rowny: %d\n",current_state);
		}
		
		switch(current_state){
		
	 		case check_comunication:
					my_write(fd, "ate0\r\n");
					endwait = clock() + 5 * CLOCKS_PER_SEC; 
					current_state = wfr_comunication;
					break;
				
			case wfr_comunication:	
					memset(buf, 0, sizeof(buf));
					retval = read(fd, buf, sizeof(buf));
					resp = check_response(buf, retval);
					
						if(resp == OK){
							current_state = check_sim_state;
						}else if(endwait < clock())
						{
							current_state =	check_comunication;
						}	
							
					printf("ANSWER: %d %s\n", resp, buf);	
					break;
					
			case check_sim_state:
					my_write(fd, "AT+CPIN?\r\n");
					endwait = clock() + 5 * CLOCKS_PER_SEC; 
					current_state = wfr_spin_respons;
					break;
					
			case wfr_spin_respons:
					memset(buf, 0, sizeof(buf));
					retval = read(fd, buf, sizeof(buf)); 
					resp = check_response(buf, retval);	
						if(strstr(buf,"SIM PIN")||strstr(buf,"SIM PUK")){
					
							sscanf(buf,"+CPIN: SIM %4s", lck_type);
							printf("You must write a %s!\n", lck_type);
							subcurrent_state = check_amount_of_pin_or_puk;
						
						}else if(resp == READY){
					
							subcurrent_state = change_your_pin; 
						
						}else if (resp == OK){
					
							current_state = subcurrent_state;
						}else if(endwait < clock())
						{
							current_state =	check_comunication;
						}	
					printf("ANSWER: %d %s\n", resp, buf);
					break;
							
			case check_amount_of_pin_or_puk:
					my_write(fd, "AT#PCT\r\n");
					endwait = clock() + 5 * CLOCKS_PER_SEC;
					if(strncmp(lck_type,"PIN",3)==0){
					current_state = wfr_amount_of_pin;
					}else if(strncmp(lck_type,"PUK",3)==0){
					current_state = wfr_amount_of_puk;
					}	
					break;
			
			case wfr_amount_of_pin:
					memset(buf, 0, sizeof(buf));
					retval = read(fd, buf, sizeof(buf)); 
					resp = check_response(buf, retval);
					
						if(strstr(buf,"#PCT:")){
					
							sscanf(buf,"#PCT: %d",&amount);
							printf("You have: %d chance yet! Enter %s\n", amount, lck_type);
						}else if(resp == OK){
								current_state = ask_for_pin;
						}else if(amount == 1){
								printf("WARRNING! YOU HAVE LAST CHANCE!\n");	
						}else if(endwait < clock()){
							current_state = check_comunication;
						}
						
						
					printf("ANSWER: %d %s\n", resp, buf);		
					break;
					
			case wfr_amount_of_puk:
					memset(buf, 0, sizeof(buf));
					retval = read(fd, buf, sizeof(buf)); 
					resp = check_response(buf,retval);
					
						if(strstr(buf,"#PCT:")){
						
							sscanf(buf,"#PCT: %d",&amount);
							printf("You have: %d chance yet!\n", amount);
						}
						if(resp == OK){
							current_state = ask_for_puk;	
						}else if(amount == 1){
							printf("WARRNING! YOU HAVE LAST CHANCE!\n");
						}else if(endwait < clock()){
							current_state = check_comunication;
						}
						
					printf("ANSWER: %d %s\n", resp, buf);
					break;
							
			case ask_for_pin:
					printf("Enter PIN: \n");
					scanf("%6s%*s",pin);
					endwait = clock() + 5 * CLOCKS_PER_SEC; 
					snprintf(buf,sizeof(buf),"AT+CPIN=\"%s\"\r\n",pin);
					my_write(fd,buf);
					current_state = wfr_enter_pin;
					printf("ANSWER: %d %s\n", resp, buf);	
					break;
					
			case ask_for_puk:
					printf("Enter your \"PUK\",\"NEWPIN\": \n");
					scanf("%11s%*s %7s%*s ",puk,pin);
					endwait = clock() + 5 * CLOCKS_PER_SEC; 
					snprintf(buf,sizeof(buf),"AT+CPIN=\"%s\",\"%s\"\r\n",puk,pin);
					my_write(fd,buf);
					current_state = wfr_enter_puk;
					printf("ANSWER: %d %s\n", resp, buf);
					break;
							
			case wfr_enter_pin:		
					memset(buf, 0, sizeof(buf));
					retval = read(fd, buf, sizeof(buf));
					resp = check_response(buf, retval);
					
						if(resp == OK){
							printf("Your Pin is correct\n");
						}else if(resp == ERROR){
							current_state = check_sim_state;
						}else if(endwait < clock()){
							current_state = check_comunication;
						}	
					break;
					
			case wfr_enter_puk:	
					memset(buf, 0, sizeof(buf));
					retval = read(fd, buf, sizeof(buf));
					resp = check_response(buf, retval);
						if(resp == OK){
							printf("PUK is correct!\n");
							current_state = check_sim_state;
						}else if(resp==ERROR){
							current_state = check_amount_of_pin_or_puk;
						}else if(endwait < clock()){
							current_state = check_comunication;
						}	
					break;
										
			case change_your_pin:
					printf("Enter your old PIN and new PIN: \n");
					scanf("%7s%*s %7s%*s",pin,newpin);
					endwait = clock() + 5 * CLOCKS_PER_SEC; 
					snprintf(buf,sizeof(buf),"AT+CPIN=\"%s\",\"%s\"\r\n",pin,newpin);
					my_write(fd,buf);
					current_state = wfr_change_pin;
					printf("ANSWER: %d %s\n", resp, buf);
					break;
					
			case wfr_change_pin:
					memset(buf, 0, sizeof(buf));
					retval = read(fd, buf, sizeof(buf));
					resp = check_response(buf, retval);
					
						if(resp == OK){
							printf("Your PIN is change!\n");
						}else if(resp == ERROR){
							current_state = change_your_pin;
						}else if(endwait < clock()){
							current_state = check_comunication;
						}
						break;
										
			case end:
					printf("THE END\n");
					break;
			
			default:
				printf("ERROR\n");
				exit(-6);
	    }



}
