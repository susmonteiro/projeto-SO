#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <ctype.h>
#include <sys/time.h>
#include "fs.h"
#include "sync.h"

//==========
//Constantes
//==========
#define MAX_COMMANDS 10 //vetor de comandos tem no maximo 10 comandos num determinado momento
#define INITVAL_COMMAND_READER 0 //inicialmente o vetor de comandos esta vazio logo nao ha comandos para consumir 
#define MAX_INPUT_SIZE 100
#define MILLION 1000000
#define N_ARGC 5        // numero de argumentos valido
#define COMMAND_NULL -1 //Comando inexistente
#define END_COMMAND 'x' //Comando criado para terminar as threads

//=====================
//Prototipos principais
//=====================
void applyCommands();
void processInput(const char *pwd);

#endif /* CONSTANTS_H */