#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>

#define BUFSZ 1024

void usage(int argc, char **argv){
   printf("usage: %s <v4|v6> <server port>\n", argv[0]);
   printf("example: %s v4 51551\n", argv[0]);
   exit(EXIT_FAILURE);
}

void closeConnection(int csock){
    char *message = "connection close\n";
    size_t count = send(csock, message, strlen(message) + 1, 0);
    if(count != strlen(message) + 1){
        logexit("send");
    }
    close(csock);
    exit(EXIT_SUCCESS);
}

const char *getFileName(const char *message){
    const char* start_pos = strstr(message, HEADER_CONTENT_IDENTIFIER);
    if (start_pos == NULL) {
        printf("Erro\n");
    }
    
    size_t start_index = start_pos - message;
    char* first_part = malloc(start_index + 1);
    if (first_part == NULL) {
        printf("Erro\n");
    }
    
    strncpy(first_part, message, start_index); // copiar a parte anterior da string
    first_part[start_index] = '\0'; // garantir que a string termina com null
    
    return first_part;
}


FILE *createFile(const char *fileName){
    FILE *file = fopen(fileName, "w");
    if (file == NULL) {
        printf("error receiving file %s\n", fileName);
    }

    return file;
}


const char *readFile(char *message){
    const char *fileName = getFileName(message);
    
    // 17 é o tamanho da mensagem de overwritten sem contar o nome do arquivo
    char *answer = malloc(17 + strlen(fileName));

    // se arquivo não existir
    if(access(fileName, F_OK) == -1){
        sprintf(answer, "file %s received\n", fileName);
    }else{
        sprintf(answer, "file %s overwritten", fileName);
    }
    
    FILE *file = createFile(fileName);

    fprintf(file, "Texto a ser escrito no arquivo\n");
    // fecha o arquivo
    fclose(file);
    return answer;
}

void identifyCommand(char *message, int csock){
    if (strncmp(message, COMMAND_EXIT, strlen(COMMAND_EXIT)) == 0) {
        closeConnection(csock);
    } else {
        readFile(message);
    }
}

void receiveMessage(int csock, char *caddrstr){
    char message[BUFSZ];
    memset(message, 0, BUFSZ);

    size_t count = recv(csock, message, BUFSZ - 1, 0);
    identifyCommand(message, csock);

    // recebi um arquivo
        //verifico se o arquivo já foi recebido
            //true -> 
                //sobreescrever
                // confirmar sobreescrita
        // false ->
                // armazenar
                // confirmar recebimento
    
    const char *answer = readFile(message);
    count = send(csock, answer, strlen(answer) + 1, 0);
    if(count != strlen(answer) + 1){
        logexit("send");
    }
    close(csock);
}

int main(int argc, char **argv){
    if(argc < 3){
        usage(argc, argv);
    }

    struct sockaddr_storage storage;
    if(0 != server_sockaddr_init(argv[1], argv[2], &storage)){
        usage(argc, argv);
    }

    int s = socket(storage.ss_family, SOCK_STREAM, 0);
    if(s == -1){
        logexit("socket");
    }

    int enable = 1;
    if(setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) != 0){
        logexit("setsockopt");
    }
    
    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if(bind(s, addr, sizeof(storage)) != 0){
        logexit("bind");
    }

    if(listen(s, 10) != 0){
        logexit("listen");
    }

    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);
    printf("bound to %s, waiting connections\n", addrstr);

    while(1){
        struct sockaddr_storage cstorage;
        struct sockaddr *caddr = (struct sockaddr *)(&cstorage);
        socklen_t caddrlen = sizeof(storage);

        int csock = accept(s, caddr, &caddrlen);
        if(csock == -1){
            logexit("accept");
        }

        char caddrstr[BUFSZ];
        addrtostr(caddr, caddrstr, BUFSZ);
        printf("[log] connection from %s\n", caddrstr);

        receiveMessage(csock, caddrstr);
    }
    exit(EXIT_SUCCESS);
}