#ifndef TECNICOFS_SERVER_API_H
#define TECNICOFS_SERVER_API_H

#include <sys/types.h>
#include <stdlib.h>

#include "fs.h"
#include "../common/tecnicofs-api-constants.h"
#include "../common/constants.h"
#include "lib/inodes.h"
#include "sync.h"
#include "socket.h"

#define MAX_ARGS_INPUTS 2
#define MAX_INPUT_SIZE 100
#define FD_EMPTY -1         //Assinala uma posicao vazia da tabela de ficheiros de cada cliente
#define MAX_FILES_OPENED 5  //Tamanho da tabela de ficheiros abertos para cliente


//======
//Macros
//======
#define CAN_READ(file_permission) file_permission & READ
#define CAN_WRITE(file_permission) file_permission & WRITE 
#define OWNER_PERMISSION(permission) atoi(permission)/10
#define OTHER_PERMISSION(permission) atoi(vec[1])%10
//verifica se, caso o user seja o owner, este tem permissao para abrir o ficheiro no "mode" especificado 
#define OWNER_HAS_PERMISSION(user_uid, owner_uid, mode, owner_permission) user_uid==owner_uid && (mode&owner_permission) == mode
//verifica se, caso o user nao seja o owner, este tem permissao para abrir o ficheiro no "mode" especificado 
#define OTHER_HAS_PERMISSION(user_uid, owner_uid, mode, other_permission) user_uid!=owner_uid && (mode&other_permission) == mode

//=========
//Estrutura
//=========
typedef struct  {
    int iNumber;
    permission open_as; 
}   tecnicofs_fd;

//==============================================
//Funcoes relativas ao vetor de fcheiros abertos
//==============================================
void initOpenedFilesCounter();
void clearClientOpenedFilesCounter(tecnicofs_fd *file_tab);
void freeOpenedFilesCounter();
void parseCommand(int socketfd, char* command, char vec[MAX_ARGS_INPUTS][MAX_INPUT_SIZE]);

//===================
// Rotina de comandos
//===================
int commandCreate(char vec[MAX_ARGS_INPUTS][MAX_INPUT_SIZE], uid_t uid);
int commandDelete(char vec[MAX_ARGS_INPUTS][MAX_INPUT_SIZE], uid_t uid);
int commandRename(char vec[MAX_ARGS_INPUTS][MAX_INPUT_SIZE], uid_t uid);
int commandOpen(char vec[MAX_ARGS_INPUTS][MAX_INPUT_SIZE], uid_t uid, tecnicofs_fd *file_tab);
int commandClose(char vec[MAX_ARGS_INPUTS][MAX_INPUT_SIZE], tecnicofs_fd *file_tab);
void commandRead(int fd, char vec[MAX_ARGS_INPUTS][MAX_INPUT_SIZE], tecnicofs_fd *file_tab);
int commandWrite(int fd, char vec[MAX_ARGS_INPUTS][MAX_INPUT_SIZE], tecnicofs_fd *file_tab);

#endif /* TECNICOFS_SERVER_API_H */