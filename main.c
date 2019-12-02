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

//Vetor que guarda o numero de clientes que tem um determinado ficheiro aberto
int opened_files[INODE_TABLE_SIZE]; 

lock idx_lock;  //mutex para a variavel idx
lock of_lock;   //mutex para o vetor opened_files

int sockfd;     //descritor de ficheiro
struct timeval start, end; //valores de tempo inicial e final




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

//========================
//Funcoes de Inicializacao
//========================
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

void initOpenedFilesCounter() {
    int i = 0;
    for(i = 0; i < INODE_TABLE_SIZE; i++) {
        opened_files[i] = 0;
    }
}

void clearClientOpenedFilesCounter(tecnicofs_fd *file_tab) {
    int i = 0;
    for(i = 0; i < MAX_OPENED_FILES; i++) {
        opened_files[file_tab[i].iNumber] -= 1;
    }
}

int initialize(){
    initSignal();       
    //estruturas
    initOpenedFilesCounter();
    initHashTable(numberBuckets);
    inode_table_init();

    idx_lock = (lock)malloc(sizeof(struct lock));
    initLock(idx_lock);   
    of_lock = (lock)malloc(sizeof(struct lock));
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
    closeSocket(sockfd);
    freeHashTab(numberBuckets);
    inode_table_destroy();
    free(idx_lock);
    free(of_lock);
    free(nameSocket);
    free(outputFile);
}

//==============================
//Funcoes relativas aos comandos
//==============================
/*  Funcao do servidor que trata do pedido do cliente para criar um novo ficheiro
*
*   Representacao do comando para criar um ficheiro:
*       [c, 'filename', 'permissions']
*   Retorno:
*       - Erro de preexistencia de ficheiro (ALREADY_EXISTS)
*       - Caso contrario (SUCESSO) 
*/
int commandCreate(char vec[MAX_ARGS_INPUTS][MAX_INPUT_SIZE], uid_t uid) {
    int iNumber;
    tecnicofs fs = hash_tab[searchHash(vec[0], numberBuckets)]; // arvore do nome atual
    

    closeWriteLock(fs->tecnicofs_lock); // permite leituras simultaneas, impede escrita
    if (lookup(fs, vec[0]) != -1){      // se ja existir 
        openLock(fs->tecnicofs_lock);
        return ALREADY_EXISTS;
    }

    iNumber = inode_create(uid, OWNER_PERMISSION(vec[1]), OTHER_PERMISSION(vec[1]));
    create(fs, vec[0], iNumber);
    openLock(fs->tecnicofs_lock);

    return SUCCESS;
}

/*  Funcao do servidor que trata do pedido do cliente para apagar um ficheiro
*
*   Representacao do comando para criar um ficheiro:
*       [d, 'filename']
*   Retorno:
*       - Erro de inexistencia de ficheiro (DOESNT_EXIST)
*       - Erro se o ficheiro estiver aberto (FILE_IS_OPENED)
*       - Erro se o cliente nao tiver permissao para renomear o ficheiro (PERMISSION_DENIED)
*       - Caso contrario (SUCCESS) 
*/
int commandDelete(char vec[MAX_ARGS_INPUTS][MAX_INPUT_SIZE], uid_t uid){
    int search_result;
    uid_t owner;
    tecnicofs fs = hash_tab[searchHash(vec[0], numberBuckets)]; //arvore do nome atual
    
    closeWriteLock(fs->tecnicofs_lock); // bloqueia leituras e escritas do fs

    if ((search_result = lookup(fs, vec[0])) == -1)     //se nao existir
        return DOESNT_EXIST;

    closeReadLock(of_lock);
    /* Procura no vetor de ficheiros abertos se algum cliente tem este ficheiro aberto
    Em caso afirmativo, cancela a operacao de apagar o ficheiro */
    if (opened_files[search_result] != 0) { 
        openLock(of_lock);
        openLock(fs->tecnicofs_lock);
        return FILE_IS_OPENED;
    }
    openLock(of_lock);

    inode_get(search_result, &owner, NULL, NULL, NULL, 0);

    // confirmar permissao para apagar (apenas consegue apagar o dono)
    if (uid != owner)
        return PERMISSION_DENIED;
    
    inode_delete(search_result);
    delete(fs, vec[0]); 
    openLock(fs->tecnicofs_lock);   

    return SUCCESS;
}

