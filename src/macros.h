#ifndef __MACROS_H__
#define __MACROS_H__

////////////////////////////////////////////////
//MISC
////////////////////////////////////////////////

#define FALSE 0
#define TRUE 1

#define _POSIX_SOURCE 1 // POSIX compliant source

////////////////////////////////////////////////
//Aplication Layer
////////////////////////////////////////////////

// Maximum number of bytes that application layer should send to link layer.
#define MAX_APPL_PACKET_SIZE 1000 

// Maximum number of data bytes sent by the application layer to the link layer
#define MAX_APPL_DATA_SIZE (MAX_APPL_PACKET_SIZE - 3)


#define PROGRESS_BAR_SIZE           40
#define COMPLETED_PROGRESS_CHAR     '='
#define END_PROGRESS_CHAR           '>'
#define NO_PROGRESS_CHAR            '.'

////////////////////////////////////////////////
//Data Link Layer
////////////////////////////////////////////////

// Maximum number of bytes that link layer receives from application layer.
#define MAX_LL_DATA_SIZE MAX_APPL_PACKET_SIZE 

// Maximum number of bytes that link layer receiver may receive from link layer transmiter.
#define MAX_LL_PACKET_SIZE (MAX_LL_DATA_SIZE * 2 + 7)


#define FLAG 0x7E

#define ADDRESS_BY_SENDER 0X03
#define ADDRESS_BY_RECEIVER 0x01

#define CONTROL_SET 0X03
#define CONTROL_UA 0X07
#define CONTROL_I0 0x00
#define CONTROL_I1 0x80
#define CONTROL_RR0 0xAA 
#define CONTROL_RR1 0xAB 
#define CONTROL_REJ0 0x54
#define CONTROL_REJ1 0x55 
#define CONTROL_DISC 0x0B

#define ESC 0x7D
#define XOR_OP 0x20

//Control packets
static unsigned const char SET_PCK[5] = {FLAG, ADDRESS_BY_SENDER, CONTROL_SET, ADDRESS_BY_SENDER ^ CONTROL_SET, FLAG};
static unsigned const char UA_PCK0[5] = {FLAG, ADDRESS_BY_SENDER, CONTROL_UA, ADDRESS_BY_SENDER ^ CONTROL_UA, FLAG};
static unsigned const char UA_PCK1[5] = {FLAG, ADDRESS_BY_RECEIVER, CONTROL_UA, ADDRESS_BY_RECEIVER ^ CONTROL_UA, FLAG};
static unsigned const char RR0_PCK[5] = {FLAG, ADDRESS_BY_SENDER, CONTROL_RR0, ADDRESS_BY_SENDER ^ CONTROL_RR0, FLAG};
static unsigned const char RR1_PCK[5] = {FLAG, ADDRESS_BY_SENDER, CONTROL_RR1, ADDRESS_BY_SENDER ^ CONTROL_RR1, FLAG};
static unsigned const char REJ0_PCK[5] = {FLAG, ADDRESS_BY_SENDER, CONTROL_REJ0, ADDRESS_BY_SENDER ^ CONTROL_REJ0, FLAG};
static unsigned const char REJ1_PCK[5] = {FLAG, ADDRESS_BY_SENDER, CONTROL_REJ1, ADDRESS_BY_SENDER ^ CONTROL_REJ1, FLAG};
static unsigned const char DISC_PCK0[5] = {FLAG, ADDRESS_BY_SENDER, CONTROL_DISC, ADDRESS_BY_SENDER ^ CONTROL_DISC, FLAG};
static unsigned const char DISC_PCK1[5] = {FLAG, ADDRESS_BY_RECEIVER, CONTROL_DISC, ADDRESS_BY_RECEIVER ^ CONTROL_DISC, FLAG};


#endif