#include "tecnicofs-client-api.h"

#define CHECK_CONNECTED() if (sockfd == NOT_CONNECTED) return TECNICOFS_ERROR_NO_OPEN_SESSION
#define CHECK_NOT_CONNECTED() if(sockfd != NOT_CONNECTED) return TECNICOFS_ERROR_OPEN_SESSION

int inSession = 0;
int sockfd = NOT_CONNECTED;

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

    if (sprintf(command, "%c %s %d%d", CREATE_COMMAND, filename, ownerPermissions, othersPermissions) != size-1) sysError("tfsCreate(sprintf)");
    
    printf("\ncommand:%s\n", command);

    // escreve o comando
    if (write(sockfd, command, size) != size) sysError("tfsCreate(write)");
    // le resposta ao comando
    if (read(sockfd, &answer, INT_SIZE) != INT_SIZE) sysError("tfsCreate(read)");
    
    printf("answer:%d\n", answer);

    if (answer == ALREADY_EXISTS) return TECNICOFS_ERROR_FILE_ALREADY_EXISTS; 
    
    return SUCCESS;
}

int tfsDelete(char *filename){
    int size = strlen(filename) + PADDING_COMMAND_D;
    char command[size];
    int answer;

    CHECK_CONNECTED();

    if (sprintf(command, "%c %s", DELETE_COMMAND, filename) != size-1) sysError("tfsDelete(sprintf)");

    printf("\ncommand:%s\n", command);

    // escreve o comando
    if (write(sockfd, command, size) != size) sysError("tfsDelete(write)");
    // le resposta ao comando
    if (read(sockfd, &answer, INT_SIZE) != INT_SIZE) sysError("tfsDelete(read)");

    printf("answer:%d\n", answer);

    if (answer == DOESNT_EXIST) return TECNICOFS_ERROR_FILE_NOT_FOUND; 
    if (answer == FILE_IS_OPENED) return TECNICOFS_ERROR_FILE_IS_OPEN;
    return SUCCESS;
}

int tfsRename(char *filenameOld, char *filenameNew){
    int size = strlen(filenameOld) + strlen(filenameNew) + PADDING_COMMAND_R;
    char command[size];
    int answer;

    CHECK_CONNECTED();

    if(!filenameNew || !filenameOld) return TECNICOFS_ERROR_OTHER;

    if (sprintf(command, "%c %s %s", RENAME_COMMAND, filenameOld, filenameNew) != size-1) sysError("tfsRename(sprintf)");

    printf("\ncommand:%s\n", command);

    // escreve o comando
    if (write(sockfd, command, size) != size) sysError("tfsRename(write)");
    // le resposta ao comando
    if (read(sockfd, &answer, INT_SIZE) != INT_SIZE) sysError("tfsRename(read)");

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

    if (sprintf(command, "%c %s %d", OPEN_COMMAND, filename, mode) != size-1) sysError("tfsOpen(sprintf)");
    
    printf("\ncommand:%s\n", command);

    // escreve o comando
    if (write(sockfd, command, size) != size) sysError("tfsOpen(write)");
    // le resposta ao comando
    if (read(sockfd, &answer, INT_SIZE) != INT_SIZE) sysError("tfsOpen(read)");
    printf("answer:%d\n", answer);

    if (answer == DOESNT_EXIST) return TECNICOFS_ERROR_FILE_NOT_FOUND; 
    if (answer == PERMISSION_DENIED) return TECNICOFS_ERROR_PERMISSION_DENIED;
    if (answer == MAX_OPENED_FILES) return TECNICOFS_ERROR_MAXED_OPEN_FILES; 
        
    return answer;
}

int tfsClose(int fd){
    int size_fd = snprintf(NULL, 0, "%d", fd);
    int size = size_fd + PADDING_COMMAND_X;
    char command[size];
    int answer;

    CHECK_CONNECTED();

    if (sprintf(command, "%c %d", CLOSE_COMMAND, fd) != size-1) sysError("tfsClose(sprintf)");

    printf("\ncommaSnd:%s\n", command);

    // escreve o comando
    if (write(sockfd, command, size) != size) sysError("tfsClose(write)");
    // le resposta ao comando
    if (read(sockfd, &answer, INT_SIZE) != INT_SIZE) sysError("tfsClose(read)");

    printf("answer:%d\n", answer);

    if (answer == NOT_OPENED) return TECNICOFS_ERROR_FILE_NOT_OPEN; 
    if (answer == INDEX_OUT_OF_RANGE) return TECNICOFS_ERROR_OTHER;

    return SUCCESS;
}


