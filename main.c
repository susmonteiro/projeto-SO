#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <signal.h>
#include <limits.h>
#include "fs.h"
#include "sync.h"
#include "lib/inodes.h"
#include "constants.h"

//==========
//Constantes
//=========+

#define MAX_INPUT_SIZE 100
#define MAX_ARGS_INPUTS 2
#define MILLION 1000000
#define N_ARGC 4
#define MAXCONNECTIONS SOMAXCONN
#define COMMAND_NULL -1 //Comando inexistente
#define MAX_FILES_OPENED 5
#define FD_EMPTY -1

#define CAN_READ(file_permission) file_permission & READ 
#define CAN_WRITE(file_permission) file_permission & WRITE 


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

//=================
//Variaveis Globais
//=================
char* nameSocket;
char* outputFile;
int numberBuckets = 1;
int nextINumber = 0;

//Implementa Consumidor de Comandos
pthread_t slave_threads[MAXCONNECTIONS];
int idx = 0;

//Tabela de Arvores de ficheiros
tecnicofs* hash_tab;
int opened_files[INODE_TABLE_SIZE]; //vetor que guarda o numero de clientes que tem um determinado ficheiro aberto
lock idx_lock;  //mutex para a variavel idx
lock of_lock;   //mutex para o vetor opened_files

int sockfd;



//================
//Funcoes de parse
//================
static void displayUsage (const char* appName){
    printf("Usage: %s\n", appName);
    exit(EXIT_FAILURE);
}

void errorParse() {
    fprintf(stderr, "Error: command invalid\n");
    exit(EXIT_FAILURE);
}

