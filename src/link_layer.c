// Link layer protocol implementation

#include "link_layer.h"
#include "serial_port.h"
#include "state_machine.h"
#include "alarm.h"

LinkLayer connectParam;
int inf_frame_num = 0;

int receive_packet(unsigned char *buf, setMessageState *state){
    Packet packet;
    packet.cur_state = START;
    packet.data_size = 0;
    int bytes;
    unsigned char byte_rcv;
    while (TRUE)
    {
        bytes = readByteSerialPort(&byte_rcv);
        if (bytes <= 0){
            break;
        }
        if (updateCurrentState(&packet, byte_rcv) == -1){
            printf("Error updating state\n");
            return -1;
        }

        if (packet.cur_state == SET_RCV || packet.cur_state == UA_RCV || packet.cur_state == DISC_RCV
            || packet.cur_state == REJ0_S || packet.cur_state == REJ1_S )
        {
            break;
        } else if (packet.cur_state == RR0_S || packet.cur_state == RR1_S){
            memcpy(buf, packet.data, packet.data_size);
            *state = packet.cur_state;
            return packet.data_size;
        } 
    }
    *state = packet.cur_state;
    return 0;
}

int send_packet_with_retries(const unsigned char *send_buf, int bufSize, setMessageState *ret_state){ 
    if (send_buf == NULL){
        printf("Message to send was NULL\n");
        return -1;
    }
    if (bufSize < 1){
        printf("Size of the buffer is incorrect\n");
        return -1;  
    }
    
    static int handler = 0;
    if (!handler){
        struct sigaction act = {0};
        act.sa_handler = alarmHandler;
        if (sigaction(SIGALRM, &act, NULL) == -1)
        {
            printf("sigaction");
            return -1;
        }
        handler = 1;
    }
    
    alarmCount = 0;
    for (int tries = 0; tries < connectParam.nRetransmissions; tries++){
        printf("Try nº: %d\n", tries+1);

        int written_bytes = writeBytesSerialPort(send_buf, bufSize);
        if (send_buf[2] == CONTROL_I0 || send_buf[2] == CONTROL_I1) connectParam.totalSentIFrames++;
        if (written_bytes == -1){
            alarm(0); 
            alarmEnabled = 0;
            sleep(1);
            continue;     
        } else if (written_bytes < bufSize) {
            alarm(0); 
            alarmEnabled = 0;
            printf("Could not write all bytes\n");
            break;      
        } 

        alarmEnabled = TRUE;
        alarm(connectParam.timeout);

        while (alarmEnabled){
            unsigned char receive_buf;
            setMessageState cur_state;
            int bytes_read = receive_packet(&receive_buf, &cur_state);
            //printf("Left");
            if (bytes_read == -1){
                continue;
            }
        
            if (cur_state == REJ0_S || cur_state == REJ1_S) {
                connectParam.numReceivedREJ++;
            }

            if (cur_state == RR0_S || cur_state == RR1_S) connectParam.numReceivedRR++;

            if (cur_state == UA_RCV || cur_state == DISC_RCV || (cur_state == RR0_S && inf_frame_num == 1)  || (cur_state == RR1_S && inf_frame_num == 0) ){
                *ret_state = cur_state;
                alarm(0); 
                alarmEnabled = 0;
                return bufSize;
            }

            if ((cur_state == REJ0_S && inf_frame_num == 0) || (cur_state == REJ1_S && inf_frame_num == 1)){
                printf("Received REJ\n"); ////
                alarm(0); 
                alarmEnabled = 0;
                break;
            }
        }
    }

    printf("Failed to send packet\n");
    return -1;
}


