#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <regex.h>

#define BUFSZ 1024
#define MAX_SUBSTRINGS 3
#define COMMAND_SELECT "select file"
#define COMMAND_SEND "send file"

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

void replaceNullTerminator(char* buffer) {

    int lastCharIndex = strlen(buffer) - 1;

    // Verifica se o último caractere é '\0'
    if (buffer[lastCharIndex] == '\0') {
        buffer[lastCharIndex] = '\\'; // Barra invertida
    }
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
        printf("%s do not exist\n", fileName);
        return;
    }
    
    strcpy(fileToBeSend, fileName);
    printf("%s selected\n", fileToBeSend);
    return;
}

void sendFile(int s){

    if(strlen(fileToBeSend) == 0){
        printf("no file selected!\n");
        return;
    }
    
    FILE *file;
    file = fopen(fileToBeSend, "r");

    // verifica se o arquivo foi aberto com sucesso
    if (file == NULL) {
        printf("%s not valid!\n", fileToBeSend);
        return;
    }

    // obtendo o tamanho do arquivo
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *buffer = malloc(size);
    if (buffer == NULL) {
        printf("Erro ao alocar memória para o buffer\n");
        return;
    }

    // lê o conteúdo do arquivo para o buffer
    if (fread(buffer, 1, size, file) != size) {
        printf("Erro ao ler o arquivo %s\n", fileToBeSend);
        free(buffer);
        fclose(file);
        return;
    }

    replaceNullTerminator(buffer);
    // concatena o nome do arquivo\start\content\end
    long contentSize = strlen(fileToBeSend) + strlen(buffer) + strlen(HEADER_END_IDENTIFIER);
    char *content = malloc(contentSize);
    sprintf(content, "%s%s%s", fileToBeSend, buffer, HEADER_END_IDENTIFIER);
    
    int totalSent = 0;
    while (totalSent < contentSize) {
        int count = send(s, content + totalSent, contentSize - totalSent, 0);
        if (count == -1) {
            printf("Erro ao enviar o arquivo %s\n", fileToBeSend);
            free(buffer);
            fclose(file);
            return;
        }
        totalSent += count;
    }

    free(buffer);
    free(content);
    fclose(file);

    char answer[BUFSZ]; 
    ssize_t count = recv(s, answer, BUFSZ - 1, 0);
    if(count < 0){
        logexit("Ocorreu um erro ou a conexão foi encerrada\n");
    }
    puts(answer);
    return;
}

void exitConnection(int s){
    int clientMsgSize = strlen(COMMAND_EXIT) + 1;
    int count = send(s, COMMAND_EXIT, clientMsgSize, 0);
    if(count != clientMsgSize){
        logexit("send");
    }
    
    char answer[BUFSZ]; 
    count = recv(s, answer, BUFSZ - 1, 0);
    if(count < 0){
        logexit("Ocorreu um erro ou a conexão foi encerrada\n");
    }
    puts(answer);
    close(s);
    exit(EXIT_SUCCESS);
}

void identifyCommand(char *command, int s){
    // remove \n from command
    command[strcspn(command, "\n")] = 0;
    
    if (strncmp(command, COMMAND_SELECT, strlen(COMMAND_SELECT)) == 0) {
        selectFile(command);
    } else if(strncmp(command, COMMAND_SEND, strlen(COMMAND_SEND)) == 0) {
        sendFile(s);
    } else {
        exitConnection(s);
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
    //memset(addrstr, 0, BUFSZ); // avaliar se precisa
    addrtostr(addr, addrstr, BUFSZ);
    printf("connected to %s\n", addrstr);

    // enviando o dado
    while(1){
        char clientMsg[BUFSZ];
        memset(clientMsg, 0, BUFSZ);
        // verificar se precisa limpar buffers
        printf("mensagem> ");

        fgets(clientMsg, BUFSZ -1, stdin);
        identifyCommand(clientMsg, s);
        //avaliar se precisa limpar buffer
    }
    close(s);
    exit(EXIT_SUCCESS);
}