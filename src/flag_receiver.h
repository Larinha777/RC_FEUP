#ifndef _FLAG_RECEIVER_H_
#define _FLAG_RECEIVER_H_

#define MAX_DATA_SIZE 1000 // duplicado em link_layer.h
#define MAX_PACKET_SIZE (MAX_DATA_SIZE*2 +7)

#define FLAG 0x7E

#define ADDRESS_BY_SENDER 0X03
#define ADDRESS_BY_RECEIVER 0x01

#define CONTROL_SET 0X03
#define CONTROL_UA 0X07
#define I0 0x00
#define I1 0x80
#define RR0 0xAA 
#define RR1 0xAB 
#define REJ0 0x54
#define REJ1 0x55 
#define DISC 0x0B

#define ESC 0x7D
#define XOR_OP 0x20

#include <string.h>

typedef enum setMessageState {START, FLAG_RCV, A_RCV, C_RCV, BCC_OK, SET_RCV, UA_RCV, DATA_RCV, RR0_S, RR1_S, REJ0_S, REJ1_S, DISC_RCV} setMessageState;

typedef struct
{
    unsigned char address;
    unsigned char control;
    setMessageState cur_state;
    unsigned char data[MAX_DATA_SIZE];
} Packet;

int updateCurrentState(Packet *packet, unsigned char byte);

#endif