/*  Funcao do servidor que trata do pedido do cliente para renomear um ficheiro no servidor
*
*   Representacao do comando para criar um ficheiro:
*       [r, 'filenameOld', 'filenameNew']
*   Retorno:
*       - Erro se o novo nome ja existir (ALREADY_EXISTS)
*       - Erro de inexistencia de ficheiro antigo (DOESNT_EXIST)
*       - Erro se o cliente nao tiver permissao para renomear o ficheiro (PERMISSION_DENIED)
*       - Caso contrario (SUCCESS) 
*/
int commandRename(char vec[MAX_ARGS_INPUTS][MAX_INPUT_SIZE], uid_t uid){
	int search_result;           //inumber retornado pela funcao lookup
    uid_t owner;
    int s1, s2;
    tecnicofs fs1, fs2;

    s1 = searchHash(vec[0], numberBuckets);
    s2 = searchHash(vec[1], numberBuckets);

    if (s1 > s2) { //para impedirmos interblocagem, percorremos hash_tab sempre na mesma direcao
        fs1 = hash_tab[s2]; //arvore do nome atual
        fs2 = hash_tab[s1]; //arvore onde vai ser inserido o ficheiro com o novo nome
    } else {
        fs1 = hash_tab[s1]; //arvore do nome atual
        fs2 = hash_tab[s2]; //arvore onde vai ser inserido o ficheiro com o novo nome
    }
    
    closeWriteLock(fs2->tecnicofs_lock);
    if (lookup(fs2, vec[1]) != -1) { // se o novo nome ja existir, a operacao e' cancelada sem devolver erro
		openLock(fs2->tecnicofs_lock);
		return ALREADY_EXISTS;
    }
    if (fs1 != fs2) closeWriteLock(fs1->tecnicofs_lock);

    if ((search_result = lookup(fs1, vec[0])) == -1) { // se o ficheiro atual nao existir, a operacao e' cancelada
		openLock(fs1->tecnicofs_lock);
		if (fs1 != fs2) openLock(fs2->tecnicofs_lock); // nao fazer unlock 2 vezes da mesma arvore
		return DOESNT_EXIST;
	}

    inode_get(search_result, &owner, NULL, NULL, NULL, 0);
    if(uid != owner){
        openLock(fs1->tecnicofs_lock);
        if (fs1 != fs2) openLock(fs2->tecnicofs_lock); // nao fazer unlock 2 vezes da mesma arvore
        return PERMISSION_DENIED;
    }

    delete(fs1, vec[0]);
	if (fs1 != fs2) openLock(fs1->tecnicofs_lock);
	// se as fs forem iguais nao podemos fazer unlock antes do create
	create(fs2, vec[1], search_result); // novo ficheiro: novo nome, mesmo Inumber
	openLock(fs2->tecnicofs_lock);

    return SUCCESS;
}


/*  Funcao do servidor que trata do pedido do cliente para abrir um ficheiro
*
*   Representacao do comando para criar um ficheiro:
*       [o, 'filename', mode]
*   Retorno:
*       - Erro de inexistencia de ficheiro (DOESNT_EXISTS)
*       - Erro se o cliente nao tiver permissao para abrir o ficheiro (PERMISSION_DENIED)
*       - Erro se nao houver posicoes livres na tabela de ficheiros para abrir um novo ficheiro (MAX_OPENED_FILES)
*       - Caso contrario, retorna o fd onde o ficheiro ficou aberto 
*/
int commandOpen(char vec[MAX_ARGS_INPUTS][MAX_INPUT_SIZE], uid_t uid, tecnicofs_fd *file_tab){
    int search_result, idx_fd;
    permission ownerp, otherp, mode;
    uid_t user;
    tecnicofs fs = hash_tab[searchHash(vec[0], numberBuckets)]; //arvore do nome atual

    closeReadLock(fs->tecnicofs_lock);    // permite leituras simultaneas, impede escrita
    
    if ((search_result = lookup(fs, vec[0])) == -1){     //se nao existir
        openLock(fs->tecnicofs_lock);
        return DOESNT_EXIST;
    }

    inode_get(search_result, &user, &ownerp, &otherp, NULL, 0);
    openLock(fs->tecnicofs_lock);

    mode = atoi(vec[1]);
    //consideramos que a permissao dos outros nao se aplica ao dono do ficheiro
    if (!(OWNER_HAS_PERMISSION(user, uid, mode, ownerp) || OTHER_HAS_PERMISSION(user, uid, mode, otherp))){
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
//=====================



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

    //tempo de fim
    if(gettimeofday(&end, NULL)) errnoPrint(); 
    printf("TecnicoFS completed in %0.04f seconds.\n", time_taken(start, end));

    print_tree_outfile();                            // imprime o conteudo final da fs para o ficheiro de saida
    liberator();                                     // liberta a memoria alocada ao longo do programa
    
    exit(EXIT_SUCCESS);
}


//==================
//Funcoes principais
//==================

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
                closeWriteLock(of_lock);
                clearClientOpenedFilesCounter(fd_table);
                openLock(of_lock);
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