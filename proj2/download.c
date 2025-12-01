
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>

#define SERVER_PORT 21

typedef struct {
    char* user;
    char* password;
    char* host;
    char* url_path;
    char* ip;
    int sockfd;
} Url_data;

int parse_URL(char* url_str, Url_data* url_struct ){
   
    /* url does not start with ftp */
    if (strncmp("ftp", url_str, 3)){ 
        printf("Cannot handle cases where the url does not start with ftp\n");
        return 1;
    }

    char* noFTP = strstr(url_str, "//") + 2;

    char* part1 = strtok(noFTP, "@");
    char* part2 = strtok(NULL, "");

    if (part2 == NULL){ //no login
        url_struct->user = NULL;
        url_struct->password = NULL;
        url_struct->host = strtok(part1, "/");
        url_struct->url_path = strtok(NULL, "");
    } else {
        url_struct->user = strtok(part1, ":");
        url_struct->password = strtok(NULL, "");
        url_struct->host = strtok(part2, "/");
        url_struct->url_path = strtok(NULL, "");
    }
    return 0;
}

int getip(Url_data* url_struct){
    struct hostent *h = gethostbyname(url_struct->host);
    if (h == NULL) {
        herror("gethostbyname");
        printf("Failed to resolve host.\n");
        return 1;
    }
    url_struct->ip = inet_ntoa(*((struct in_addr *) h->h_addr));
    return 0;
}

int send_str(Url_data url_struct, char* buf, int buf_size){
    /*send a string to the server*/
    size_t bytes;
    bytes = write(url_struct.sockfd, buf, buf_size);
    if (bytes > 0)
        printf("Bytes written %ld\n", bytes);
    else {
        perror("write()");
        exit(-1);
    }

}

/**
 * Checks if a file exists in the current directory. If successful, 
 * the function closes the file and returns 1. Otherwise, returns 0.
 */
int file_exists(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (f) {
        fclose(f);
        return 1;
    }
    return 0;
}

/**
 * Opens the file in append mode to allow data writing. It ensures 
 * that the output filename does not overwrite an existing file, by 
 * adding a prefix to the name, until a unique filename is generated.
 */
int createFile(FILE **fptr, const char *filename){ 
    if (filename == NULL){
        printf("Empty string filename in createFile\n");
        return 1;
    } 
    
    char *filename2 = malloc(strlen(filename) + 1);
    strcpy(filename2, filename);
    
    while (file_exists(filename2))
    {
        int len = strlen(filename2);
        char *temp = malloc(len + 2); 
        if (!temp) {
            printf("malloc failed");
            free(filename2);
            return 1;
        }
        
        temp[0] = '1';
        strcpy(temp + 1, filename2);
        free(filename2);
        filename2 = temp;
    }
    
    *fptr = fopen(filename2, "a");
    if (*fptr == NULL) {
        free(filename2);
        printf("File creation failed in createFile\n");
        return 1;
    }
    free(filename2);
    return 0;
} 


int read_file(Url_data url_struct){
    /*read 1000 from the file in the server*/
    FILE *fptr;
    createFile(&fptr, "file_received.html");
    int bytes, buf_size = 1000;
    char buf[buf_size];
    do
    {
        bytes = read(url_struct.sockfd, buf, buf_size);
        if(fwrite(buf, 1, bytes, fptr) != bytes){
            printf("Could not write in the file.\n");
            return 1;
        }
        printf("Bytes read and written in the file %d\n", bytes);
    }while(bytes == 1000);
    return 0;
}

int login(Url_data url_struct){
    if(url_struct.user == NULL && url_struct.password == NULL) {
        printf("There is no user nor password.\n");
        return 0;
    }
    if(url_struct.user == NULL) {
        printf("Missing user.\n");
        return 1;
    }
    if(url_struct.password == NULL) {
        printf("Missing password.\n");
        return 1;
    }

    char buf[256];
    strcpy(buf, "USER ");
    strcat(buf, url_struct.user);
    if (send_str(url_struct, buf, strlen(buf)) == 1) {
        printf("Failed to send user\n");
        return 1;
    }

    strcpy(buf, "PASS ");
    strcat(buf, url_struct.password);
    if (send_str(url_struct, buf, strlen(buf)) == 1) {
        printf("Failed to send password\n");
        return 1;
    }

    return 0;
}


int main(int argc, char **argv) {
    // ftp://demo:password@test.rebex.net/readme.txt


    if (argc > 2){
        printf("Only one argument needed. The rest will be ignored. Carrying ON.\n");
    } else if (argc < 2){
        printf("Missing argument.\n");
        return 1;
    }

    Url_data url;

    if (parse_URL( argv[1], &url) == 1){
        return 1;
    }

    // printf("user: %s\n",url.user);
    // printf("password: %s\n",url.password);
    // printf("host: %s\n",url.host);
    // printf("url_path: %s\n",url.url_path);  

    if (getip(&url) == 1){ // buscar ip e companhia
        printf("Ip not found\n");
        return 1;
    }
    
    struct sockaddr_in server_addr;
    
    /*server address handling*/
    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(url.ip);    /*32 bit Internet address network byte ordered*/
    server_addr.sin_port = htons(SERVER_PORT);        /*server TCP port must be network byte ordered */
    
    /*open a TCP socket*/
    if ((url.sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        return 1;
    }
    
    /*connect to the server*/
    if (connect(url.sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        perror("connect()");
        return 1;
    }

    if (login(url) == 1){
        printf("Failed to login\n");
        return 1;
    }

    if (send_str(url, "PASV", strlen("PASV")) == 1) {
        printf("Failed to send password\n");
        return 1;
    }

    char buf[256];
    strcpy(buf, "RETR ");
    strcat(buf, url.url_path);
    if (send_str(url, buf, strlen(buf)) == 1) {
        printf("Failed to send file request\n");
        return 1;
    }

    if(read_file(url) == 1){
        printf("Failed to read file\n");
        return 1;
    }

    if (send_str(url, "QUIT", strlen("QUIT")) == 1) {
        printf("Failed to send file request\n");
        return 1;
    }

    if (close(url.sockfd)<0) {
        perror("close()");
        return 1;
    }


    // verificar host / path
        // pedir ficheiro
        // verificar se está correto
            // criar ficheiro com esse conteudo    

    return 0;
}