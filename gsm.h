#ifndef GSM_H
#define GSM_H
#include <unistd.h>
enum STATUS_GSM{	
	NONE,
	OK,
	ERROR,
	CME_ERROR,
	CMS_ERROR,};
enum STATUS_GSM check_response(char *buf, ssize_t size);
void state_machine(int fd);
#endif
