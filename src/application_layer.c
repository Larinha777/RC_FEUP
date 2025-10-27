// Application layer protocol implementation

#include "application_layer.h"
#include <stdio.h>
#include "link_layer.h"
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>


void print_pck(unsigned char *packet, int packet_size){
    for(int i = 0; i < packet_size; i++){
        printf("var = 0x%02X\n", packet[i]);
    }
}

int big_endian_to_int(unsigned char *out, int len) {
    int value = 0;
    for (int i = 0; i < len; i++) {
        value = (value << 8) | out[i];
    }
    return value;
}

int int_to_big_endian(int value, unsigned char *out) {
    int size = 0;

    if (value == 0) {
        out[0] = 0;
        return 1;
    }

    int temp = value;
    while (temp > 0) {
        temp >>= 8;
        size++;
    }

    for (int i = 0; i < size; i++) {
        out[size - 1 - i] = (value >> (i * 8)) & 0xFF;
    }
    return size;
}

int file_exists(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (f) {
        fclose(f);
        return 1;
    }
    return 0;
}

//used by Rx - confirmar se nome é repetido
int createFile(FILE **fptr, const char *filename){ 
    if (filename == NULL){
        perror("Empty string filename in createFile\n");
        return -1;
    } 

    char *filename2 = malloc(strlen(filename) + 1);
    strcpy(filename2, filename);

    while (file_exists(filename2))
    {
        int len = strlen(filename2);
        char *temp = malloc(len + 2); 
        if (!temp) {
            perror("malloc failed");
            free(filename2);
            return -1;
        }

        temp[0] = '1';
        strcpy(temp + 1, filename2);
        free(filename2);
        filename2 = temp;
    }

    *fptr = fopen(filename2, "a");
    if (*fptr == NULL) {
        free(filename2);
        perror("File creation failed in createFile\n");
        return -1;
    }
    free(filename2);
    return 0;
} 

int writeFile(FILE *fptr, unsigned char *data, int data_size){
    if (fptr == NULL){
        perror("File pointer is null in writeFile\n");
        return -1;
    }
    if (data == NULL){
        perror("Data pointer is null in writeFile\n"); // CORRIGIR -> ESTA A PRINTAR ESTA MSG
        return -1;
    }
    if (data_size < 1){
        perror("Size of data was not correctly given to writeFile\n");
        return -1;
    }

    if(fwrite(data, 1, data_size, fptr) < 0){
        perror("Could not write in the file in writeFile\n");
        return -1;
    }

    return 0;
}

//to do reviews, need to do end packet
int parsePck(unsigned char *packet, int packet_size, char **filename, int *file_size){
    if(packet[0] == 1){ // packet control START
        int p_index = 1; 
        while(p_index < packet_size){
            unsigned char T = packet[p_index++];
            unsigned char L = packet[p_index++];
            if(T == 0){
                *file_size = big_endian_to_int(packet+p_index, L);
                printf("Start parse of file with size %d\n", *file_size);
            }
            else if(T == 1){
                *filename = malloc(L);
                memcpy(*filename, &packet[p_index], L);
            }
            p_index += L;
        }
        return 1;

    } else if(packet[0] == 3){
        int p_index = 1; 
        while(p_index < packet_size){
            unsigned char T = packet[p_index++];
            unsigned char L = packet[p_index++];
            if(T == 0){
                int end_file_size = big_endian_to_int(packet + p_index, L);

                if (end_file_size != *file_size){
                    perror("Size in Start packet differs from End packet\n");
                    return -1;
                }
            }
            else if(T == 1){
                char *end_filename = malloc(L);
                memcpy(end_filename, &packet[p_index], L);
                if (strcmp(end_filename, *filename) != 0){
                    perror("Filename in Start packet differs from End packet\n");
                    return -1;
                }
            }
            p_index += L;
        }
        return 3;

    } else if(packet[0] == 2) {
        return 2;
    }
    return -1;
}

int extractDataPck(unsigned char *packet, int packet_size, unsigned char *data, int *data_size){
    // print_pck(packet, packet_size);
    if(packet[0] == 2){
        *data_size = 256 * packet[1] + packet[2];
        if(packet_size != *data_size + 3 ) return -1;
        memcpy(data, packet+3 , *data_size);
        return 0;
    }
    else return -1;
}

//used by Tx
int openFile(const char *filename, FILE **fptr){
    if (filename[0] == '\0'){
        perror("Empty string filename in openFile\n");
        return -1;
    } 
    *fptr = fopen(filename, "r");
    if (!*fptr) {
        perror("Failed to open file\n");
        return -1;
    }
    return 0;
} 

int readFragFile(FILE *fptr, unsigned char *data, int data_size){
    int size_read = fread(data, 1, data_size, fptr);

    if(size_read < 0){
        return -1;
    }
    return size_read;
} 

void buildCtrlPck(unsigned char *packet, int *packet_size, unsigned char control_field, const char *filename, int file_total_size){   
    unsigned char size_str[8];
    
    int len_filename = strlen(filename);
    int len_file_total_size = int_to_big_endian(file_total_size, size_str);

    // Get packet size            
    *packet_size = 1 + 2 + len_filename + 2 + len_file_total_size;
    
    int index = 0;
    packet[index++] = control_field;
    
    // type 0 ->  file size
    packet[index++] = 0;
    packet[index++] = len_file_total_size;
    memcpy((packet) + index, size_str, len_file_total_size);
    index += len_file_total_size;

    // type 1 ->  file name
    packet[index++] = 1;
    packet[index++] = len_filename;
    memcpy((packet) + index, filename, len_filename);

} 

