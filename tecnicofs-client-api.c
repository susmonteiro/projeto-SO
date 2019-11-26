#include "tecnicofs-client-api.h"


int inSession = 0;
int socketfd = NOT_CONNECTED;

void sysError(const char* msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}



int tfsCreate(char *filename, permission ownerPermissions, permission othersPermissions){
    int filenamelen = strlen(filename);
    int size = filenamelen + PADDING_COMMAND_C;
    char command[size];
    int answer;

    if (socketfd == NOT_CONNECTED) return TECNICOFS_ERROR_NO_OPEN_SESSION;

    sprintf(command, "%c %s %d%d", 'c', filename, ownerPermissions, othersPermissions);
    
    printf("command:%s\n", command);

    if (send(socketfd, command, size, 0) != size) sysError("tfsCreate(send)");
    if (recv(socketfd, &answer, sizeof(int*), 0)) sysError("tfsCreate(recv)");
    
    printf("answer:%d\n", answer);

    if (answer == FAIL) return TECNICOFS_ERROR_FILE_ALREADY_EXISTS; 
    
    return 0;
}

int tfsDelete(char *filename){return 0;}
int tfsRename(char *filenameOld, char *filenameNew){return 0;}
int tfsOpen(char *filename, permission mode){return 0;}
int tfsClose(int fd){return 0;}
int tfsRead(int fd, char *buffer, int len){return 0;}
int tfsWrite(int fd, char *buffer, int len){return 0;}

int tfsMount(char * address){
    int servlen;
    struct sockaddr_un serv_addr;
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
    
    return 0;
}

int tfsUnmount(){

    socketfd = NOT_CONNECTED;
    return 0;
}
