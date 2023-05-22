#pragma once

#include <stdlib.h>
#include <arpa/inet.h>

#define HEADER_END_IDENTIFIER "end"
#define COMMAND_EXIT "exit"
char extensions[6][4] = {"txt", "cpp", "c", "py", "tex", "java"};

void logexit(const char *msg);
int addparse(const char *addrstr, const char *portstr, struct sockaddr_storage *storage);
void addrtostr(const struct sockaddr *addr, char *str, size_t strsize);
int server_sockaddr_init(const char *proto, const char *portstr, struct sockaddr_storage *storage);