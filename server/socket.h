#ifndef SOCKET_H
#define SOCKET_H

#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <limits.h>
#include <unistd.h>

#include "fs.h"
#include "sync.h"
#include "../common/erro.h"
#include "../common/constants.h"

/* SOMAXCONN e' o numero maximo de ligacoes que o servidor pode ter simultaneamente.
Optamos por definir que este corresponde tambem ao numero maximo de ligacoes 
(e consequentemente threads escravas) que uma execucao do programa permite */
#define MAXCONNECTIONS SOMAXCONN    

int socketInit();
void readCommandfromSocket(int fd, char* buffer);
uid_t getSockUID(int fd);
void processClient(int sockfd);
void feedback(int sockfd, int msg);
void closeSocket(int fd);


#endif /* SOCKET_H */