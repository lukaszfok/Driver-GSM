#ifndef GSM_H
#define GSM_H
#include <unistd.h>
/*!
* \brief Enum stores a status of GSM
*
* Enum allows stores and check what status GSM have at this moment
*/
enum STATUS_GSM{	
	NONE,
	OK,
	ERROR,
	CME_ERROR,
	CMS_ERROR,};
/*!
* \brief Enum is assigned to function which chceck response from GSM
*	
* Function have two paramether
* \param[in] first patameter stores a bufor
* \param[in] second check a size of bufor	
*/	
enum STATUS_GSM check_response(char *buf, ssize_t size);
/*!
* \brief Function stories state of machine 
* \param[in] open comunication port bettwen modem GSM and user		
*/
void open_comunication(int fd);
#endif
