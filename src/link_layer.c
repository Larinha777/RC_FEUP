// Link layer protocol implementation

#include "link_layer.h"
#include "serial_port.h"
#include "flag_receiver.h"
#include "alarm.h"
// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

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
        bytes = readByteSerialPort(&byte_rcv); //this might return -1, maybe check for that
        if (bytes == -1){
            break;
        }
        if (updateCurrentState(&packet, byte_rcv) == -1){
            perror("Error updating state\n");
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

// maybe rename this
int send_packet(const unsigned char *send_buf, int bufSize, setMessageState *ret_state){ 
    if (send_buf == NULL){
        perror("Message to send was NULL\n");
        return -1;
    }
    if (bufSize < 1){
        perror("Size of the buffer is impossible\n");
        return -1;  
    }
    
    static int handler = 0;
    if (!handler){
        struct sigaction act = {0};
        act.sa_handler = alarmHandler;
        if (sigaction(SIGALRM, &act, NULL) == -1)
        {
            perror("sigaction");
            return -1;
        }
        handler = 1;
    }
    
    alarmCount = 0;
    for (int tries = 0; tries < connectParam.nRetransmissions; tries++){

        int written_bytes = writeBytesSerialPort(send_buf, bufSize);
        if (written_bytes == -1){
            alarm(0); 
            alarmEnabled = 0;
            perror("No bytes written\n");
            break;      
        } else if (written_bytes < bufSize) {
            alarm(0); 
            alarmEnabled = 0;
            perror("Could not write all bytes\n");
            break;      
        } 

        alarmEnabled = TRUE;
        alarm(connectParam.timeout);

        while (alarmEnabled){

            unsigned char receive_buf;
            setMessageState cur_state;
            int bytes_read = receive_packet(&receive_buf, &cur_state);
            if (bytes_read == -1){
                continue;
            }
        
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
        perror("openSerialPort");
        return -1;
    }

    connectParam = connectionParameters;

    setMessageState state;
    
    if (connectionParameters.role == LlTx) { //Transmiter  

        unsigned char buf[MAX_DATA_SIZE] = {0}; 
        buf[0] = FLAG;
        buf[1] = ADDRESS_BY_SENDER;
        buf[2] = CONTROL_SET;
        buf[3] = buf[1] ^ buf[2];
        buf[4] = FLAG;
        
        setMessageState ret_state;
        if (send_packet(buf, 5, &ret_state) == -1){
            printf("Failed to send packet in llopen of the transmiter\n");
            return -1;
        }
        printf("Send set and received ua\n");////


    } else { //Receiver
        unsigned char buf[MAX_DATA_SIZE] = {0}; 
        
        if (receive_packet(buf, &state) != 0 || state != SET_RCV){
            perror("Failed to receive packet in llopen of the receiver\n");
            return -1;
        } 
        printf("Received Set\n");////


        buf[0] = FLAG;
        buf[1] = ADDRESS_BY_SENDER;
        buf[2] = CONTROL_UA;
        buf[3] = buf[1] ^ buf[2];
        buf[4] = FLAG;

        int written_bytes = writeBytesSerialPort(buf, 5);
        if (written_bytes == -1){
            perror("No bytes written\n");
            return -1;        
        } else if (written_bytes < 5) {
            perror("Could not write all bytes\n");
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
        perror("Poiter to the date is NULL in llwrite\n");
        return -1;
    }
    if (data_bufSize < 1){
        perror("Size of buffer not positive in llwrite\n");
        return -1;      
    }

    unsigned char packet_buf[MAX_PACKET_SIZE] = {0};

    packet_buf[0] = FLAG;
    packet_buf[1] = ADDRESS_BY_SENDER;
    
    if (inf_frame_num == 0){
        packet_buf[2] = I0;
    } else {
        packet_buf[2] = I1;
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
        
    if (send_packet(packet_buf, pck_p, &ret_state) == -1){
        perror("Error receiving packets in llwrite\n");
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
    unsigned char buf[MAX_DATA_SIZE] = {0};
    int buf_size = receive_packet(buf, &state);
    printf("Size of packet in llread %d\n", buf_size);////
    if ( buf_size == -1){
        perror("Failed to receive data in llread\n");
        return -1;
    } 
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

        send_msg[2]=RR0;
        send_msg[3]=ADDRESS_BY_SENDER ^ RR0;
        if (inf_frame_num == 1)
        { 
            memcpy(packet, buf, buf_size);
            bytes_returned = buf_size;
        }
        break;

    case RR1_S:
        send_msg[2]=RR1;
        send_msg[3]=ADDRESS_BY_SENDER ^ RR1;
        // correct packet was sent 
        // wanted 0 received 0
        if (inf_frame_num == 0){
            memcpy(packet, buf, buf_size);
            bytes_returned = buf_size;
        }

        // if inf_fram_num == 1 
        // repeated packet was sent 
        // wanted 1 received 0

        break;

    case REJ0_S:

        // faulty packet was sent 
        // wanted 0 received incorrect 0
        if (inf_frame_num == 0){
            send_msg[2]=REJ0;
            send_msg[3]=ADDRESS_BY_SENDER ^ REJ0;
        } 

        // if inf_fram_num == 1 
        // faulty packet sent was a duplicate 
        // wanted 1 received 0 (already has correct 0)
        else 
        {
            send_msg[2]=RR1;
            send_msg[3]=ADDRESS_BY_SENDER ^ RR1;
        }
        break;    

    case REJ1_S:
        // if inf_fram_num == 0
        // faulty packet sent was a duplicate 
        // wanted 0 received 1 (already has correct 1)
        if (inf_frame_num == 0){
            send_msg[2]=RR0;
            send_msg[3]=ADDRESS_BY_SENDER ^ RR0;            
        } 

        // if inf_fram_num == 1
        // faulty packet was sent
        // wanted 1 received faulty 1
        else 
        {
            send_msg[2]=REJ1;
            send_msg[3]=ADDRESS_BY_SENDER ^ REJ1;
        }

        break;    

    default:
        perror("Failed to identify the state in llread\n");
        return -1;
    }
    
    int written_bytes = writeBytesSerialPort(send_msg, 5);
    if (written_bytes == -1){
        perror("No bytes written\n");
        return -1;        
    } else if (written_bytes < 5) {
        perror("Could not write all bytes\n");
        return -1;        
    }

    inf_frame_num ^= 1;
    return bytes_returned;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose()
{ // to do
    unsigned char buf[MAX_DATA_SIZE] = {0};
    buf[0] = FLAG;
    buf[2] = DISC;
    buf[4] = FLAG;
    setMessageState ret_state;
    
    
    if (connectParam.role == LlRx){
        buf[1] = ADDRESS_BY_RECEIVER;
        buf[3] = buf[1] ^ buf[2];
        ret_state = START;
        while(ret_state != DISC_RCV){
            receive_packet(buf, &ret_state);
        }
        int written_bytes = writeBytesSerialPort(buf, 5);
        if (written_bytes == -1){
            perror("No bytes written\n");
            return -1;        
        } else if (written_bytes < 5) {
            perror("Could not write all bytes\n");
            return -1;        
        }
        
        sleep(1);
        receive_packet(buf, &ret_state);

    } else {
        buf[1] = ADDRESS_BY_SENDER;
        buf[3] = buf[1] ^ buf[2];
        if (send_packet(buf, 5, &ret_state) == -1 || ret_state !=DISC_RCV){
            perror("Failed to send packet in llclose\n");
            return -1;
        }

        buf[2] = CONTROL_UA;
        buf[3] = buf[1] ^ buf[2];

        int written_bytes = writeBytesSerialPort(buf, 5);
        if (written_bytes == -1){
            perror("No bytes written\n");
            return -1;        
        } else if (written_bytes < 5) {
            perror("Could not write all bytes\n");
            return -1;        
        }
    }

    if (closeSerialPort() < 0)
    {
        perror("closeSerialPort");
        return -1;
    }

    return 0;
}


