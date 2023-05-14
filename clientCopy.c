        
#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFSZ 1024
#define MAX_SUBSTRINGS 3
#define COMMAND_SELECT "select file"
#define COMMAND_SEND "send file"
#define COMMAND_EXIT "exit"
char extensions[6][4] = {"txt", "c", "cpp", "py", "tex", "java"};
char fileToBeSend[BUFSZ];

bool isFileExtensionValid(const char *fileExtension){
    bool isValid = false;
    for(int i = 0; i < sizeof(extensions); i++){
         if (strcmp(fileExtension, extensions[i]) == 0) {
            isValid = true;
            break;
         }
    }
    return isValid;
}

char *getFileExtension(const char *fileName){
    char fileNameCopy[strlen(fileName) + 1];
    strcpy(fileNameCopy, fileName);

    char* token;
    char *lastToken;

    // Divide o nome do arquivo em "tokens" com base no .
    token = strtok((char*) fileNameCopy, ".");
    lastToken = token;
    while (token != NULL) {
        lastToken = token;
        token = strtok(NULL, ".");
    }

    return lastToken;
}

const char *getFileName(const char *command, int comandSize){
    return command + comandSize + 1;
}

void selectFile(const char *command){
    const char *fileName = getFileName(command, strlen(COMMAND_SELECT));
    char *fileExtension = getFileExtension(fileName);
    
    if(strlen(fileName) == 0){
        printf("no file selected!\n");
        return;
    }

    if(!isFileExtensionValid(fileExtension)){
        printf("%s not valid!\n", fileName);
        return;
    }

    if(access(fileName, F_OK) == -1){
        printf("%s does not exist\n", fileName);
        return;
    }
    
    strcpy(fileToBeSend, fileName);
    printf("%s selected\n", fileToBeSend);
    return;
}

void sendFile(const char *command){
    
}

void identifyCommand(char *command){
    // remove \n from command
    command[strcspn(command, "\n")] = 0;
    
    if (strncmp(command, COMMAND_SELECT, strlen(COMMAND_SELECT)) == 0) {
       selectFile(command);
    } else if(strncmp(command, COMMAND_SEND, strlen(COMMAND_SEND)) == 0) {
        printf("enviar\n");
    } else if(strncmp(command, COMMAND_EXIT, strlen(COMMAND_EXIT)) == 0){
        printf("sair\n");
    } else {
        printf("Command invalid\n");
    }
}

void usage(int argc, char **argv){
    printf("usage: %s <server Ip> <server port>", argv[0]);
    printf("example: %s 122.0.0.1 51151", argv[0]);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv){
    if(argc < 3){
        usage(argc, argv);
    }

    struct sockaddr_storage storage;
    if(0 != addparse(argv[1], argv[2], &storage)){
        usage(argc, argv);
    }

    int s = socket(storage.ss_family, SOCK_STREAM, 0);
    if(s == -1){
        logexit("socket");
    }

    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if(connect(s, addr, sizeof(storage)) != 0){
        logexit("connect");
    }

    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);
    printf("connected to %s\n", addrstr);

    // enviando o dado
    char clientMsg[BUFSZ];
    memset(clientMsg, 0, BUFSZ);
    printf("mensagem> ");
    fgets(clientMsg, BUFSZ -1, stdin);
    identifyCommand(clientMsg);

    int clientMsgSize = strlen(clientMsg) + 1;
    int count = send(s, clientMsg, clientMsgSize, 0);
    if(count != clientMsgSize){
        logexit("send");
    }

    // recebendo o dado
    // ele recebe um pouco de bytes por vez e ordena dentro do buffer
    // quando count é 0, todos os bytes chegaram
    memset(clientMsg, 0, BUFSZ);
    unsigned total = 0;
    while(1){
        count = recv(s, clientMsg + total, BUFSZ - total, 0);
        if(count == 0){
            // connection terminated.
            break;
        }
        total += count;
    }

    printf("received %u bytes\n", total);
    puts(clientMsg);

    close(s);
    exit(EXIT_SUCCESS);
}


// ver se é um select file -OK
        //verifico a extensão -OK
            // se n tiver arquivo 
                // logo erro -OK
            //se a extensão for válida
                // eu valido se o arquivo existe -OK
                    // true -> mando mensagem confirmando que foi selecionado 
                        //e eu armazeno
                    // false -> eu logo erro

    // ver se é um send file
        // verifico se eu tenho algum arquivo salvo (já foi selecionado)
        //envio
    // exit
        // termino a conexão