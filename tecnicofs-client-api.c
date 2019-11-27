#include "tecnicofs-client-api.h"


int inSession = 0;
int socketfd = NOT_CONNECTED;

void sysError(const char* msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}



int tfsCreate(char *filename, permission ownerPermissions, permission othersPermissions){
    int size = strlen(filename) + PADDING_COMMAND_C;
    char command[size];
    int answer;

    if (socketfd == NOT_CONNECTED) return TECNICOFS_ERROR_NO_OPEN_SESSION;

    sprintf(command, "%c %s %d%d", 'c', filename, ownerPermissions, othersPermissions);
    
    printf("\ncommand:%s\n", command);

    if (send(socketfd, command, size, 0) != size) sysError("tfsCreate(send)");
    if (recv(socketfd, &answer, INT_SIZE, 0) != INT_SIZE) sysError("tfsCreate(recv)");
    
    printf("answer:%d\n", answer);

    if (answer == ALREADY_EXISTS) return TECNICOFS_ERROR_FILE_ALREADY_EXISTS; 
    
    return SUCCESS;
}

int tfsDelete(char *filename){
    int size = strlen(filename) + PADDING_COMMAND_D;
    char command[size];
    int answer;

    sprintf(command, "%c %s", 'd', filename);

    printf("\ncommand:%s\n", command);

    if (send(socketfd, command, size, 0) != size) sysError("tfsDelete(send)");
    if (recv(socketfd, &answer, INT_SIZE, 0) != INT_SIZE) sysError("tfsDelete(recv)");

    printf("answer:%d\n", answer);

    if (answer == DOESNT_EXIST) return TECNICOFS_ERROR_FILE_NOT_FOUND; 

    return SUCCESS;
}

int tfsRename(char *filenameOld, char *filenameNew){
    int size = strlen(filenameOld) + strlen(filenameNew) + PADDING_COMMAND_R;
    char command[size];
    int answer;

    sprintf(command, "%c %s %s", 'r', filenameOld, filenameNew);

    printf("\ncommand:%s\n", command);

    if (send(socketfd, command, size, 0) != size) sysError("tfsRename(send)");
    if (recv(socketfd, &answer, INT_SIZE, 0) != INT_SIZE) sysError("tfsRename(recv)");

    printf("answer:%d\n", answer);

    if (answer == DOESNT_EXIST) return TECNICOFS_ERROR_FILE_NOT_FOUND; 
    if (answer == ALREADY_EXISTS) return TECNICOFS_ERROR_FILE_ALREADY_EXISTS; 
    if (answer == PERMISSION_DENIED) return TECNICOFS_ERROR_PERMISSION_DENIED; 

    return SUCCESS;
}

int tfsOpen(char *filename, permission mode){return SUCCESS;}
int tfsClose(int fd){return SUCCESS;}
int tfsRead(int fd, char *buffer, int len){return SUCCESS;}
int tfsWrite(int fd, char *buffer, int len){return SUCCESS;}

int tfsMount(char * address){
    int servlen;
    struct sockaddr_un serv_addr;
    uid_t uid = getuid();

    if(socketfd != NOT_CONNECTED) 
        return TECNICOFS_ERROR_OPEN_SESSION;

    if((socketfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
        sysError("tfsMount(socket)");


    bzero((char*) &serv_addr, sizeof(serv_addr));
    serv_addr.sun_family = AF_UNIX;
    strncpy(serv_addr.sun_path, address, sizeof(serv_addr.sun_path)-1);
    servlen = strlen(serv_addr.sun_path) + sizeof(serv_addr.sun_family);
    
    if(connect(socketfd, (struct sockaddr*) &serv_addr, servlen) < 0)
        return TECNICOFS_ERROR_CONNECTION_ERROR;
    if (send(socketfd, &uid, sizeof(uid_t*), 0) != sizeof(uid_t*)) sysError("tfsMount(sendUid)");
    return SUCCESS;
}

int tfsUnmount(){

    socketfd = NOT_CONNECTED;
    return SUCCESS;
}
