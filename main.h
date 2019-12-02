#ifndef MAIN_H
#define MAIN_H


#include <getopt.h>
#include <ctype.h>
#include <unistd.h>
#include <signal.h>

#include "erro.h"
#include "fs.h"
#include "socket.h"
#include "sync.h"
#include "lib/inodes.h"
#include "constants.h"

//==========
//Constantes
//=========+

#define MAX_INPUT_SIZE 100
#define MAX_ARGS_INPUTS 2
#define MILLION 1000000
#define N_ARGC 4            //Numero de argumentos que o programa deve receber
#define COMMAND_NULL -1     //Comando inexistente
#define MAX_FILES_OPENED 5  //Tamanho da tabela de ficheiros abertos para cliente
#define FD_EMPTY -1         //Assinala uma posicao vazia da tabela de ficheiros de cada cliente

#define CAN_READ(file_permission) file_permission & READ
#define CAN_WRITE(file_permission) file_permission & WRITE 
#define OWNER_PERMISSION(permission) atoi(permission)/10
#define OTHER_PERMISSION(permission) atoi(vec[1])%10
//verifica se, caso o user seja o owner, este tem permissao para abrir o ficheiro no "mode" especificado 
#define OWNER_HAS_PERMISSION(user_uid, owner_uid, mode, owner_permission) user_uid==owner_uid && (mode&owner_permission) == mode
//verifica se, caso o user nao seja o owner, este tem permissao para abrir o ficheiro no "mode" especificado 
#define OTHER_HAS_PERMISSION(user_uid, owner_uid, mode, other_permission) user_uid!=owner_uid && (mode&other_permission) == mode

typedef struct  {
    int iNumber;
    permission open_as; 
}   tecnicofs_fd;

//=====================
//Prototipos principais
//=====================
void *clientSession(void * socketfd);
void endServer();
void feedback(int sockfd, int msg);
static void displayUsage (const char* appName);

#endif /* MAIN_H */