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
    int bytes = 1;
    while (connectParam.role == LlRx || (connectParam.role == LlTx && bytes == 1 ) )
    {
        unsigned char byte_rcv;
        bytes = readByteSerialPort(&byte_rcv); //this might return -1, maybe check for that
        if (bytes == -1){
            perror("Failed to receive packet\n");
            return -1;
        }

        updateCurrentState(&packet, byte_rcv);

        /*
        printf("Byte received: %c\n", byte);
        printf("var = 0x%02X\n", byte);
        */
        
        if (packet.cur_state == SET_RCV || packet.cur_state == UA_RCV || packet.cur_state == DISC_RCV
            || packet.cur_state == REJ0_S || packet.cur_state == REJ1_S )
        {
            break;
        } else if (packet.cur_state == RR0_S || packet.cur_state == RR1_S){
            buf = packet.data;
            break;
        } 
    }
    *state = packet.cur_state;
    return 0;
}


int send_packet(const unsigned char *send_buf, int bufSize, setMessageState *ret_state){ 
    volatile int STOP = FALSE;
    
    struct sigaction act = {0};
    act.sa_handler = &alarmHandler;
    if (sigaction(SIGALRM, &act, NULL) == -1)
    {
        perror("sigaction");
        return -1;
    }

    writeBytesSerialPort(send_buf, bufSize); // check -1
    sleep(1);

    printf("sent first try\n");


    while (STOP == FALSE)
    {
        unsigned char receive_buf;
        setMessageState cur_state;
        receive_packet(&receive_buf, &cur_state);
        
        if (cur_state == UA_RCV || cur_state == DISC_RCV || (cur_state == RR0_S && inf_frame_num == 1)  || (cur_state == RR1_S && inf_frame_num == 0) ){
            *ret_state = cur_state;
            alarm(0);
            STOP = TRUE;
        }

        if (alarmCount < connectParam.nRetransmissions)
        {
            if ((cur_state == REJ0_S && inf_frame_num == 0) || (cur_state == REJ1_S && inf_frame_num == 1)){
                alarm(connectParam.timeout); 
                alarmCount++;
                alarmEnabled = TRUE;
                writeBytesSerialPort(send_buf, bufSize); // check -1
                sleep(1);
                continue;
            }

            if (alarmEnabled == FALSE)
            {
                alarm(connectParam.timeout); 
                alarmEnabled = TRUE;
                
                writeBytesSerialPort(send_buf, bufSize); // check -1
                sleep(1);
            }
        } 
        else 
        {
            perror("Failed to send packet\n");
            return -1;
        }
    }
    return 0;
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
    printf("opened serial port\n");

    connectParam = connectionParameters;

    setMessageState state;
    
    if (connectionParameters.role == LlTx) { //Transmiter  
        
        unsigned char buf[MAX_DATA_SIZE] = {0}; // Duvidassss!!!!!!!!!!
        buf[0] = FLAG;
        buf[1] = ADDRESS_BY_SENDER;
        buf[2] = CONTROL_SET;
        buf[3] = buf[1] ^ buf[2];
        buf[4] = FLAG;
        
        setMessageState ret_state;
        if (send_packet(buf, 5, &ret_state) != 0){// Duvidassss!!!!!!!!!! Necessario dar check de ua?
            perror("Failed to receive packet in llopen of the transmiter\n");
            return -1;
        }
        printf("sent packet\n");


    } else { //Receiver
        unsigned char buf[MAX_DATA_SIZE] = {0}; // Duvidassss!!!!!!!!!!
        
        if (receive_packet(buf, &state) != 0 || state != SET_RCV){
            perror("Failed to receive packet in llopen of the receiver\n");
            return -1;
        } 
        printf("received packet\n");


        buf[0] = FLAG;
        buf[1] = ADDRESS_BY_SENDER;
        buf[2] = CONTROL_UA;
        buf[3] = buf[1] ^ buf[2];
        buf[4] = FLAG;

        writeBytesSerialPort(buf, 5); // check -1
        printf("sent ua packet\n");

        sleep(1);

    }

    return 0;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *data_buf, int data_bufSize)
{ 
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
    int i = 0;
    while (i < data_bufSize){
        bcc2 ^= data_buf[i];
        
        if (data_buf[i] == ESC || data_buf[i] == FLAG){
            packet_buf[i+4] = ESC;
            i++;
            packet_buf[i+4] = data_buf[i] ^ XOR_OP; 
            data_bufSize++;
        } else {
            packet_buf[i+4] = data_buf[i];
        }
        i++;
    }

    if (bcc2 == ESC || bcc2 == FLAG){
        packet_buf[data_bufSize + 4] = ESC;
        packet_buf[data_bufSize + 5] = bcc2 ^ XOR_OP; 
        packet_buf[data_bufSize + 6] = FLAG;
    } else {
        packet_buf[data_bufSize + 4] = bcc2;
        packet_buf[data_bufSize + 5] = FLAG;
    }

    setMessageState ret_state = START;

    while (!(((ret_state == RR1_S) && (inf_frame_num == 0)) || ((ret_state == RR0_S) && (inf_frame_num == 1)))){
        send_packet(packet_buf, MAX_PACKET_SIZE, &ret_state);
    }

    inf_frame_num ^= 1;
    return 0;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet) //
{
    setMessageState state;
    unsigned char buf[MAX_DATA_SIZE] = {0};
    
    if (receive_packet(buf, &state) != 0){
        perror("Failed to receive packet in llread\n");
        return -1;
    } 

    switch (state)
    {
    case RR0_S:
        // if inf_fram_num == 0 
        // repeated packet was sent 
        // wanted 0 received 1

        // correct packet was sent
        // wanted 1 received 1
        if (inf_frame_num == 1)
        { 
            packet = buf;
        }

        buf[2]=RR0;
        buf[3]=ADDRESS_BY_SENDER ^ RR0;
        break;

    case RR1_S:
        // correct packet was sent 
        // wanted 0 received 0
        if (inf_frame_num == 0){
            packet = buf;
        }

        // if inf_fram_num == 1 
        // repeated packet was sent 
        // wanted 1 received 0

        buf[2]=RR1;
        buf[3]=ADDRESS_BY_SENDER ^ RR1;
        break;

    case REJ0_S:

        // faulty packet was sent 
        // wanted 0 received incorrect 0
        if (inf_frame_num == 0){
            buf[2]=REJ0;
            buf[3]=ADDRESS_BY_SENDER ^ REJ0;
        } 

        // if inf_fram_num == 1 
        // faulty packet sent was a duplicate 
        // wanted 1 received 0 (already has correct 0)
        else 
        {
            buf[2]=RR1;
            buf[3]=ADDRESS_BY_SENDER ^ RR1;
        }
        break;    

    case REJ1_S:
        // if inf_fram_num == 0
        // faulty packet sent was a duplicate 
        // wanted 0 received 1 (already has correct 1)
        if (inf_frame_num == 0){
            buf[2]=RR0;
            buf[3]=ADDRESS_BY_SENDER ^ RR0;            
        } 

        // if inf_fram_num == 1
        // faulty packet was sent
        // wanted 1 received faulty 1
        else 
        {
            buf[2]=REJ1;
            buf[3]=ADDRESS_BY_SENDER ^ REJ1;
        }

        break;    

    default:
        perror("Failed to identify the state in llread\n");
        return -1;
    }

    buf[0]=FLAG;
    buf[1]=ADDRESS_BY_SENDER;
    buf[4]=FLAG;

    writeBytesSerialPort(buf, 5); // check -1
    sleep(1);

    return 0;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose()
{ // to do
    unsigned char buf[MAX_DATA_SIZE] = {0}; // Duvidassss!!!!!!!!!!
    buf[0] = FLAG;
    buf[1] = ADDRESS_BY_SENDER;
    buf[2] = DISC;
    buf[3] = buf[1] ^ buf[2];
    buf[4] = FLAG;
    
    setMessageState ret_state;
    if (send_packet(buf, 5, &ret_state) != 0){
        perror("Failed to send packet in llclose\n");
        return -1;
    }
    buf[2] = CONTROL_UA;
    buf[3] = buf[1] ^ buf[2];
    writeBytesSerialPort(buf, 5); // check -1
    
    if (closeSerialPort() < 0)
    {
        perror("closeSerialPort");
        return -1;
    }

    // TODO: Implement this function

    return 0;
}


