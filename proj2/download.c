
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

/* If the url is a valid one, it works, if not, it doesnt*/
int parse_URL(char* url_str, Url_data* url_struct ){
   
    /* url does not start with ftp */
    if (strncmp("ftp", url_str, 3)){ 
        printf("Cannot handle cases where the url does not start with ftp\n");
        return 1;
    }

    //printf("full str: %s\n",url_str);

    char* noFTP = strstr(url_str, "//") + 2;
    //printf("noFTP str: %s\n",noFTP);

    char* part1 = strtok(noFTP, "@");
    char* part2 = strtok(NULL, "");

    if (part2 == NULL){ //no login
        url_struct->user = "";
        url_struct->password = "";
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

int check_valid_url( Url_data* url_struct ){
    //se erro dar print
    printf("Function check_valid_url not implemented\n");
    return 1;
}

int getip( Url_data* url_struct ){
    struct hostent *h = gethostbyname(url_struct->host);
    if (h == NULL) {
        herror("gethostbyname");
        printf("Failed to resolve host.\n");
        return 1;
    }
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
    
    
    if (check_valid_url(&url) == 1){
        return 1;
    }
    
    if (getip(&url) == 1){ // buscar ip e companhia
        printf("Ip not found\n");
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