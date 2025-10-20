// Application layer protocol implementation

#include "application_layer.h"
#include <stdio.h>
#include "link_layer.h"
#include <string.h>
#include <stdlib.h>

#define FILE_NAME "penguin-received.gif"

//used by Rx
int createFile(FILE *fptr, unsigned char filename, unsigned char file_size){ //hardcoded for project
    fptr = fopen(FILE_NAME, "a");
    if (fptr == NULL) {
        perror("File creation failed\n");
        return -1;
    }

    return 0;
} 

int writeFile(FILE *fptr, unsigned char *data, unsigned char data_size){
    if(fwrite(data, 1, data_size, fptr) < 0){
        return -1;
    }
    return 0;
}

int parsePck(unsigned char *packet, unsigned char packet_size, char *filename, char *file_size){
    if(packet[0] == 1){ // packet control START 
        int p_index = 1; 
        while(p_index < packet_size){
            unsigned char T = packet[p_index++];
            unsigned char L = packet[p_index++];
            if(T == 0){
                *file_size = 0;
                for(int i = 0; i < L; i++){
                    *file_size = (*file_size << 8) + packet[p_index + i];
                }
            }
            else if(T == 1){
                memcpy(filename, &packet[p_index], L);
                filename[L] = '\0';
            }
            p_index += L;
        }
        return 1;

    } else if(packet[0] == 3){
        //fazer end
        return 3;
    } else if(packet[0] == 2) {
        return 2;
    }
    return -1;
}

int extractDataPck(unsigned char *packet, unsigned char packet_size, unsigned char *data, unsigned char *data_size){
    if(packet[0] == 2){
        *data_size = 256 * packet[1] + packet[2];
        if(packet_size != *data_size + 3 ) return -1;
        memcpy(data, &packet[3] , *data_size);
        return 0;
    }
    else return -1;
}

//used by Tx
int openFile(const char *filename, FILE *fptr){
    fptr = fopen(filename, "r");
    if (fptr == NULL) {
        perror("Failed to open file\n");
        return -1;
    }
    return 0;
} 

int readFragFile(FILE *fptr, unsigned char *data, unsigned char *data_size){
    data_size =(unsigned char *) fread(data, 1, *data_size, fptr);
    if(data_size == 0) return 1;
    if(data_size < 0){
        return -1;
    }
    return 0;
} 

void buildCtrlPck(unsigned char *packet, unsigned char *packet_size, 
            unsigned char control_field, int param_count,
            unsigned char *T, unsigned char *L, const char **V){
    
    // Get packet size            
    *packet_size = 1;
    for (int i = 0; i < param_count; i++){
        packet_size += 2 + L[i];
    }
    packet = malloc(*packet_size);

    //Get packet
    int p_index = 0;
    packet[p_index] = control_field;
    p_index ++;

    for (int i = 0; i < param_count; i++){
        packet[p_index] = T[i];
        packet[p_index + 1] = L[i];
        memcpy(&packet[p_index + 2],V[i], L[i]);
        p_index += L[i] + 2;
    }    
} 

void buildDataPck(unsigned char *packet, unsigned char *packet_size, unsigned char *data, unsigned char data_size){
    // Get packet size            
    *packet_size = data_size + 3;
    packet = malloc(*packet_size);

    //Create packet
    packet[0] = 2;
    packet[1] = data_size >> 8;
    packet[2] = data_size & 0x0FF;
    memcpy(&packet[3],data, data_size);

} 

void closeFile(FILE *fptr){
    fclose(fptr);
}

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    printf("reached app layer\n");
    LinkLayer connectionParameters;
    strcpy(connectionParameters.serialPort, serialPort);
    if (strcmp(role, "tx")){
        connectionParameters.role = LlTx;
    } else if (strcmp(role, "rx")) {
        connectionParameters.role = LlRx;
    } else {
        perror("Did not understand role");
        return;
    }
    connectionParameters.baudRate = baudRate;
    connectionParameters.nRetransmissions = nTries;
    connectionParameters.timeout = timeout;
    
    printf("defined parameters\n");
    
    // open connection 
    // done by both Tx and Rx
    llopen(connectionParameters);
    
    printf("opened connection\n");

    
    if (connectionParameters.role == LlRx){ //Rx
        printf("reached rx\n");
        
        unsigned char packet[MAX_PAYLOAD_SIZE];
        char *filename_rcv = FILE_NAME, *file_size = NULL;

        int parse_res = 0;
        unsigned char packet_size;
        do{
            packet_size = llread(packet); // gets start packet
            if(packet_size == -1) return;
            parse_res = parsePck(packet, packet_size, filename_rcv, file_size);
            if(parse_res == -1) return;
        } while (parse_res != 1);
        
        FILE *fptr = NULL;
        if (createFile(fptr, *filename_rcv, *file_size) == -1) return;
        unsigned char *data = NULL, data_size;
        while(TRUE){
            packet_size = llread(packet); // gets start packet
            if(packet_size == -1) return;
            parse_res = parsePck(packet, packet_size, filename_rcv, file_size);
            if(parse_res == -1) return;
            else if(parse_res == 2) {
                extractDataPck(packet, packet_size, data, &data_size);
            }
            if (parse_res == 3) break;           
            if(writeFile(fptr, data, data_size) == -1) return;
        }
        //llclose(); 

    } else { //Tx
        printf("reached tx\n");

        FILE * file = NULL;
        if (openFile(filename, file) == -1) return;

        // Create start packet
        unsigned char *packet = NULL;
        unsigned char packet_size;

        fseek(file, 0L, SEEK_END);
        char file_size = ftell(file);
        rewind(file);

        unsigned char T[2] = {0,1};
        unsigned char L[2] = {sizeof(filename), sizeof(file_size)};
        const char *V[2] = {filename, &file_size};

        buildCtrlPck(packet, &packet_size, 1, 2, T, L, V);
        llwrite(packet, packet_size);

        unsigned char *data_buf = NULL;
        unsigned char data_buf_size;
        unsigned char frag_file_res;
        
        while (TRUE /*ha cenas no ficheiro*/){
            frag_file_res =readFragFile(file, data_buf,&data_buf_size);
            if(frag_file_res == 1) break;
            buildDataPck(packet, &packet_size, data_buf, data_buf_size); 
            llwrite(packet, packet_size);
        }

        buildCtrlPck(packet, &packet_size, 3, 2, T, L, V);
        llwrite(packet, packet_size);

        llclose();
    }

}