void sysError(const char* msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

static void parseArgs (long argc, char* const argv[]){
    if (argc != N_ARGC) {
        fprintf(stderr, "Invalid format:\n");
        displayUsage(argv[0]);
    } else {
        if((nameSocket = (char*)malloc(sizeof(char)*(strlen(argv[1])+1))) == NULL) sysError("parseArgs(malloc)");
        strcpy(nameSocket, argv[1]);  // XXX falta \0 ??
        if ((outputFile = (char*)malloc(sizeof(char)*(strlen(argv[2])+1))) == NULL) sysError("parseArgs(malloc)");
        strcpy(outputFile, argv[2]);

        if((numberBuckets = atoi(argv[3])) < 1) sysError("parseArgs(numBucketsNonPositive)");
    }

}

//========================
//Funcoes de Inicializacao
//========================
void blockSigThreads(){
    //Mascara de signals que queremos bloquear acesso pelas threads
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    if (pthread_sigmask(SIG_BLOCK, &set, NULL) != 0) sysError("blockSigThreads(pthread_sigmask)");
}

void initSignal(){
    struct sigaction act;
    sigemptyset(&act.sa_mask);
    sigaddset(&act.sa_mask, SIGINT);    
    act.sa_sigaction = &endServer;
    act.sa_flags = SA_SIGINFO;

    if (sigaction(SIGINT, &act, NULL) < 0) sysError("initSignals(sigaction)");
}

void initOpenedFiles() {
    int i = 0;
    for(i = 0; i < INODE_TABLE_SIZE; i++) {
        opened_files[i] = 0;
    }
}
//Inicializa memoria usada pela HashTable de tecnicofs (arvores) e os seus trincos
void initHashTable(int size){
    int i = 0;
	hash_tab = (tecnicofs*)malloc(size * sizeof(tecnicofs)); //alocacao da tabela para tecnicofs
	for(i = 0; i < size; i++) {
		hash_tab[i] = new_tecnicofs();  // aloca um tecnicofs (uma arvore)
		initLock(hash_tab[i]->tecnicofs_lock);          // inicializa trinco desse tecnicofs (arvore)

		hash_tab[i]->bstRoot = NULL;    // raiz de cada arvore
	}
}

int socketInit() {
    int sockfd, servlen;
    struct sockaddr_un serv_addr;
    if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
        sysError("SocketInit(socket)");

    unlink(nameSocket);

    bzero((char*) &serv_addr, sizeof(serv_addr));
    serv_addr.sun_family = AF_UNIX;
    strncpy(serv_addr.sun_path, nameSocket, sizeof(serv_addr.sun_path)-1);
    servlen = strlen(serv_addr.sun_path) + sizeof(serv_addr.sun_family);
    if(bind(sockfd, (struct sockaddr*) &serv_addr, servlen) < 0)
        sysError("SocketInit(bind)");

    listen(sockfd, MAXCONNECTIONS);
    return sockfd;
}

int initialize(){
    initSignal();       
    //estruturas
    initOpenedFiles();
    initHashTable(numberBuckets);
    inode_table_init();
    //
    initLock(idx_lock);
    initLock(of_lock);
    return socketInit(); 

}


//================================
//Funcoes de libertacao de memoria
//================================




//Liberta memoria usada pela HashTable de tecnicofs (arvores) e os seus trincos
void freeHashTab(int size){
    int i;
    destroyLock(idx_lock);
    destroyLock(of_lock);
	for(i = 0; i < size; i++) {
		destroyLock(hash_tab[i]->tecnicofs_lock);
		free_tecnicofs(hash_tab[i]);
	}
	free(hash_tab); //liberta tabela (final)
}


void liberator(){
    close(sockfd);
    freeHashTab(numberBuckets);
    inode_table_destroy();
    free(nameSocket);
    free(outputFile);
}
//==============================
//Funcoes relativas aos comandos
//==============================

int fileLookup(tecnicofs fs, char *filename){
    int search_result;   //inumber retornado pela funcao lookup

    closeReadLock(fs->tecnicofs_lock);    // permite leituras simultaneas, impede escrita
    search_result = lookup(fs, filename); //procura por nome, devolve inumber
    openLock(fs->tecnicofs_lock);
    return search_result;
}


int commandCreate(char vec[MAX_ARGS_INPUTS][MAX_INPUT_SIZE], uid_t uid) {
    int iNumber;
    tecnicofs fs = hash_tab[searchHash(vec[0], numberBuckets)]; //arvore do nome atual
    
    if (fileLookup(fs, vec[0]) != -1)     //se ja existir 
        return ALREADY_EXISTS;


    // sscanf(ownerp, "%d", vec[1][0]);
    // sscanf(otherp, "%d", vec[1][1]);

    printf("permiss:%d %d\n", atoi(vec[1])/10, atoi(vec[1])%10);

    closeWriteLock(fs->tecnicofs_lock);    // bloqueia leituras e escritas do fs
    iNumber = inode_create(uid, atoi(vec[1])/10, atoi(vec[1])%10);      //FIXME nao deixar isto nojento
    create(fs, vec[0], iNumber);
    openLock(fs->tecnicofs_lock);


    puts("commandCreate");

    return SUCCESS;
}

int commandDelete(char vec[MAX_ARGS_INPUTS][MAX_INPUT_SIZE], uid_t uid){
    int search_result;
    uid_t owner;
    tecnicofs fs = hash_tab[searchHash(vec[0], numberBuckets)]; //arvore do nome atual

    if ((search_result = fileLookup(fs, vec[0])) == -1)     //se nao existir
        return DOESNT_EXIST;

    closeReadLock(of_lock);
    if (opened_files[search_result] != 0) return FILE_IS_OPENED;
    openLock(of_lock);

    inode_get(search_result, &owner, NULL, NULL, NULL, 0);

    // confirmar permissao para apagar (apenas consegue apagar o dono)
    if (uid != owner)
        return PERMISSION_DENIED;
    
    closeWriteLock(fs->tecnicofs_lock);    // bloqueia leituras e escritas do fs
    printf("seacrhres %d", search_result);
    inode_delete(search_result);
    delete(fs, vec[0]); 
    openLock(fs->tecnicofs_lock);   

    puts("CommandDelete");

    return SUCCESS;
}

//Comando renomear
int commandRename(char vec[MAX_ARGS_INPUTS][MAX_INPUT_SIZE], uid_t uid){
	int search_result;           //inumber retornado pela funcao lookup
    uid_t owner;
    tecnicofs fs1 = hash_tab[searchHash(vec[0], numberBuckets)]; //arvore do nome atual
    tecnicofs fs2 = hash_tab[searchHash(vec[1], numberBuckets)]; //name2 != null, tal como verificado na funcao applyCommands()

	while (1) {
        
		if (TryLock(fs2)) {
			if (lookup(fs2, vec[1]) != -1) { //se ja existir, a operacao e' cancelada sem devolver erro
				Unlock(fs2);
				return ALREADY_EXISTS;
			} else if (fs1==fs2 || TryLock(fs1)) {

                // Procurar o ficheiro pelo nome; Se existir [search_result = Inumber do Ficheiro]
				if ((search_result = lookup(fs1, vec[0])) == -1) {
					Unlock(fs1);
					if (fs1 != fs2) Unlock(fs2); // nao fazer unlock 2 vezes da mesma arvore
					return DOESNT_EXIST;
				}
                // Verifica se nao e' o dono que esta a tentar alterar o nome do ficheiro
                inode_get(search_result, &owner, NULL, NULL, NULL, 0);
                if(uid != owner){
                    Unlock(fs1);
					if (fs1 != fs2) Unlock(fs2); // nao fazer unlock 2 vezes da mesma arvore
					return PERMISSION_DENIED;
                }


				delete(fs1, vec[0]);
				if (fs1 != fs2) Unlock(fs1);
				// se as fs forem iguais nao podemos fazer unlock antes do create
				create(fs2, vec[1], search_result); // novo ficheiro: novo nome, mesmo Inumber
				Unlock(fs2);

				break;

			} else 
				Unlock(fs2);
		}
	}

    return SUCCESS;
}

int commandOpen(char vec[MAX_ARGS_INPUTS][MAX_INPUT_SIZE], uid_t uid, tecnicofs_fd *file_tab){
    int search_result, idx_fd;
    permission ownerp, otherp, mode;
    uid_t user;
    tecnicofs fs = hash_tab[searchHash(vec[0], numberBuckets)]; //arvore do nome atual

    if ((search_result = fileLookup(fs, vec[0])) == -1)     //se nao existir
        return DOESNT_EXIST;

    inode_get(search_result, &user, &ownerp, &otherp, NULL, 0);

    mode = atoi(vec[1]);
    //consideramos que a permissao dos outros nao se aplica ao dono do ficheiro
    if(!((user==uid && (mode&ownerp) == mode) || (user!=uid && (mode&otherp) == mode))){
        // printf("user %d | uid %d | mod %d | ownerp %d | otherp %d \n\t1and %d ==== 2and %d\n",user, uid, mode, ownerp, otherp, mode&ownerp, mode&otherp);
        // puts("nao criado");
        return PERMISSION_DENIED;
    }
    
    idx_fd = -1;
    while (file_tab[++idx_fd].iNumber != FD_EMPTY)
        if (idx_fd == MAX_FILES_OPENED)
            return MAX_OPENED_FILES;
    
    
    file_tab[idx_fd].iNumber = search_result;
    file_tab[idx_fd].open_as = mode;

    closeWriteLock(of_lock);
    opened_files[search_result]++; //incrementa o numero de clientes que tem o ficheiro (cujo iNumber e' search_result) aberto
    openLock(of_lock);

    puts("CommandOpen");

    printf("%d\n", file_tab[idx_fd].iNumber);
    return idx_fd;
}

int commandClose(char vec[MAX_ARGS_INPUTS][MAX_INPUT_SIZE], tecnicofs_fd *file_tab){
    int idx_fd = atoi(vec[0]);  //indice da tabela de ficheiros do cliente

    if (idx_fd < 0 || idx_fd > 4) return INDEX_OUT_OF_RANGE;

    if (file_tab[idx_fd].iNumber == FD_EMPTY) return NOT_OPENED;

    closeWriteLock(of_lock);
    opened_files[file_tab[idx_fd].iNumber]--; //decrementa o numero de clientes que tem o ficheiro (cujo iNumber e' file_tab[idx_fd].iNumber) aberto
    openLock(of_lock);

    file_tab[idx_fd].iNumber = FD_EMPTY;

    puts("commandClose");
    return SUCCESS;
}

void commandRead(int fd, char vec[MAX_ARGS_INPUTS][MAX_INPUT_SIZE], tecnicofs_fd *file_tab){
    // confirmar input
    int tab_idx = atoi(vec[0]);
    int buffer_len = atoi(vec[1]); 
    tecnicofs_fd file;
    char buffer[buffer_len];
    int msg = SUCCESS;
    if(tab_idx < 0 || tab_idx > MAX_FILES_OPENED) msg = INDEX_OUT_OF_RANGE;

    file = file_tab[tab_idx];
    if (file.iNumber == FD_EMPTY) msg = NOT_OPENED;
    if (!(CAN_READ(file.open_as))) msg = PERMISSION_DENIED;
    
    //ler apenas em caso de sucesso
    if(msg == SUCCESS)
    //If the length of src is less than n, strncpy() writes additional null bytes to dest to ensure that a total of n bytes are  writ‐ten.
        msg = inode_get(file.iNumber, NULL, NULL, NULL, buffer, buffer_len-1); // buffer comeca em idx 0, -1 cuida do offset
        // msg passa a ser o numero de caracteres lidos
    
    feedback(fd, msg); //informar de possivel erro
    printf("%d\n", msg);
    if(msg > 0){ // caso a msg nao seja erro

        // O TAMANHO MSG NAO INCLUI O \O!!!!!!!! STRLEN

        printf("conteudo do bugger: %s\n", buffer);
        // +1 PARA INCLUIR O \0
        if (write(fd, buffer, msg+1) != msg+1) sysError("commandRead(write)");
    }

}

int commandWrite(int fd, char vec[MAX_ARGS_INPUTS][MAX_INPUT_SIZE], tecnicofs_fd *file_tab){
    int tab_idx = atoi(vec[0]);
    tecnicofs_fd file;
    if(tab_idx < 0 || tab_idx > MAX_FILES_OPENED) return INDEX_OUT_OF_RANGE;

    file = file_tab[tab_idx];
    if (file.iNumber == FD_EMPTY) return NOT_OPENED;
    if (!(CAN_WRITE(file.open_as))) return PERMISSION_DENIED;
    
    puts("commandWrite");

    return inode_set(file.iNumber, vec[1], strlen(vec[1]));
}

/* Abre o ficheiro de output e escreve neste a arvore */
void print_tree_outfile() {
    FILE *fpout = fopen(outputFile, "w");

    if (!fpout) {
        errnoPrint();
        return;
    }

    print_HashTab_tree(fpout, hash_tab, numberBuckets); //imprimir a arvore
    fclose(fpout);
}

//=====================
//Funcoes sobre sockets
//=====================0



void processClient(int sockfd){
    int newsockfd;
    socklen_t clilen;
    struct sockaddr_un cli_addr;
    
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

//=====================
//Funcoes sobre tarefas
//=====================

/* A funcao devolve a diferenca entre tempo inicial e fical, em segundos*/
float time_taken(struct timeval start, struct timeval end) {
    float secs;
    float microseconds;
    
    secs = end.tv_sec - start.tv_sec; 
    microseconds = (end.tv_usec - start.tv_usec)/(float)MILLION;
    return secs + microseconds;
}


void endServer(){
    int i;
    for(i = 0; i < idx; i++)
        if(pthread_join(slave_threads[i], NULL)) sysError("endServer(pthread_join)");

    print_tree_outfile();                            // imprime o conteudo final da fs para o ficheiro de saida
    liberator();                                     // liberta a memoria alocada ao longo do programa
    
    exit(EXIT_SUCCESS);
}


//==================
//Funcoes principais
//==================
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

void parseCommand(int socketfd, char* command, char vec[MAX_ARGS_INPUTS][MAX_INPUT_SIZE]){
    char input[MAX_INPUT_SIZE];

    readCommandfromSocket(socketfd, input);

    puts("sscanf do parse");
    int numTokens = sscanf(input, "%c %s %s", command, vec[0], vec[1]); //scanf formatado 
    printf("\t%d\n", numTokens);

    printf("%c === %s === %s.\n", *command, vec[0], vec[1]);

    if (numTokens > 4 || numTokens < 1) { // qualquer comando que nao tenha 2 ou 3 argumentos, e' invalido
        fprintf(stderr, "Error: invalid command in Queue\n");
        exit(EXIT_FAILURE);
    } else if ((numTokens == 1 && *command != END_COMMAND) || (numTokens == 2 && *command != DELETE_COMMAND && *command != CLOSE_COMMAND)) { //teste para caso comando 'x' ,'z', 'd' 
        fprintf(stderr, "Error: invalid command in Queue\n");
        exit(EXIT_FAILURE);
    }


}

uid_t getSockUID(int fd){
    struct ucred u_cred;
    socklen_t len = sizeof(struct ucred);
    if (getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &u_cred, &len) == -1) sysError("clientSession(getsockopt)");
    return u_cred.uid;
}


//funcao chamada pelas threads consumidoras e trata de executar os comandos do vetor de comandos
void *clientSession(void* socketfd) {
    int fd = *((int *)socketfd);
    tecnicofs_fd fd_table[MAX_FILES_OPENED];
    int i;

    //UID do cliente
    uid_t uid;
    uid = getSockUID(fd);

    //Bloqueia a interrupcao(signal)
    //Mascara de signals que queremos bloquear acesso pelas threads
    blockSigThreads();

    //inicializa tabela de descritores de ficheiros
    for(i = 0; i < MAX_FILES_OPENED; i++)
        fd_table[i].iNumber = FD_EMPTY;
   
    while(1) { 
        char command;
        char args[MAX_ARGS_INPUTS][MAX_INPUT_SIZE];
        int result = INT_MIN;

        parseCommand(fd, &command, args);

        switch (command) {
            case CREATE_COMMAND:
                result = commandCreate(args, uid);
                break;

            case DELETE_COMMAND:
                result = commandDelete(args, uid);
                break;

            case RENAME_COMMAND:
                result = commandRename(args, uid);
                break;

            case OPEN_COMMAND:
                result = commandOpen(args, uid, fd_table);
                break;

            case CLOSE_COMMAND:
                result = commandClose(args, fd_table);
                break;
            case READ_COMMAND:
                commandRead(fd, args, fd_table);
                break;

            case WRITE_COMMAND:
                result = commandWrite(fd, args, fd_table);
                break;

            case END_COMMAND:
                //antes de sair da funcao applyCommands(), a tarefa atual coloca um novo comando de finalizacao no vetor, 
                //para que a proxima tarefa a aceder ao vetor de comandos tambem possa terminar
                if(close(fd) == -1) sysError("clientSession(close)");
                pthread_exit(NULL);

            default: { /* error */
                fprintf(stderr, "Error: command to apply\n");
                exit(EXIT_FAILURE);
            }
        }

        feedback(fd, result);
    }
}

//===========
//Funcao MAIN
//===========
int main(int argc, char* argv[]) {
    
    parseArgs(argc, argv);                                  // verifica numero de argumentos

    sockfd = initialize();
    processClient(sockfd);

    exit(EXIT_SUCCESS);
}