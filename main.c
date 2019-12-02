#include "main.h"

//=================
//Variaveis Globais
//=================
char* nameSocket;
char* outputFile;
int numberBuckets = 1;

//Implementa handler de clientes
pthread_t slave_threads[MAXCONNECTIONS];
int idx = 0;

//Tabela de Arvores de ficheiros
tecnicofs* hash_tab;

int sockfd;     //descritor de socket principal do servidor
struct timeval start, end; //valores de tempo inicial e final

/* lock of_lock;
 *///===========
//Funcao MAIN
//===========
int main(int argc, char* argv[]) {
    
    parseArgs(argc, argv);      // verifica numero de argumentos

    sockfd = startRuntime();
    processClient(sockfd);

    exit(EXIT_SUCCESS);         // redundante, em principio nao e' executado
}


//==================
//Funcoes principais
//==================

void parseCommand(int socketfd, char* command, char vec[MAX_ARGS_INPUTS][MAX_INPUT_SIZE]){
    char input[MAX_INPUT_SIZE];

    readCommandfromSocket(socketfd, input);

    int numTokens = sscanf(input, "%c %s %s", command, vec[0], vec[1]); //scanf formatado 

    if (numTokens > 4 || numTokens < 1) { // qualquer comando que nao tenha 2 ou 3 argumentos, e' invalido
        fprintf(stderr, "Error: invalid command in Queue\n");
        exit(EXIT_FAILURE);
    } else if ((numTokens == 1 && *command != END_COMMAND) || (numTokens == 2 && *command != DELETE_COMMAND && *command != CLOSE_COMMAND)) { //teste para caso comando 'x' ,'z', 'd' 
        fprintf(stderr, "Error: invalid command in Queue\n");
        exit(EXIT_FAILURE);
    }


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
                clearClientOpenedFilesCounter(fd_table);
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


//================
//Funcoes de parse
//================
static void displayUsage (const char* appName){
    printf("Usage: %s\n", appName);
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

//=======
//Runtime
//=======
int startRuntime(){
    initSignal();       
    initOpenedFilesCounter();
    initHashTable(numberBuckets);
    inode_table_init();
    //of_lock = (lock)malloc(sizeof(struct lock));
    //initLock(of_lock);
    return socketInit(); 
}

void endRuntime(){
    closeSocket(sockfd);
    freeHashTab(numberBuckets);
    freeOpenedFilesCounter();
    inode_table_destroy();
    free(nameSocket);
    free(outputFile);
}


    //+++++++++
    //Hashtable
    //+++++++++

//Inicializa memoria usada pela HashTable de tecnicofs (arvores) e os seus trincos
void initHashTable(int size){
    int i = 0;
	hash_tab = (tecnicofs*)malloc(size * sizeof(tecnicofs)); //alocacao da tabela para tecnicofs
	for(i = 0; i < size; i++) {
		hash_tab[i] = new_tecnicofs();          // aloca um tecnicofs (uma arvore)
		initLock(hash_tab[i]->tecnicofs_lock);  // inicializa trinco desse tecnicofs (arvore)
		
        hash_tab[i]->bstRoot = NULL;    // raiz de cada arvore
	}
}

//Liberta memoria usada pela HashTable de tecnicofs (arvores) e os seus trincos
void freeHashTab(int size){
    int i;
	for(i = 0; i < size; i++) {
		destroyLock(hash_tab[i]->tecnicofs_lock);
		free_tecnicofs(hash_tab[i]);
	}
	free(hash_tab); //liberta tabela (final)
}


    //+++++++
    //Signals
    //+++++++

void initSignal(){
    struct sigaction act;
    sigemptyset(&act.sa_mask);
    sigaddset(&act.sa_mask, SIGINT);    
    act.sa_sigaction = &endServer;
    act.sa_flags = SA_SIGINFO;

    if (sigaction(SIGINT, &act, NULL) < 0) sysError("initSignals(sigaction)");
}

void blockSigThreads(){
    //Mascara de signals que queremos bloquear acesso pelas threads
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    if (pthread_sigmask(SIG_BLOCK, &set, NULL) != 0) sysError("blockSigThreads(pthread_sigmask)");
}


/* A funcao devolve a diferenca entre tempo inicial e fical, em segundos*/
float time_taken(struct timeval start, struct timeval end) {
    float secs;
    float microseconds;
    
    secs = end.tv_sec - start.tv_sec; 
    microseconds = (end.tv_usec - start.tv_usec)/(float)MILLION;
    return secs + microseconds;
}

/*  Funcao que trata de acabar o servidor ordeiramente
 *  Espera pelo final de cada tarefa escrava
 *  Chamada:
 *      - Atraves do Signal (SIGINT) 
 *      - Quando o numero maximo de ligacao de clientes e atingido
 * 
 *  Apenas acessivel pela tarefa principal
 *  Acaba programa principal (Runtime exit)
 */
void endServer(){
    int i;
    for(i = 0; i < idx; i++)
        if(pthread_join(slave_threads[i], NULL)) sysError("endServer(pthread_join)");

    //tempo de fim
    if(gettimeofday(&end, NULL)) errnoPrint(); 
    printf("TecnicoFS completed in %0.04f seconds.\n", time_taken(start, end));

    // imprime o conteudo final da fs para o ficheiro de saida
    print_HashTab_tree(outputFile, hash_tab, numberBuckets);
     
    // liberta a memoria alocada ao longo do programa
    endRuntime(); 
                                        
    exit(EXIT_SUCCESS);
}