void buildDataPck(unsigned char *packet, int *packet_size, unsigned char *data, int *data_size){
    // Get packet size            
    *packet_size = *data_size + 3;

    //Create packet
    (packet)[0] = 2;
    (packet)[1] = *data_size >> 8;
    (packet)[2] = *data_size & 0x0FF;
    memcpy(&packet[3],data, *data_size);
} 

void closeFile(FILE *fptr){
    fclose(fptr);
}

void testConnection(LinkLayer connectionParameters){
    unsigned char test_msg[] = "Hello, this is a test message!";
    unsigned char rcv_buf[100] = {0};
    int msg_size = sizeof(test_msg);

    if (connectionParameters.role == LlTx){
        printf("-Tx: Sending test message\n");
        if (llwrite(test_msg, msg_size) == -1){
            perror("Could not send test message\n");
            return;
        }
        printf("-Tx: Test message sent\n");
    } else if (connectionParameters.role == LlRx){
        printf("-Rx: Waiting to receive test message\n");
        int rcv_size = llread(rcv_buf);
        if (rcv_size == -1){
            perror("Could not receive test message\n");
            return;
        }
        printf("-Rx: Test message received: %.*s\n", rcv_size, rcv_buf);
    }
}



void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    LinkLayer connectionParameters;
    strcpy(connectionParameters.serialPort, serialPort);
    if (strcmp(role, "tx") == 0){
        connectionParameters.role = LlTx;
    } else if (strcmp(role, "rx") == 0) {
        connectionParameters.role = LlRx;
    } else {
        perror("Did not understand role");
        return;
    }
    connectionParameters.baudRate = baudRate;
    connectionParameters.nRetransmissions = nTries;
    connectionParameters.timeout = timeout;
    
    // open connection 
    // done by both Tx and Rx
    if (llopen(connectionParameters) == -1){
        perror("Could not open connection\n");
        return;
    }
    
    printf("-llopen complete\n");

    // testConnection(connectionParameters);
    // return;

    if (connectionParameters.role == LlRx){ //Rx
        printf("-Reached rx in app layer\n");
        
        unsigned char packet[MAX_PAYLOAD_SIZE];
        char *filename_rcv;
        int file_size = 0;

        int parse_res = 0;
        int packet_size;
        do{
            packet_size = llread(packet); // gets start packet
            if(packet_size == -1) return;
            else if (packet_size == 0) continue;
            printf("-Read 1 packet(prob start)\n");
            parse_res = parsePck(packet, packet_size, &filename_rcv, &file_size);
            if(parse_res == -1){
                perror("Could not parse packet received\n");
                return;
            } 
        } while (parse_res != 1);

        FILE *fptr = NULL;
        if (createFile(&fptr, filename) == -1) return;
        unsigned char data[MAX_PAYLOAD_SIZE] = {0};
        int data_size;
        int data_acc = 0;
        while(TRUE){
            packet_size = llread(packet); 
            if(packet_size == -1) return;
            if(packet_size == 0) continue;
            parse_res = parsePck(packet, packet_size, &filename_rcv, &file_size);
            if(parse_res == -1) return;
            else if(parse_res == 2) { 
                if (extractDataPck(packet, packet_size, data, &data_size) == -1){
                    perror("Failed to extract data \n");
                    return;
                }
                data_acc += data_size;
            }
            if (parse_res == 3) break;           
            if(writeFile(fptr, data, data_size) == -1) return;
        }
        // if (data_acc != file_size){
        //     printf("%d %d\n", data_acc, file_size);
        //     perror("Total size of the data received is different from expected data");
        //     return;
        // }

        //confirmar se filesize correto
        if (llclose() == -1){
            perror("Error on disconnecting in llclose\n");
        }

    } else { //Tx
        printf("-Reached tx in app layer\n");

        FILE * file = NULL;
        if (openFile(filename, &file) == -1) return;

        // Create start packet
        unsigned char packet[MAX_PAYLOAD_SIZE] = {0} ;
        int packet_size;
        
        // Get file size
        int fd =fileno(file);
        struct stat st;
        fstat(fd, &st);
        int file_total_size = st.st_size;
        

        buildCtrlPck(packet, &packet_size, 1, filename, file_total_size);

        printf("-Built start packet\n");

        if (llwrite(packet, packet_size) == -1){
            perror("Could not send START packet\n");
            return;
        }

        int data_buf_size = MAX_PAYLOAD_SIZE - 4, frag_file_res ;
        unsigned char data_buf[MAX_PAYLOAD_SIZE - 4] = {0}; 
        int i = 1;
        while (TRUE){ // there is data in the file
            frag_file_res = readFragFile(file, data_buf, data_buf_size);
            printf("-%d Read frag file with size %d, %d\n", i, data_buf_size, frag_file_res);
            if(frag_file_res < 1) break;
            buildDataPck(packet, &packet_size, data_buf, &frag_file_res); 

            if (llwrite(packet, packet_size) == -1){
                perror("Could not send data packet\n");
                return;
            }
            i++;
        }

        buildCtrlPck(packet, &packet_size, 3, filename, file_total_size);
        llwrite(packet, packet_size);

        if (llclose() == -1){
            perror("Error on disconnecting in llclose\n");
        }
    }

}
