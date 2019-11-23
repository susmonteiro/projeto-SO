#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "fs.h"
#include "sync.h"

//==========
//Constantes
//==========
#define MAX_INPUT_SIZE 100
#define MAX_ARGS_INPUTS 3
#define MILLION 1000000
#define N_ARGC 4
#define MAXCONNECTIONS SOMAXCONN
#define COMMAND_NULL -1 //Comando inexistente
#define END_COMMAND 'x' //Comando criado para terminar as threads

//=====================
//Prototipos principais
//=====================
void *applyCommands();
void processInput(const char *pwd);

//=================
//Variaveis Globais
//=================
char* nameSocket;
char* outputFile;
int numberBuckets = 1;
int nextINumber = 0;

//Implementa Consumidor de Comandos
enum state {T_CREATED, T_TERMINATED};
enum state state_threads[MAXCONNECTIONS] = {T_CREATED}; 
pthread_t slave_threads[MAXCONNECTIONS];
int idx = 0;
int NTerminated = 0;


//Vetor de comandos
char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int index_prod = 0;
int index_cons = 0;

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
        nameSocket = (char*)malloc(sizeof(char)*strlen(argv[1]));
        strcpy(nameSocket, argv[1]);  // falta \0 ??
        outputFile = (char*)malloc(sizeof(char)*strlen(argv[2]));
        strcpy(outputFile, argv[2]);
        numberBuckets = atoi(argv[3]);
    }

}


//=====================
//Funcoes sobre Arvores
//=====================

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

//Liberta memoria usada pela HashTable de tecnicofs (arvores) e os seus trincos
void freeHashTab(int size){
    int i;
	for(i = 0; i < size; i++) {
		destroyLock(hash_tab[i]);
		free_tecnicofs(hash_tab[i]);
	}
	free(hash_tab); //liberta tabela (final)
}

void commandCreate(char* vec){
    int iNumber;
    tecnicofs fs = hash_tab[searchHash(vec[1], numberBuckets)]; //arvore do nome atual

    wClosed(fs);    // bloqueia leituras e escritas do fs
    iNumber = ++nextINumber;
    create(fs, vec[1], iNumber);
    wOpen(fs);
}

void commandDelete(char* vec){
    tecnicofs fs = hash_tab[searchHash(vec[1], numberBuckets)]; //arvore do nome atual

    wClosed(fs);    // bloqueia leituras e escritas do fs
    delete(fs, vec[1]); 
    wOpen(fs);   
}

void commandLookup(char *vec){
    int searchResult;   //inumber retornado pela funcao lookup
    tecnicofs fs = hash_tab[searchHash(vec[1], numberBuckets)]; //arvore do nome atual

    rClosed(fs);    // permite leituras simultaneas, impede escrita
    searchResult = lookup(fs, vec[1]); //procura por nome, devolve inumber
    rOpen(fs);
    if(!searchResult)
        printf("%s not found\n", vec[1]);
    else
        printf("%s found with inumber %d\n", vec[1], searchResult);
}

//Comando renomear
void commandRename(char *vec){
	int searchResult;           //inumber retornado pela funcao lookup
    tecnicofs fs1 = hash_tab[searchHash(vec[1], numberBuckets)]; //arvore do nome atual
    tecnicofs fs2 = hash_tab[searchHash(vec[2], numberBuckets)]; //name2 != null, tal como verificado na funcao applyCommands()

	while (1) {
		if (TryLock(fs2)) {
			if (lookup(fs2, vec[2])) { //se ja existir, a operacao e' cancelada sem devolver erro
				Unlock(fs2);
				break; 
			} else if (fs1==fs2 || TryLock(fs1)) {

                // Procurar o ficheiro pelo nome; Se existir [searchResult = Inumber do Ficheiro]
				if ((searchResult = lookup(fs1, vec[1])) == 0) {
					Unlock(fs1);
					if (fs1 != fs2) Unlock(fs2); // nao fazer unlock 2 vezes da mesma arvore
					break;
				}

				delete(fs1, vec[1]);
				if (fs1 != fs2) Unlock(fs1);
				// se as fs forem iguais nao podemos fazer unlock antes do create
				create(fs2, vec[2], searchResult); // novo ficheiro: novo nome, mesmo Inumber
				Unlock(fs2);

				break;

			} else 
				Unlock(fs2);
		}
	}
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
int socketInit() {
    int sockfd, servlen;
    struct sockaddr_un serv_addr, serv_addr;
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

void processClient(int sockfd){
    int clilen, newsockfd;
    int pid;
    struct sockaddr_un cli_addr;
    for(;;) {
        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr*) &cli_addr, &clilen);
        if(newsockfd < 0) sysError("processClient(accept)");
        
        wClosed_rc(&mutex);
        if(pthread_create(slave_threads[idx], NULL, applyCommands, newsockfd)) sysError("processClient(thread)");
        idx++;
        wOpen_rc(&mutex);

    }
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
void parseCommand(int fd, char* vec){
    char command[MAX_INPUT_SIZE];
    read(fd, command, MAX_INPUT_SIZE);

    int numTokens = sscanf(command, "%c %s %s", vec[0], vec[1], vec[2]); //scanf formatado 
    
    if (numTokens > 4 || numTokens < 2) { // qualquer comando que nao tenha 1,2,3 argumentos, e' invalido
        fprintf(stderr, "Error: invalid command in Queue\n");
        exit(EXIT_FAILURE);
    } else if (numTokens == 2 && (vec[0] != END_COMMAND || vec[0] != 'd')) { //teste para caso comando 'x' e 'r' 
        fprintf(stderr, "Error: invalid command in Queue\n");
        exit(EXIT_FAILURE);
    }

}

//funcao chamada pelas threads consumidoras e trata de executar os comandos do vetor de comandos
void *applyCommands(void * socketFd){
    int fd = *((int *)socketFd);
    while(1) { //enquanto houver comandos

        char command[MAX_ARGS_INPUTS];
        parseCommand(fd, command);

        switch (command[0]) {
            case 'c':
                commandCreate(command);
                break;

            case 'l':
                commandLookup(command);
                break;

            case 'd':
                commandDelete(command);
                break;

            case 'r':
                commandRename(command);
                break;

            case 'o':
                commandOpen(command);

            case END_COMMAND:
                //antes de sair da funcao applyCommands(), a tarefa atual coloca um novo comando de finalizacao no vetor, 
                //para que a proxima tarefa a aceder ao vetor de comandos tambem possa terminar
                pushEndCommand(); 
                NTerminated++;
                return;

            default: { /* error */
                fprintf(stderr, "Error: command to apply\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}





//===========
//Funcao MAIN
//===========
int main(int argc, char* argv[]) {
    int sockfd;
    parseArgs(argc, argv);                                  // verifica numero de argumentos

    initHashTable(numberBuckets);
    
    initMutex(&mutex);
    initCond(&cond);
    sockfd = socketInit();                                  // inicializa as tarefas e chama a funcao applyCommands()
    processClient(sockfd);

    
    print_tree_outfile();                            // imprime o conteudo final da fs para o ficheiro de saida
    
    freeHashTab(numberBuckets);
    exit(EXIT_SUCCESS);
}