int tfsRead(int fd, char *buffer, int len){
    // int actual_buffer_len = len - 1; //uma string comeca na posicao 0
    int fd_size = snprintf(NULL, 0, "%d", fd); 
    // int len_size = snprintf(NULL, 0, "%d", actual_buffer_len); 
    int len_size = snprintf(NULL, 0, "%d", len); 

    int size = fd_size + len_size + PADDING_COMMAND_L;
    char command[size];
    int answer;


    //check enough space in buffer
    if (len < 0) return TECNICOFS_ERROR_OTHER; 
    // if (sizeof(buffer) <= len) {
    //     printf("\n\t%ld", sizeof(buffer));
    //     return TECNICOFS_ERROR_OTHER;
    // }

    CHECK_CONNECTED();

    if (sprintf(command, "%c %d %d", READ_COMMAND, fd, len) != size-1) sysError("tfsRead(sprintf)");

    printf("\ncommaSnd:%s\n", command);

    // escreve o comando
    if (write(sockfd, command, size) != size) sysError("tfsRead(write)");
    // le resposta ao comando
    if (read(sockfd, &answer, INT_SIZE) != INT_SIZE) sysError("tfsRead(readAnswer)");

    printf("answer:%d\n", answer);

    if(answer >= 0) //se positivo igual a numero de caracteres lidos excepto o \0 dai o answer+1
        if(read(sockfd, buffer, answer+1) != answer+1) sysError("tfsRead(readBuffer)");


    if (answer == NOT_OPENED) return TECNICOFS_ERROR_FILE_NOT_OPEN; 
    if (answer == INDEX_OUT_OF_RANGE) return TECNICOFS_ERROR_OTHER;
    if (answer == PERMISSION_DENIED) return TECNICOFS_ERROR_INVALID_MODE;
    
    return answer; 
}

int tfsWrite(int fd, char *buffer, int len){
    int fd_size = snprintf(NULL, 0, "%d", fd); 
    int size = fd_size + strlen(buffer) + PADDING_COMMAND_W;
    char command[size];
    int answer;

    //check enough space in buffer
    if (len < 0) return TECNICOFS_ERROR_OTHER; 
    if (strlen(buffer) > len) return TECNICOFS_ERROR_OTHER;

    CHECK_CONNECTED();

    //  PASSAR SO BUFFER ATE LEN????

    if (sprintf(command, "%c %d %s", WRITE_COMMAND, fd, buffer) != size-1) sysError("tfsWrite(sprintf)");

    printf("\ncommaSnd:%s\n", command);

    // escreve o comando
    if (write(sockfd, command, size) != size) sysError("tfsWrite(write)");
    // le resposta ao comando
    if (read(sockfd, &answer, INT_SIZE) != INT_SIZE) sysError("tfsWrite(read)");

    printf("answer:%d\n", answer);

    if (answer == NOT_OPENED) return TECNICOFS_ERROR_FILE_NOT_OPEN; 
    if (answer == INDEX_OUT_OF_RANGE) return TECNICOFS_ERROR_OTHER;


    return answer; 

}

int tfsMount(char * address){
    int servlen;
    struct sockaddr_un serv_addr;

    CHECK_NOT_CONNECTED();

    if((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
        sysError("tfsMount(socket)");


    bzero((char*) &serv_addr, sizeof(serv_addr));
    serv_addr.sun_family = AF_UNIX;
    strncpy(serv_addr.sun_path, address, sizeof(serv_addr.sun_path)-1);
    servlen = strlen(serv_addr.sun_path) + sizeof(serv_addr.sun_family);
    
    if(connect(sockfd, (struct sockaddr*) &serv_addr, servlen) < 0)
        return TECNICOFS_ERROR_CONNECTION_ERROR;

    return SUCCESS;
}

int tfsUnmount(){
    int size = CHAR_SIZE + PADDING_COMMAND_Z; //compensar pelo '\0'
    char command[size];
    
    CHECK_CONNECTED();

    if (sprintf(command, "%c", END_COMMAND) != size-1) sysError("tfsUnmount(sprintf)");
    
    // escreve o comando
    if (write(sockfd, command, size) != size) sysError("tfsUnmount(write)");
    // le resposta ao comando
    if(close(sockfd) == -1) sysError("tfsUnmount(close)");

    sockfd = NOT_CONNECTED;

    return SUCCESS;
}
