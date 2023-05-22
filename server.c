#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>

#define BUFSZ 500

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

FILE *createFile(const char *fileName){
    FILE *file = fopen(fileName, "w");
    if (file == NULL) {
        printf("error receiving file %s\n", fileName);
    }

    return file;
}

int searchForExtension(const char* buffer, char* fileName, size_t index){
    for(int i = 0; i < 6; i++){
        int size = index + strlen(extensions[i]) + 1;
        char fileNameWithExtension[BUFSZ];
        strncpy(fileNameWithExtension, buffer, size);
        fileNameWithExtension[size] = '\0';
        
        char *content = malloc(size);
        sprintf(content, "%s.%s", fileName, extensions[i]);
        if(strcmp(fileNameWithExtension, content) == 0){
            return size;
        }
        free(content);
    }
    return index;
}

char *processMessage(char *message){
    // obtem o nome do arquivo com a extensão
    const char* extension = strchr(message, '.');
    size_t index = extension - message;

    char *fileName = malloc(index + 1);
    strncpy(fileName, message, index);
    (fileName)[index] = '\0';
    
    int newIndex = searchForExtension(message, fileName, index);
    char *newFileName = malloc(newIndex + 1);
    strncpy(newFileName, message, newIndex);
    (newFileName)[newIndex] = '\0';

    // obtém o conteúdo do arquivo
    char *content = malloc(strlen(message + newIndex) + 1);
    strcpy(content, message + newIndex);

    // 17 é o tamanho da mensagem de overwritten sem contar o nome do arquivo
    char *answer = malloc(17 + strlen(newFileName));

    // se arquivo não existir
    if(access(newFileName, F_OK) == -1){
        sprintf(answer, "file %s received\n", newFileName);
    }else{
        sprintf(answer, "file %s overwritten", newFileName);
    }
    
    FILE *file = createFile(newFileName);

    int finalContentSize = strlen(content) - strlen(HEADER_END_IDENTIFIER) + 1;
    char finalContent[finalContentSize];
    strncpy(finalContent, content, finalContentSize);
    finalContent[finalContentSize - 1] = '\0';

    fprintf(file, "%s", finalContent);

    fclose(file);
    return answer;
}

void identifyCommand(char *message, int csock){
    if (strncmp(message, COMMAND_EXIT, strlen(COMMAND_EXIT)) == 0) {
        closeConnection(csock);
    } else {
        char *answer = processMessage(message);

        int totalSent = 0;
        int contentSize = strlen(answer);
        while (totalSent < contentSize) {
            int count = send(csock, answer + totalSent, contentSize - totalSent, 0);
            if (count == -1) {
                printf("Erro ao enviar a resposta\n");
                free(answer);
                return;
            }
            totalSent += count;
        }
        free(answer);
    }
}

void receiveMessage(int csock, char *caddrstr){
    char message[BUFSZ];
    memset(message, 0, BUFSZ);

    recv(csock, message, BUFSZ - 1, 0);
    identifyCommand(message, csock);
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

        while(1){
            receiveMessage(csock, caddrstr);
        }
        
        close(csock);
    }
    
    exit(EXIT_SUCCESS);
}


/*
TODO:
    CONSEGUIR USAR OUTROS COMANDOS DEPOIS DO SEND
TESTAR:
    FUNCIONAMENNTO IPV4
    ENVIO DE ARQUIVO Q N EXISTE NO SERVIDOR
*/