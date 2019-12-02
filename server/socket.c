#include "socket.h"

extern char* nameSocket;
extern struct timeval start;    //valor de tempo inicial
extern struct timeval end;      //valor de tempo final

extern pthread_t slave_threads[MAXCONNECTIONS];
extern int idx;         //posicao atual de slave_threads

extern void *clientSession(void* socketfd);
extern void endServer();
extern void blockSigInt();
extern void unblockSigInt();

//inicializa o socket do servidor
int socketInit() {
    int sockfd, servlen;
    struct sockaddr_un serv_addr;
    int str_size = strlen(SOCKETDIR) + strlen(nameSocket) + 1; //  
    char pwd[str_size];

    if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
        sysError("SocketInit(socket)");

    if(sprintf(pwd, "%s%s", SOCKETDIR, nameSocket) != str_size-1) sysError("socketInit(pwdName)");

    if (unlink(pwd) != 0) sysError("socketInit(unlink)");

    bzero((char*) &serv_addr, sizeof(serv_addr));
    serv_addr.sun_family = AF_UNIX;

    strncpy(serv_addr.sun_path, pwd, sizeof(serv_addr.sun_path)-1);
    servlen = strlen(serv_addr.sun_path) + sizeof(serv_addr.sun_family);

    if(bind(sockfd, (struct sockaddr*) &serv_addr, servlen) < 0)
        sysError("SocketInit(bind)");

    if (listen(sockfd, MAXCONNECTIONS) != 0) sysError("socketInit(listen)");
    return sockfd;
}

//le um comando do socket
void readCommandfromSocket(int fd, char* buffer){
    char c;
    int i = 0;

    if (read(fd, &c, CHAR_SIZE) != CHAR_SIZE) sysError("readCommandfromSocket(read)");
    
    while(1){   
        if(c == '\0'){  //quando for atingido um '\0', chegamos ao fim do comando
            buffer[i] = c;
            break;
        }
        buffer[i++] = c;
        if (read(fd, &c, CHAR_SIZE) != CHAR_SIZE) sysError("readCommandfromSocket(read)");
    }
}

//retorna o uid do cliente
uid_t getSockUID(int fd){
    struct ucred u_cred;
    socklen_t len = sizeof(struct ucred);
    if (getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &u_cred, &len) == -1) sysError("clientSession(getsockopt)");
    return u_cred.uid;
}

//funcao do servidor que trata de aceitar os pedidos de ligacao dos clientes,
//criando um novo socket e uma tarefa, para criar dialogo com o cliente
void processClient(int sockfd) {
    int newsockfd;
    socklen_t clilen;
    struct sockaddr_un cli_addr;
    
    // Tempo de inicio
    if(gettimeofday(&start, NULL)) errnoPrint(); 

    for(;;) {
        clilen = sizeof(cli_addr);
        if ((newsockfd = accept(sockfd, (struct sockaddr*) &cli_addr, &clilen)) == -1) sysError("processCliente(accept)");
        if(newsockfd < 0) sysError("processClient(accept)");
        
        //Confirma espaco para aceitar
        if(idx == MAXCONNECTIONS) endServer(); //quando o maximo de ligacoes possivel for atingido, terminamos ordeiramente o servidor
        
        blockSigInt();  //bloqueamos os sinais antes e depois da funcao pthread_create
        if(pthread_create(&slave_threads[idx], NULL, clientSession, (void*) &newsockfd)) sysError("processClient(thread)");
        unblockSigInt();
        
        idx++; //apenas acessivel pela tarefa principal
    }
}

//funcao que trata de enviar uma resposta ao pedido do cliente
void feedback(int sockfd, int msg){
    if(msg == INT_MIN) return;  //caso a mensagem tenha o valor default, nao se envia resposta ao cliente
    if(write(sockfd, &msg, INT_SIZE) != INT_SIZE) sysError("feedback(write)");
}

//fecho de um socket
void closeSocket(int fd){
    if(close(fd) < 0) sysError("closeSocket(close)");
}