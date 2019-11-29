#include "tecnicofs-client-api.h"

#define CHECK_CONNECTED() if (fd == NOT_CONNECTED) return TECNICOFS_ERROR_NO_OPEN_SESSION
#define CHECK_NOT_CONNECTED() if(fd != NOT_CONNECTED) return TECNICOFS_ERROR_OPEN_SESSION

int inSession = 0;
int fd = NOT_CONNECTED;

void sysError(const char* msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}


/*  Funcao utilizada pelo cliente para criar um novo ficheiro no servidor
*
*   Representacao do comando para criar um ficheiro:
*       "c 'filename' 'permissions'\0"
*   Retorno:
*       - Erro se nao existir conexao (TECNICOFS_ERROR_NO_OPEN_SESSION)
*       - Erro de preexistencia de ficheiro (TECNICOFS_ERROR_FILE_ALREADY_EXISTS)
*       - Caso contrario (SUCESSO) 
*/
int tfsCreate(char *filename, permission ownerPermissions, permission othersPermissions){
    int size = strlen(filename) + PADDING_COMMAND_C;
    char command[size];
    int answer;

    CHECK_CONNECTED();

    sprintf(command, "%c %s %d%d", CREATE_COMMAND, filename, ownerPermissions, othersPermissions);
    
    printf("\ncommand:%s\n", command);

    // escreve o comando
    if (write(fd, command, size) != size) sysError("tfsCreate(write)");
    // le resposta ao comando
    if (read(fd, &answer, INT_SIZE) != INT_SIZE) sysError("tfsCreate(read)");
    
    printf("answer:%d\n", answer);

    if (answer == ALREADY_EXISTS) return TECNICOFS_ERROR_FILE_ALREADY_EXISTS; 
    
    return SUCCESS;
}

int tfsDelete(char *filename){
    int size = strlen(filename) + PADDING_COMMAND_D;
    char command[size];
    int answer;

    CHECK_CONNECTED();

    sprintf(command, "%c %s", DELETE_COMMAND, filename);

    printf("\ncommand:%s\n", command);

    // escreve o comando
    if (write(fd, command, size) != size) sysError("tfsDelete(write)");
    // le resposta ao comando
    if (read(fd, &answer, INT_SIZE) != INT_SIZE) sysError("tfsDelete(read)");

    printf("answer:%d\n", answer);

    if (answer == DOESNT_EXIST) return TECNICOFS_ERROR_FILE_NOT_FOUND; 

    return SUCCESS;
}

int tfsRename(char *filenameOld, char *filenameNew){
    int size = strlen(filenameOld) + strlen(filenameNew) + PADDING_COMMAND_R;
    char command[size];
    int answer;

    CHECK_CONNECTED();


    sprintf(command, "%c %s %s", RENAME_COMMAND, filenameOld, filenameNew);

    printf("\ncommand:%s\n", command);

    // escreve o comando
    if (write(fd, command, size) != size) sysError("tfsRename(write)");
    // le resposta ao comando
    if (read(fd, &answer, INT_SIZE) != INT_SIZE) sysError("tfsRename(read)");

    printf("answer:%d\n", answer);

    if (answer == DOESNT_EXIST) return TECNICOFS_ERROR_FILE_NOT_FOUND; 
    if (answer == ALREADY_EXISTS) return TECNICOFS_ERROR_FILE_ALREADY_EXISTS; 
    if (answer == PERMISSION_DENIED) return TECNICOFS_ERROR_PERMISSION_DENIED; 

    return SUCCESS;
}


int tfsOpen(char *filename, permission mode){
    int size = strlen(filename) + PADDING_COMMAND_O;
    char command[size];
    int answer;

    CHECK_CONNECTED();

    sprintf(command, "%c %s %d", OPEN_COMMAND, filename, mode);
    
    printf("\ncommand:%s\n", command);

    // escreve o comando
    if (write(fd, command, size) != size) sysError("tfsOpen(write)");
    // le resposta ao comando
    if (read(fd, &answer, INT_SIZE) != INT_SIZE) sysError("tfsOpen(read)");
    printf("answer:%d\n", answer);

    if (answer == DOESNT_EXIST) return TECNICOFS_ERROR_FILE_NOT_FOUND; 
    if (answer == PERMISSION_DENIED) return TECNICOFS_ERROR_PERMISSION_DENIED;
    if (answer == MAX_OPENED_FILES) return TECNICOFS_ERROR_MAXED_OPEN_FILES; 
        
    return answer;
}

int tfsClose(int fd){return SUCCESS;}
int tfsRead(int fd, char *buffer, int len){return SUCCESS;}
int tfsWrite(int fd, char *buffer, int len){return SUCCESS;}

int tfsMount(char * address){
    int servlen;
    struct sockaddr_un serv_addr;

    CHECK_NOT_CONNECTED();

    if((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
        sysError("tfsMount(socket)");


    bzero((char*) &serv_addr, sizeof(serv_addr));
    serv_addr.sun_family = AF_UNIX;
    strncpy(serv_addr.sun_path, address, sizeof(serv_addr.sun_path)-1);
    servlen = strlen(serv_addr.sun_path) + sizeof(serv_addr.sun_family);
    
    if(connect(fd, (struct sockaddr*) &serv_addr, servlen) < 0)
        return TECNICOFS_ERROR_CONNECTION_ERROR;

    return SUCCESS;
}

int tfsUnmount(){
    int size = CHAR_SIZE + PADDING_COMMAND_Z; //compensar pelo '\0'
    char command[size];
    
    CHECK_CONNECTED();

    sprintf(command, "%c", END_COMMAND);

    // escreve o comando
    if (write(fd, command, size) != size) sysError("tfsUnmount(write)");
    // le resposta ao comando
    if(close(fd) == -1) sysError("tfsUnmount(close)");

    fd = NOT_CONNECTED;

    return SUCCESS;
}
