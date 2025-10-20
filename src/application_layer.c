// Application layer protocol implementation

#include "application_layer.h"
#include <stdio.h>
#include "link_layer.h"
#include <string.h>


//used by Rx
void createFile(/**/){} 
void writeFile(){}
void parsePck(unsigned char *packet){}
void extractDataPck(unsigned char *packet){}

//used by Tx
void openFile(const char *filename){} 
void readFragFile(/**/){} 
void buildCtrlPck(/**/){} 
void buildDataPck(/**/){} 

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    
    LinkLayer connectionParameters;
    strcpy(connectionParameters.serialPort, serialPort);
    if (strcmp(role, "tx")){
        connectionParameters.role = LlTx;
    } else {
        connectionParameters.role = LlRx;
    }
    connectionParameters.baudRate = baudRate;
    connectionParameters.nRetransmissions = nTries;
    connectionParameters.timeout = timeout;
    
    
    // open connection 
    // done by both Tx and Rx
    llopen(connectionParameters);

    
    if (connectionParameters.role == LlRx){ //Rx
        
        unsigned char packet[MAX_PAYLOAD_SIZE];
        llread(packet); // gets start packet
        createFile(/**/);
        while (TRUE /*data != endpacket*/){ // maybe state machine
            llread(packet);
            parsePck(packet); // q faz isto??
            extractDataPck(packet);
            writeFile(/**/);
        }
        
        llclose(); 

    } else { //Tx
        openFile(filename); 
        buildCtrlPck(/**/); //start packet
        while (TRUE /*ha cenas no ficheiro*/){
            readFragFile(/**/);
            unsigned char *data_buf;
            int data_bufSize;
            buildDataPck(/**/); 
            llwrite(data_buf, data_bufSize);
        }

        buildCtrlPck(/**/); //end packet

        llclose();
    }

}
