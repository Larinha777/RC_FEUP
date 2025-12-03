#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <sys/select.h>
#include <errno.h>

#define SERVER_PORT 21
#define USER_DEFAULT "anonymous"
#define PASS_DEFAULT "password"

typedef struct {
    char* user;
    char* password;
    char* host;
    char* url_path;
    char* res_ip;
    int res_sockfd;    
    char* data_ip;
    int data_port;
    int data_sockfd;
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
    url_struct->res_ip = inet_ntoa(*((struct in_addr *) h->h_addr));
    printf("%s\n", url_struct->res_ip);
    return 0;
}

int get_sockfd(char* ip_adddr, int port){
    struct sockaddr_in server_addr;
    int sockfd;
    
    /*server address handling*/
    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip_adddr);    /*32 bit Internet address network byte ordered*/
    server_addr.sin_port = htons(port);        /*server TCP port must be network byte ordered */
    
    /*open a TCP socket*/
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        return -1;
    }
    
    /*connect to the server*/
    if (connect(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        perror("connect()");
        return -1;
    }

    return sockfd;
}

int send_str(Url_data url_struct, char* buf, int buf_size){
    /*send a string to the server*/
    size_t bytes;
    bytes = write(url_struct.res_sockfd, buf, buf_size);
    if (bytes > 0)
        printf("Bytes written %ld\n", bytes);
    else {
        perror("write()");
        return 1;
    }
    return 0;
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

int get_filename(Url_data url_struct, char **filename){
    char *next = strtok(url_struct.url_path, "/");
    
    do{
        *filename = next;
        next = strtok(NULL, "/");
    } while (next != NULL);

    return 0;
}

int read_file(Url_data url_struct){
    /*read 1000 from the file in the server*/
    FILE *fptr;
    char *filename;
    get_filename(url_struct, &filename);

    createFile(&fptr, filename);
    int bytes, buf_size = 1000;
    char buf[buf_size];
    do
    {
        bytes = read(url_struct.data_sockfd, buf, buf_size);
        if(fwrite(buf, 1, bytes, fptr) != bytes){
            printf("Could not write in the file.\n");
            return 1;
        }
        printf("Bytes read and written in the file %d\n", bytes);
    }while(bytes == 1000);
    return 0;
}

int read_str1(Url_data url_struct, char *msg, int msg_len) {
    int bytes = read(url_struct.res_sockfd, msg, msg_len);
    printf("%.*s", bytes, msg);
    return bytes;
}



int read_str2(Url_data url_struct, char *buf, int maxlen) {
    int timeout_sec = 1;
    int i = 0;
    fd_set rfds;
    struct timeval tv;

    while (i < maxlen - 1) {
        FD_ZERO(&rfds);
        FD_SET(url_struct.res_sockfd, &rfds);
        tv.tv_sec = timeout_sec;
        tv.tv_usec = 0;

        int sel = select(url_struct.res_sockfd + 1, &rfds, NULL, NULL, &tv);
        if (sel < 0) {
            perror("select");
            return -1;
        } else if (sel == 0) {
            //fprintf(stderr, "read_line: timeout after %d seconds\n", timeout_sec);
            return -2;
        }

        ssize_t n = read(url_struct.res_sockfd, &buf[i], 1);
        if (n < 0) {
            if (errno == EINTR) continue;
            perror("read");
            return -1;
        }
        if (n == 0) { // EOF
            break;
        }

        if (buf[i] == '\n') {
            i++;
            break;
        }
        i++;
    }
    buf[i] = '\0';
    printf("%.*s", i+1, buf);

    return i; // bytes read
}

int read_str(Url_data url_struct, char *buf, int maxlen) {
    int bytes =  read_str2( url_struct, buf, maxlen);
    char discard[maxlen];
    int bytes_discard = bytes; 
    while  (bytes_discard > 0 ){
        bytes_discard = read_str2( url_struct, discard, maxlen);
    }
    return bytes;
}

int check_msg(char *msg, int msg_len, char* code) {
    return 0;
}

int login(Url_data url_struct){
    if(url_struct.user == NULL && url_struct.password == NULL) {
        printf("There is no user nor password, assuming default ones.\n");
        url_struct.user = USER_DEFAULT;
        url_struct.password = PASS_DEFAULT;
    } 
    else if(url_struct.user == NULL) {
        printf("Missing user.\n");
        return 1;
    }
    else if(url_struct.password == NULL) {
        printf("Missing password.\n");
        return 1;
    }

    char buf[256];
    strcpy(buf, "USER ");
    strcat(buf, url_struct.user);
    strcat(buf, "\r\n");
    if (send_str(url_struct, buf, strlen(buf)) == 1) {
        printf("Failed to send user\n");
        return 1;
    }
    

    int msg_size = 1000;
    char msg[msg_size];
    int bytes = read_str(url_struct, msg, msg_size);
    if (bytes == -1) {
        printf("Failed to read message\n");
        return 1;
    }

    if (check_msg(buf, bytes, "331") == 1) {
        printf("Message is not the one expected\n");
        return 1;
    }

    strcpy(buf, "PASS ");
    strcat(buf, url_struct.password);
    strcat(buf, "\r\n");
    if (send_str(url_struct, buf, strlen(buf)) == 1) {
        printf("Failed to send password\n");
        return 1;
    }
    bytes  = read_str(url_struct, msg, msg_size);
    if (bytes == -1) {
        printf("Failed to read message\n");
        return 1;
    }

    if (check_msg(buf, bytes, "230") == 1) {
        printf("Message is not the one expected\n");
        return 1;
    }
    return 0;
}

int parse_pasv(Url_data *url_struct, char *msg, int msg_len) {
    //exemplo: 227 Entering Passive Mode (194,108,117,16,4,15)
    strtok(msg, "(");
    char *p1 = strtok(NULL, ",");
    char *p2 = strtok(NULL, ",");
    char *p3 = strtok(NULL, ",");
    char *p4 = strtok(NULL, ",");
    char *p5 = strtok(NULL, ",");
    char *p6 = strtok(NULL, ")");
    
    url_struct->data_ip = malloc(32);  
    snprintf(url_struct->data_ip, 32, "%s.%s.%s.%s", p1, p2, p3, p4);

    url_struct->data_port = atoi(p5) * 256 + atoi(p6);

    return 0;
}

int main(int argc, char **argv) {

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

    if (getip(&url) == 1){
        printf("Ip not found\n");
        return 1;
    }

    url.res_sockfd = get_sockfd(url.res_ip,  SERVER_PORT);
    if (url.res_sockfd == -1){
        return 1;
    }

    int msg_size = 1000;
    char msg[msg_size];
    int bytes_read = read_str(url, msg, msg_size);
    if (bytes_read == -1) {
        printf("Failed to read message\n");
        return 1;
    }
    
    if (check_msg(msg, bytes_read, "220") == 1) {
        printf("Message is not the one expected\n");
        return 1;
    }
    
    if (login(url) == 1){
        printf("Failed to login\n");
        return 1;
    }


    if (send_str(url, "PASV\r\n", strlen("PASV\r\n")) == 1) {
        printf("Failed to send password\n");
        return 1;
    }
    bytes_read = read_str(url, msg, msg_size);
    if (bytes_read == -1) {
        printf("Failed to read message\n");
        return 1;
    }
    if (check_msg(msg, bytes_read, "227") == 1) {
        printf("Message is not the one expected\n");
        return 1;
    }
    if (parse_pasv(&url, msg, bytes_read) == 1) {
        printf("Pasv message could not be understood\n");
        return 1;
    }



    url.data_sockfd = get_sockfd(url.data_ip, url.data_port);
    if (url.data_sockfd == -1){
        return 1;
    }


    char buf[256];
    strcpy(buf, "RETR ");
    strcat(buf, url.url_path);
    strcat(buf, "\r\n");
    printf("%s", buf);
    if (send_str(url, buf, strlen(buf)) == 1) {
        printf("Failed to send file request\n");
        return 1;
    }
    bytes_read = read_str(url, msg, msg_size);
    if (bytes_read == -1) {
        printf("Failed to read message\n");
        return 1;
    }
    if (check_msg(msg, bytes_read, "150") == 1) {
        printf("Message is not the one expected\n");
        return 1;
    }

    if(read_file(url) == 1){
        printf("Failed to read file\n");
        return 1;
    }


    if (send_str(url, "QUIT\r\n", strlen("QUIT\r\n")) == 1) {
        printf("Failed to send file request\n");
        return 1;
    }

    if (close(url.res_sockfd)<0) {
        perror("close()");
        return 1;
    }

    return 0;
}