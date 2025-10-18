// Application layer protocol implementation

#include "application_layer.h"
#include <stdio.h>
#include "link_layer.h"
#include <string.h>

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
    
    
    // abrir file

    llopen(connectionParameters);


    //start and end packets
    //rcver create, write file createFile(…) writeFile(…) parsePck(…) extractDataPck
    //trsmer openFile(…) readFragFile(…) buildCtrlPck(…) buildDataPck(…)
    if (connectionParameters.role == LlRx){
        unsigned char actual_data[MAX_PAYLOAD_SIZE];
        while (llread(actual_data)){
            //get datta
        }
        llclose(); 


        
    } else {
         // ler 1000 em 1000 b
         // eviar 1000 by
        //llwrite();
        llclose();
    }

}
