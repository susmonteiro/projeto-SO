#include "socket.h"

extern char* nameSocket;
extern struct timeval start;
extern struct timeval end; //valores de tempo inicial e final

extern pthread_t slave_threads[MAXCONNECTIONS];
extern int idx;
extern lock idx_lock;  //mutex para a variavel idx

extern void *clientSession(void* socketfd);
extern void endServer();


int socketInit() {
    int sockfd, servlen;
    struct sockaddr_un serv_addr;
    int str_size = strlen(SOCKETDIR) + strlen(nameSocket) + 1; //  
    char pwd[str_size];
    if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
        sysError("SocketInit(socket)");

    if(sprintf(pwd, "%s%s", SOCKETDIR, nameSocket) != str_size-1) sysError("socketInit(pwdName)");

    unlink(pwd);

    bzero((char*) &serv_addr, sizeof(serv_addr));
    serv_addr.sun_family = AF_UNIX;

    strncpy(serv_addr.sun_path, pwd, sizeof(serv_addr.sun_path)-1);
    servlen = strlen(serv_addr.sun_path) + sizeof(serv_addr.sun_family);

    if(bind(sockfd, (struct sockaddr*) &serv_addr, servlen) < 0)
        sysError("SocketInit(bind)");

    listen(sockfd, MAXCONNECTIONS);
    return sockfd;
}


void readCommandfromSocket(int fd, char* buffer){
    char c;
    int i = 0;
    puts("ReadCommandfromSocket");

    size_t size = read(fd, &c, CHAR_SIZE);
    printf("%ld\n", size);
    while(size){   
        if(c == '\0'){
            buffer[i] = c;
            break;
        }
        buffer[i++] = c;
        size = read(fd, &c, CHAR_SIZE);
    }

}

uid_t getSockUID(int fd){
    struct ucred u_cred;
    socklen_t len = sizeof(struct ucred);
    if (getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &u_cred, &len) == -1) sysError("clientSession(getsockopt)");
    return u_cred.uid;
}

void processClient(int sockfd){
    int newsockfd;
    socklen_t clilen;
    struct sockaddr_un cli_addr;
    
    // Tempo de inicio
    if(gettimeofday(&start, NULL)) errnoPrint(); 

    for(;;) {
        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr*) &cli_addr, &clilen);
        if(newsockfd < 0) sysError("processClient(accept)");
        

        closeWriteLock(idx_lock);
        //Confirma espaco para aceitar
        if(idx == MAXCONNECTIONS) endServer();
        if(pthread_create(&slave_threads[idx], NULL, clientSession, (void*) &newsockfd)) sysError("processClient(thread)");
        idx++; 
        
        openLock(idx_lock);

        puts("prossclient");

    }
}

void feedback(int sockfd, int msg){
    if(msg == INT_MIN) return;
    if(write(sockfd, &msg, INT_SIZE) != INT_SIZE) sysError("feedback(write)");
}


void closeSocket(int fd){
    if(close(fd) < 0) sysError("closeSocket(close)");
}