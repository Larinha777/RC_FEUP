#include "flag_receiver.h"

int data_counter;
int data_bcc;
unsigned char last_data_byte;
int last_esc = 0;

int updateCurrentState(Packet *packet, unsigned char byte){
    switch (packet->cur_state)
    {
        case START:
            if (byte == FLAG){
                packet->cur_state = FLAG_RCV;
                return 0;
            }
            break;

        case FLAG_RCV:
            if (byte == FLAG){
                packet->cur_state = FLAG_RCV;
                return 0;
            }
            if (byte == ADDRESS_BY_SENDER){
                packet->cur_state = A_RCV;
                packet->address = byte;
                return 0;
            }
            break;

        case A_RCV:
            if (byte == FLAG){
                packet->cur_state = FLAG_RCV;
                return 0;
            }
            if (byte == CONTROL_SET || byte == CONTROL_UA || byte == DISC ||
                byte == RR0 || byte == RR1 || byte == REJ0 || byte == REJ1 || byte == I0 || byte == I1){
                packet->cur_state = C_RCV;
                packet->control = byte;
                return 0;
            }
            break;

        case C_RCV:
            if (byte == FLAG){
                packet->cur_state = FLAG_RCV;
                return 0;
            }
            if (packet->address ^ packet->control == byte){
                packet->cur_state = BCC_OK;
                return 0;
            } 
            break;

        case BCC_OK:
            if (byte == FLAG){
                switch (packet->control)
                {
                    case CONTROL_SET:
                    packet->cur_state = SET_RCV;
                    return 0;
                    case CONTROL_UA:
                    packet->cur_state = UA_RCV;
                    return 0;
                    case DISC:
                    packet->cur_state = DISC_RCV;
                    return 0;
                    case RR0:
                    packet->cur_state = RR0_S;
                    return 0;
                    case RR1:
                    packet->cur_state = RR1_S;
                    return 0;
                    case REJ0:
                    packet->cur_state = REJ0_S;
                    return 0;
                    case REJ1:
                    packet->cur_state = REJ1_S;
                    return 0;
                
                default:
                    break;
                }
            }
            if (packet->control == I0 || packet->control == I1){
                
                data_bcc = byte;
                last_data_byte = byte;
                data_counter = 0;

                packet->cur_state = DATA_RCV;
                return 0;
            }
            break;
            
        case DATA_RCV:
            if (byte == FLAG){
                if (data_bcc == 0){
                    if (packet->control == I0){
                        packet->cur_state = RR1_S;
                    } else if (packet->control == I1){
                        packet->cur_state = RR0_S;
                    }
                    return 0;
                } else {
                    if (packet->control == I0){
                        packet->cur_state = REJ0_S;
                    } else if (packet->control == I1){
                        packet->cur_state = REJ1_S;
                    } 
                    return 0;
                }
            } 

            if ((last_data_byte == ESC) && (last_esc == 0)){
                last_esc = 1;
                last_data_byte = byte ^ XOR_OP;
            } else {
                data_bcc ^= byte;
                packet->data[data_counter] = last_data_byte;
                data_counter ++;
                last_esc = 0;
            }
        
            if (data_counter > MAX_DATA_SIZE){ //more data than expected
                if (packet->control == I0){
                    packet->cur_state = REJ0_S;
                } else if (packet->control == I1){
                    packet->cur_state = REJ1_S;
                }                 
                return 0;
            }

            packet->cur_state = DATA_RCV;
            return 0;

        default:
            return -1;
    }
    

    memset(packet->data, 0, MAX_DATA_SIZE);
    packet->cur_state = START;
    return 0;
}




