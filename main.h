#ifndef MAIN_H
#define MAIN_H


#include <getopt.h>
#include <ctype.h>
#include <unistd.h>
#include <signal.h>

#include "erro.h"
#include "socket.h"
#include "sync.h"
#include "lib/inodes.h"
#include "constants.h"
#include "tecnicofs-server-api.h"

//==========
//Constantes
//==========
#define MILLION 1000000
#define N_ARGC 4            //Numero de argumentos que o programa deve receber
#define COMMAND_NULL -1     //Comando inexistente


//=====================
//Prototipos principais
//=====================
static void displayUsage (const char* appName);
static void parseArgs (long argc, char* const argv[]);

//===========================
// Handler direto de clientes
//===========================
void *clientSession(void * socketfd);

//=======
//Runtime
//=======
int startRuntime();
void endRuntime();
// Hashtab
void initHashTable(int size);
void freeHashTab(int size);
// Signals
void initSignal();
void blockSigInt();
void unblockSigInt();
void endServer();
// Misc
float time_taken(struct timeval start, struct timeval end);


#endif /* MAIN_H */