////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters)
{
    if (openSerialPort(connectionParameters.serialPort, connectionParameters.baudRate) < 0)
    {
        printf("openSerialPort");
        return -1;
    }

    connectParam = connectionParameters;
    
    if (connectionParameters.role == LlTx) { //Transmiter  
        
        setMessageState ret_state;
        if (send_packet_with_retries(SET_PCK, 5, &ret_state) == -1){
            printf("Failed to send packet in llopen of the transmiter\n");
            return -1;
        }
        printf("Send set and received ua\n");////
        
    } else if (connectionParameters.role == LlRx) { //Receiver
        setMessageState state;
        unsigned char buf[MAX_LL_DATA_SIZE] = {0}; 
        
        if (receive_packet(buf, &state) != 0 || state != SET_RCV){
            printf("Failed to receive packet in llopen of the receiver\n");
            return -1;
        } 
        printf("Received Set\n");////

        int written_bytes = writeBytesSerialPort(UA_PCK0, 5);
        if (written_bytes == -1){
            printf("No bytes written\n");
            return -1;        
        } else if (written_bytes < 5) {
            printf("Could not write all bytes\n");
            return -1;        
        } 
        printf("Sent ua\n");////
    }
    return 0;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *data_buf, int data_bufSize)
{ 
    if (data_buf == NULL){
        printf("Poiter to the date is NULL in llwrite\n");
        return -1;
    }
    if (data_bufSize < 1){
        printf("Size of buffer not positive in llwrite\n");
        return -1;      
    }

    unsigned char packet_buf[MAX_LL_PACKET_SIZE] = {0};

    packet_buf[0] = FLAG;
    packet_buf[1] = ADDRESS_BY_SENDER;
    
    if (inf_frame_num == 0){
        packet_buf[2] = CONTROL_I0;
    } else {
        packet_buf[2] = CONTROL_I1;
    }

    packet_buf[3] = packet_buf[1] ^ packet_buf[2];

    unsigned char bcc2 = 0;
    int data_p = 0, pck_p = 4;
    while (data_p < data_bufSize){
        bcc2 ^= data_buf[data_p];
        
        if (data_buf[data_p] == ESC || data_buf[data_p] == FLAG){
            packet_buf[pck_p] = ESC;
            pck_p++;
            packet_buf[pck_p] = data_buf[data_p] ^ XOR_OP; 
        } else {
            packet_buf[pck_p] = data_buf[data_p];
        }
        data_p++;
        pck_p++;
    }
  
    if (bcc2 == ESC || bcc2 == FLAG){
        packet_buf[pck_p++] = ESC;
        packet_buf[pck_p++] = bcc2 ^ XOR_OP; 
        packet_buf[pck_p++] = FLAG;
    } else {
        packet_buf[pck_p++] = bcc2;
        packet_buf[pck_p++] = FLAG;
    }

    setMessageState ret_state = START;
    
    connectParam.dupSentIFrames++;
    if (send_packet_with_retries(packet_buf, pck_p, &ret_state) == -1){
        printf("Error receiving packets in llwrite\n");
        return -1;
    }

    printf("Successfully sent Packet I%d\n", inf_frame_num);////
    inf_frame_num ^= 1;
    return data_p;
    
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet) //
{
    setMessageState state;
    unsigned char buf[MAX_LL_DATA_SIZE] = {0};
    int buf_size = receive_packet(buf, &state);
    printf("Size of packet in llread %d\n", buf_size);////
    if ( buf_size == -1){
        printf("Failed to receive data in llread\n");
        return -1;
    } 
    connectParam.totalReceivedIFrames++;
    
    unsigned char send_msg[5] = {0};
    send_msg[0]=FLAG;
    send_msg[1]=ADDRESS_BY_SENDER;
    send_msg[4]=FLAG;

    int bytes_returned = 0;

    switch (state)
    { // falta adicionar o set_rcv
    case RR0_S:
        // if inf_fram_num == 0 
        // repeated packet was sent 
        // wanted 0 received 1

        // correct packet was sent
        // wanted 1 received 1

        send_msg[2]=CONTROL_RR0;
        send_msg[3]=ADDRESS_BY_SENDER ^ CONTROL_RR0;
        connectParam.numSentRR++;
        if (inf_frame_num == 1)
        { 
            memcpy(packet, buf, buf_size);
            bytes_returned = buf_size;
            inf_frame_num ^= 1;
            connectParam.dupReceivedIFrames++;
        }
        break;

    case RR1_S:
        send_msg[2]=CONTROL_RR1;
        send_msg[3]=ADDRESS_BY_SENDER ^ CONTROL_RR1;
        connectParam.numSentRR++;
        // correct packet was sent 
        // wanted 0 received 0
        if (inf_frame_num == 0){
            memcpy(packet, buf, buf_size);
            bytes_returned = buf_size;
            inf_frame_num ^= 1;
            connectParam.dupReceivedIFrames++;
        }

        // if inf_fram_num == 1 
        // repeated packet was sent 
        // wanted 1 received 0

        break;

    case REJ0_S:

        // faulty packet was sent 
        // wanted 0 received incorrect 0
        if (inf_frame_num == 0){
            send_msg[2] = CONTROL_REJ0;
            send_msg[3] = ADDRESS_BY_SENDER ^ CONTROL_REJ0;
            connectParam.numSentREJ++;
        } 

        // if inf_fram_num == 1 
        // faulty packet sent was a duplicate 
        // wanted 1 received 0 (already has correct 0)
        else 
        {
            send_msg[2] = CONTROL_RR1;
            send_msg[3] = ADDRESS_BY_SENDER ^ CONTROL_RR1;
            connectParam.numSentRR++;
        }
        break;    

    case REJ1_S:
        // if inf_fram_num == 0
        // faulty packet sent was a duplicate 
        // wanted 0 received 1 (already has correct 1)
        if (inf_frame_num == 0){
            send_msg[2] = CONTROL_RR0;
            send_msg[3] = ADDRESS_BY_SENDER ^ CONTROL_RR0;   
            connectParam.numSentRR++;
         
        } 

        // if inf_fram_num == 1
        // faulty packet was sent
        // wanted 1 received faulty 1
        else 
        {
            send_msg[2] = CONTROL_REJ1;
            send_msg[3] = ADDRESS_BY_SENDER ^ CONTROL_REJ1;
            connectParam.numSentREJ++;
        }

        break;    

    default:
        return 0;
    }
    
    int written_bytes = writeBytesSerialPort(send_msg, 5);
    if (written_bytes == -1){
        printf("No bytes written\n");
        return -1;        
    } else if (written_bytes < 5) {
        printf("Could not write all bytes\n");
        return -1;        
    }

    return bytes_returned;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose()
{
    unsigned char buf[5] = {0};
    setMessageState ret_state = START;
    
    if (connectParam.role == LlRx){ //Lx
        while(ret_state != DISC_RCV){
            receive_packet(buf, &ret_state);
        }
        int written_bytes = writeBytesSerialPort(DISC_PCK1, 5);
        if (written_bytes == -1){
            printf("No bytes written\n");
            return -1;        
        } else if (written_bytes < 5) {
            printf("Could not write all bytes\n");
            return -1;        
        }
        
        sleep(1);
        receive_packet(buf, &ret_state);

        printf("Received a total of %d IFrames in which %d were correct\n", connectParam.totalReceivedIFrames, connectParam.dupReceivedIFrames);
        printf("Sent %d RR and %d REJ\n", connectParam.numSentRR, connectParam.numSentREJ);
    } else if (connectParam.role == LlTx) {
        if (send_packet_with_retries(DISC_PCK0, 5, &ret_state) == -1 || ret_state !=DISC_RCV){
            printf("Failed to send packet in llclose\n");
            return -1;
        }

        int written_bytes = writeBytesSerialPort(UA_PCK1, 5);
        if (written_bytes == -1){
            printf("No bytes written\n");
            return -1;        
        } else if (written_bytes < 5) {
            printf("Could not write all bytes\n");
            return -1;        
        }

        printf("Sent a total of %d IFrames in which %d were correct\n", connectParam.totalSentIFrames, connectParam.dupSentIFrames);
        printf("Received %d RR and %d REJ\n", connectParam.numReceivedRR, connectParam.numReceivedREJ);
    }

    if (closeSerialPort() < 0)
    {
        printf("closeSerialPort");
        return -1;
    }
    return 0;
}


