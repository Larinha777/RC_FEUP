#ifndef _STATE_MACHINE_H_
#define _STATE_MACHINE_H_

#include <string.h>
#include <stdio.h>
#include "macros.h"

typedef enum setMessageState {START, FLAG_RCV, A_RCV, C_RCV, BCC_OK, SET_RCV, UA_RCV, DATA_RCV, RR0_S, RR1_S, REJ0_S, REJ1_S, DISC_RCV} setMessageState;

typedef struct
{
    unsigned char address;
    unsigned char control;
    setMessageState cur_state;
    unsigned char data[MAX_LL_DATA_SIZE];
    unsigned int data_size; 
} Packet;

int updateCurrentState(Packet *packet, unsigned char byte);

#endif