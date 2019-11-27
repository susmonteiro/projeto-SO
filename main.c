#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include "fs.h"
#include "sync.h"
#include "lib/inodes.h"
#include "constants.h"

//==========
//Constantes
//==========
#define _GNU_SOURCE

#define MAX_INPUT_SIZE 100
#define MAX_ARGS_INPUTS 2
#define MILLION 1000000
#define N_ARGC 4
#define MAXCONNECTIONS SOMAXCONN
#define COMMAND_NULL -1 //Comando inexistente
#define END_COMMAND 'z' //Comando criado para terminar as threads
#define MAX_FILES_OPENED 5


typedef struct  {
    int iNumber;
    permission open_as; 
}   tecnicofs_fd;

//=====================
//Prototipos principais
//=====================
void *clientSession(void * socketfd);

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
pthread_mutex_t mutex;
pthread_cond_t cond;


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
        nameSocket = (char*)malloc(sizeof(char)*(strlen(argv[1])+1));
        strcpy(nameSocket, argv[1]);  // XXX falta \0 ??
        outputFile = (char*)malloc(sizeof(char)*(strlen(argv[2])+1));
        puts(argv[1]);
        strcpy(outputFile, argv[2]);

        numberBuckets = atoi(argv[3]);
    }

}

//========================
//Funcoes de Inicializacao
//========================

//Inicializa memoria usada pela HashTable de tecnicofs (arvores) e os seus trincos
void initHashTable(int size){
    int i = 0;
	hash_tab = (tecnicofs*)malloc(size * sizeof(tecnicofs)); //alocacao da tabela para tecnicofs
	for(i = 0; i < size; i++) {
		hash_tab[i] = new_tecnicofs();  // aloca um tecnicofs (uma arvore)
		initLock(hash_tab[i]);          // inicializa trinco desse tecnicofs (arvore)

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
        sysError("SocketInit(bind):");

    listen(sockfd, MAXCONNECTIONS);
    return sockfd;
}

int initialize(){
    initHashTable(numberBuckets);
    inode_table_init();
    initMutex(&mutex);
    initCond(&cond);
    return socketInit(); 

}


//================================
//Funcoes de libertacao de memoria
//================================




//Liberta memoria usada pela HashTable de tecnicofs (arvores) e os seus trincos
void freeHashTab(int size){
    int i;
	for(i = 0; i < size; i++) {
		destroyLock(hash_tab[i]);
		free_tecnicofs(hash_tab[i]);
	}
	free(hash_tab); //liberta tabela (final)
}


void liberator(){
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

    rClosed(fs);    // permite leituras simultaneas, impede escrita
    search_result = lookup(fs, filename); //procura por nome, devolve inumber
    rOpen(fs);
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

    wClosed(fs);    // bloqueia leituras e escritas do fs
    iNumber = inode_create(uid, atoi(vec[1])/10, atoi(vec[1])%10);      //FIXME nao deixar isto nojento
    create(fs, vec[0], iNumber);
    wOpen(fs);


    puts("commandCreate");

    return SUCCESS;
}

int commandDelete(char vec[MAX_ARGS_INPUTS][MAX_INPUT_SIZE], uid_t uid){
    int search_result;
    uid_t owner;
    tecnicofs fs = hash_tab[searchHash(vec[0], numberBuckets)]; //arvore do nome atual

    if ((search_result = fileLookup(fs, vec[0])) == -1)     //se nao existir
        return DOESNT_EXIST;

    inode_get(search_result, &owner, NULL, NULL, NULL, 0);
    if (uid != owner)
        return PERMISSION_DENIED;
    
    wClosed(fs);    // bloqueia leituras e escritas do fs
    printf("seacrhres %d", search_result);
    inode_delete(search_result);
    delete(fs, vec[0]); 
    wOpen(fs);   

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

int commandOpen(char vec[MAX_ARGS_INPUTS][MAX_INPUT_SIZE]){
    return SUCCESS;
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
//=====================


void processClient(int sockfd){
    int newsockfd;
    socklen_t clilen;
    struct sockaddr_un cli_addr;
    
    for(;;) {
        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr*) &cli_addr, &clilen);
        if(newsockfd < 0) sysError("processClient(accept)");
        

        wClosed_rc(&mutex);

        if(pthread_create(&slave_threads[idx], NULL, clientSession, (void*) &newsockfd)) sysError("processClient(thread)");
        idx++; 
        
        wOpen_rc(&mutex);

        puts("prossclient");

    }
}

void feedback(int sockfd, int msg){
    if(send(sockfd, &msg, INT_SIZE, 0) != INT_SIZE) sysError("feedback(write)");
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




//==================
//Funcoes principais
//==================
void readCommandfromSocket(int fd, char* buffer){
    char c;
    int idx = 0;
    while(recv(fd, &c, CHAR_SIZE, 0)){        
        if(c == '\0'){
            buffer[idx] = c;
            break;
        }
        buffer[idx++] = c;
    }
}

void parseCommand(int socketfd, char* command, char vec[MAX_ARGS_INPUTS][MAX_INPUT_SIZE]){
    char input[MAX_INPUT_SIZE];


    readCommandfromSocket(socketfd, input);

    printf("\n%s\n", input);
    
    int numTokens = sscanf(input, "%c %s %s", command, vec[0], vec[1]); //scanf formatado 
    printf("\t%d\n", numTokens);

    printf("%c === %s === %s.\n", *command, vec[0], vec[1]);

    if (numTokens > 4 || numTokens < 2) { // qualquer comando que nao tenha 2 ou 3 argumentos, e' invalido
        fprintf(stderr, "Error: invalid command in Queue\n");
        exit(EXIT_FAILURE);
    } else if (numTokens == 2 && !(*command == END_COMMAND || *command == 'd')) { //teste para caso comando 'x' e 'd' 
        fprintf(stderr, "Error: invalid command in Queue\n");
        exit(EXIT_FAILURE);
    }


}

//funcao chamada pelas threads consumidoras e trata de executar os comandos do vetor de comandos
void *clientSession(void* socketfd) {
    int fd = *((int *)socketfd);
    tecnicofs_fd fd_table[MAX_FILES_OPENED];
    int i;
    struct ucred ucredential;
    int len = sizeof(struct ucred);
    if (getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &ucred, &len) == -1)
    if(recv(fd, &uid, sizeof(uid_t*), 0) != sizeof(uid_t*)) sysError("applyCommands(recvUid)");

    for(i = 0; i < MAX_FILES_OPENED; i++)
        fd_table->iNumber = -1;
   
    while(1) { 
        char command;
        char args[MAX_ARGS_INPUTS][MAX_INPUT_SIZE];
        int result;
        parseCommand(fd, &command, args);

        switch (command) {
            case 'c':
                result = commandCreate(args, uid);
                break;

            case 'd':
                result = commandDelete(args, uid);
                break;

            case 'r':
                result = commandRename(args, uid);
                break;

            case 'o':
                result = commandOpen(args);

            case END_COMMAND:
                //antes de sair da funcao applyCommands(), a tarefa atual coloca um novo comando de finalizacao no vetor, 
                //para que a proxima tarefa a aceder ao vetor de comandos tambem possa terminar
                return NULL;

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
    int sockfd;
    parseArgs(argc, argv);                                  // verifica numero de argumentos

    sockfd = initialize();
    processClient(sockfd);

    
    print_tree_outfile();                            // imprime o conteudo final da fs para o ficheiro de saida
    
    liberator();
    exit(EXIT_SUCCESS);
}