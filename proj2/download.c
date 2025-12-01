
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>

#include <string.h>

typedef struct {
    char* user;
    char* password;
    char* host;
    char* url_path;
    char** ip;
} Url_data;


int parse_URL(char* url_str, Url_data* url_struct ){
    
    printf("parse_URL not implemented");
    return 1;
}

int check_valid_url( Url_data* url_struct ){
    //se erro dar print
    printf("check_valid_url not implemented");
    return 1;
}

int getip( Url_data* url_struct ){
    printf("getip not implemented");
    return 1;
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
    
    if (check_valid_url(&url) == 1){
        return 1;
    }
    
    if (getip(&url) == 1){ // buscar ip e companhia
        printf("Ip not found");
        return 1;
    }

    
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