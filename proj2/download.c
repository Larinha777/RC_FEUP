
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
    
    int sockfd;
    struct sockaddr_in server_addr;
    
    /*server address handling*/
    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(url.ip);    /*32 bit Internet address network byte ordered*/
    server_addr.sin_port = htons(SERVER_PORT);        /*server TCP port must be network byte ordered */
    
    /*open a TCP socket*/
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        return 1;
    }
    
    /*connect to the server*/
    if (connect(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        perror("connect()");
        return 1;
    }
    
    if (close(sockfd)<0) {
        perror("close()");
        return 1;
    }
    
    return 1;
    
    /*open a TCP socket*/
    
    /*connect to the server*/
    

    char *buf; // create the request
    // verify user / password
        // login
    // qlqr cena passivo

    


    // verificar host / path
        // pedir ficheiro
        // verificar se está correto
            // criar ficheiro com esse conteudo

            

    return 0;
}