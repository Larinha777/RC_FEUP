#include "state_machine.h"

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
                return 0;
            }
            if (byte == ADDRESS_BY_SENDER || byte == ADDRESS_BY_RECEIVER){
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
            if (byte == CONTROL_SET || byte == CONTROL_UA || byte == CONTROL_DISC ||
                byte == CONTROL_RR0 || byte == CONTROL_RR1 || byte == CONTROL_REJ0 || 
                byte == CONTROL_REJ1 || byte == CONTROL_I0 || byte == CONTROL_I1){
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
            if ((packet->address ^ packet->control) == byte){
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
                    case CONTROL_DISC:
                    packet->cur_state = DISC_RCV;
                    return 0;
                    case CONTROL_RR0:
                    packet->cur_state = RR0_S;
                    return 0;
                    case CONTROL_RR1:
                    packet->cur_state = RR1_S;
                    return 0;
                    case CONTROL_REJ0:
                    packet->cur_state = REJ0_S;
                    return 0;
                    case CONTROL_REJ1:
                    packet->cur_state = REJ1_S;
                    return 0;
                
                default:
                    break;
                }
            }
            if (packet->control == CONTROL_I0 || packet->control == CONTROL_I1){
                
                data_bcc = 0;
                last_data_byte = byte;
                data_counter = 0;
                packet->data_size = 0;

                packet->cur_state = DATA_RCV;
                return 0;
            }
            break;
            
        case DATA_RCV:
            if (byte == FLAG){
                if (data_bcc == last_data_byte){
                    packet->data_size = data_counter;
                    if (packet->control == CONTROL_I0){
                        packet->cur_state = RR1_S;
                    } else if (packet->control == CONTROL_I1){
                        packet->cur_state = RR0_S;
                    }
                    return 0;
                } else {
                    if (packet->control == CONTROL_I0){
                        packet->cur_state = REJ0_S;
                    } else if (packet->control == CONTROL_I1){
                        packet->cur_state = REJ1_S;
                    } 
                    return 0;
                }
            } 

            // byte destuffing
            if ((last_data_byte == ESC) && (last_esc == 0)){
                last_esc = 1;
                last_data_byte = byte ^ XOR_OP;
            } else {
                data_bcc ^= last_data_byte;
                packet->data[data_counter] = last_data_byte;
                last_data_byte = byte;
                data_counter ++;
                last_esc = 0;
            }

            return 0;

        default:
            return -1;
    }

    memset(packet->data, 0, MAX_APPL_DATA_SIZE);
    packet->cur_state = START;
    return 0;
}




