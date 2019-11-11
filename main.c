#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <ctype.h>
#include <sys/time.h>
#include "fs.h"
#include "sync.h"

//==========
//Constantes
//==========
#define MAX_COMMANDS 10 //vetor de comandos tem no maximo 10 comandos num determinado momento
#define INITVAL_COMMAND_READER 0 //inicialmente o vetor de comandos esta vazio logo nao ha comandos para consumir0 
#define MAX_INPUT_SIZE 100
#define MILLION 1000000
#define N_ARGC 5
#define COMMAND_NULL -1 //Comando inexistente
#define END_COMMAND 'x' //Comando criado para terminar as threads

//=====================
//Prototipos principais
//=====================
void applyCommands();
void processInput(const char *pwd);

//=================
//Variaveis Globais
//=================
int numberThreads = 0;
int numberBuckets = 1;
int nextINumber = 0;

//Implementa Produtor-Consumidor de Comandos
pthread_t *tid_cons;
pthread_t tid_prod;

//Vetor de comandos
char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int index_prod = 0;
int index_cons = 0;

//Tabela de Arvores de ficheiros
tecnicofs* hash_tab;
pthread_mutex_t mutex_rm;   //bloqueio para removeCommand()
sem_t sem_prod, sem_cons;   //semaforo para controlo do vetor comandos


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

static void parseArgs (long argc, char* const argv[]){
    if (argc != N_ARGC) {
        fprintf(stderr, "Invalid format:\n");
        displayUsage(argv[0]);
    } else {
        numberThreads = atoi(argv[3]); 
        numberBuckets = atoi(argv[4]);
    }

}

//====================
//Funcoes de semaforos
//====================
void initSemaforos(int initVal1, int initVal2){
    create_semaforo(&sem_prod,initVal1);
    create_semaforo(&sem_cons,initVal2);
}

void destroySemaforos(){
    delete_semaforo(&sem_prod);
    delete_semaforo(&sem_cons);
}

//=================================
//Funcoes sobre o vetor de comandos
//=================================
void insertCommand(char* data) {
    esperar(&sem_prod);     // verifica se pode produzir
    wClosed_rc(&mutex_rm);  // impede acessos simultaneos ao vetor de comandos

    strcpy(inputCommands[index_prod], data); // copia o comando lido para a proxima posicao livre do vetor de comandos
    index_prod = (index_prod + 1) % MAX_COMMANDS; // incrementa posicao; vetor de comandos "circular"

    wOpen_rc(&mutex_rm);
    assinalar(&sem_cons);   // alerta o consumidor que tem algo a consumir
}

int removeCommand(char *command) {
    // pop do comando
    int iNumber = 0;
    esperar(&sem_cons);     // verifica se pode consumir
    wClosed_rc(&mutex_rm); // impede acessos simultaneos ao vetor de comandos
        
    strcpy(command, inputCommands[index_cons]);  //incrementa o indice
    index_cons = (index_cons + 1) % MAX_COMMANDS;

    if (command == NULL)    //salvaguarda
        iNumber = COMMAND_NULL; //nova iteracao do while

    if(command[0] == 'c')
        iNumber = ++nextINumber; //obter novo inumber (sequencial)

        
    wOpen_rc(&mutex_rm);
    assinalar(&sem_prod);       // alerta o produtor que pode produzir

    return iNumber;
        
}

//Introduz no vetor um commando de termino 'x'
void pushEndCommand(){
    char endCommand[2] = {END_COMMAND, '\0'};
    insertCommand(endCommand);
}


//=====================
//Funcoes sobre Arvores
//=====================

void initHashTable(int size){
    int i = 0;
	hash_tab = (tecnicofs*)malloc(size * sizeof(tecnicofs)); //alocacao da tabela para tecnicofs
	for(i = 0; i < size; i++) {
		hash_tab[i] = new_tecnicofs();  // aloca um tecnicofs (uma arvore)
		initLock(hash_tab[i]);          // inicializa trinco desse tecnicofs (arvore)

		hash_tab[i]->bstRoot = NULL;    // raiz de cada arvore
	}
}

void freeHashTab(int size){
    int i;
	for(i = 0; i < size; i++) {
		destroyLock(hash_tab[i]);
		free_tecnicofs(hash_tab[i]);
	}
	free(hash_tab);
}
//Comando renomear
void renameCommand(tecnicofs fs1, char *name1, char *name2){
	int searchResult;
    tecnicofs fs2 = hash_tab[searchHash(name2, numberBuckets)]; //name2 != null, tal como verificado na funcao applyCommands()

	while (1) {
		if (TryLock(fs2)) {
			if (lookup(fs2, name2)) { //se ja existir, a operacao e' cancelada sem devolver erro
				Unlock(fs2);
				break; 
			} else if (fs1==fs2 || TryLock(fs1)) {

                // Procurar o ficheiro pelo nome; Se existir [searchResult = Inumber do Ficheiro]
				if ((searchResult = lookup(fs1, name1)) == 0) {
					Unlock(fs1);
					if (fs1 != fs2) Unlock(fs2); // nao fazer unlock 2 vezes da mesma arvore
					break;
				}

				delete(fs1, name1);
				if (fs1 != fs2) Unlock(fs1);
				// se as fs forem iguais nao podemos fazer unlock antes do create
				create(fs2, name2, searchResult); // novo ficheiro: novo nome, mesmo Inumber
				Unlock(fs2);

				break;

			} else 
				Unlock(fs2);
		}
	}
}

/* Abre o ficheiro de output e escreve neste a arvore */
void print_tree_outfile(const char *pwd) {
    FILE *fpout = fopen(pwd, "w");

    if (!fpout) {
        errnoPrint();
        return;
    }

    print_HashTab_tree(fpout, hash_tab, numberBuckets); //imprimir a arvore
    fclose(fpout);
}


//=====================
//Funcoes sobre Tarefas
//=====================

/* A funcao devolve a diferenca entre tempo inicial e fical, em segundos*/
float time_taken(struct timeval start, struct timeval end) {
    float secs;
    float microseconds;
    
    secs = end.tv_sec - start.tv_sec; 
    microseconds = (end.tv_usec - start.tv_usec)/(float)MILLION;
    return secs + microseconds;
}

//cria a tarefa produtora
void startInput(char *pwd) {
    if (pthread_create(&tid_prod, NULL, (void *)processInput, pwd)){    // carrega o vetor global com comandos
        fprintf(stderr, "Error: not able to create thread.\n");
        exit(EXIT_FAILURE);
    }
}

//cria as tarefas consumidoras
void commands_threads_init(){
    int i;
    for (i = 0; i < numberThreads; i++) { // inicializar numberThreads tarefas, com applyCommands()
        if (pthread_create(&tid_cons[i], NULL, (void *)applyCommands, NULL)) { 
            fprintf(stderr, "Error: not able to create thread.\n");
            exit(EXIT_FAILURE);
        }
    }

}

/* Define o numero de tarefas que vao ser criadas na pool, [threads_init()]
   Numero de threads e' uma variavel global */
void startCommands() {
    #if defined(MUTEX) || defined(RWLOCK)
        if(numberThreads < 1) {
            fprintf(stderr, "Error: number of threads invalid.\n");
            exit(EXIT_FAILURE); 
        }
    #else   //para versao nosync (sem threads)
        if(numberThreads != 1) {
            fprintf(stderr, "Error: number of threads invalid.\n");
            exit(EXIT_FAILURE); 
        }
        numberThreads = 1; // variavel global
    #endif

    commands_threads_init(); // inicia pool de tarefas
}


//Funcao que trata a finalizacao das tarefas
void joinAllThreads(){
    int i;
    if (pthread_join(tid_prod, NULL)){ // mata produtora)
        fprintf(stderr, "Error: not able to terminate thread.\n");
        exit(EXIT_FAILURE);
    } 
    for (i = 0; i < numberThreads; i++){ //terminar todas as tarefas
        if (pthread_join (tid_cons[i], NULL)) {
            fprintf(stderr, "Error: not able to terminate thread.\n");
            exit(EXIT_FAILURE);
        }
    }
}


/* Inicializa a tarefa produtora e pool de tarefas (consumidoras) que chamam a funcao applyCommands()
   Apos terminar as tarefas imprime o tempo decorrido no STDOUT */
void threads_init(char *pwd) {
    struct timeval start, end; //tempo
    tid_cons = (pthread_t*) malloc(sizeof(pthread_t*)*(numberThreads)); // inicializa as threads consumidoras
    initMutex(&mutex_rm);
    initSemaforos(MAX_COMMANDS, INITVAL_COMMAND_READER);

    if(gettimeofday(&start, NULL))  errnoPrint(); 

    startInput(pwd);    //funcao que trata da tarefa produtora
    startCommands();    //funcao que trata das tarefas consumidoras
    joinAllThreads();   //funcao que trata de terminar todas as tarefas (a produtora e as consumidoras)
    

    if(gettimeofday(&end, NULL)) errnoPrint(); 
    printf("TecnicoFS completed in %0.04f seconds.\n", time_taken(start, end));

    destroySemaforos();
    destroyMutex(&mutex_rm);
    free(tid_cons);
}


//==================
//Funcoes principais
//==================
//funcao que e' chamada pela thread produtora e que copia os comandos do ficheiro para o vetor de comandos, 
// verificando se os comandos sao validos
void processInput(const char *pwd){
    char line[MAX_INPUT_SIZE];
    FILE *fp = fopen(pwd, "r");

    if (!fp)
        errnoPrint(); // se o ficheiro nao for aberto
    
    while (fgets(line, sizeof(line)/sizeof(char), fp)){
        char token;
        char name1[MAX_INPUT_SIZE];
        char name2[MAX_INPUT_SIZE];

        int numTokens = sscanf(line, "%c %s %s", &token, name1, name2);

        /* perform minimal validation */
        if (numTokens < 1) {
            continue;
        }
        switch (token) {
            case 'c':
            case 'l':
            case 'd':
                if(numTokens != 2)
                    errorParse();
                insertCommand(line);
                break;
            case 'r':
                if(numTokens != 3) //Comando r recebe 2 nomes
                    errorParse();
                insertCommand(line);
                break;
            case '#':
                break; /* se for um comentario, ignora essa linha */
            default: { /* error */
                errorParse();
            }
        }
    }

    if(fclose(fp)){ // se o ficheiro nao for corretamente fechado
        fprintf(stderr, "Error: file not closed.\n");
        exit(EXIT_FAILURE);
    }

    pushEndCommand(); //acrescenta um ultimo comando ao vetor (exit)
}

//funcao chamada pelas threads consumidoras e trata de executar os comandos do vetor de comandos
void applyCommands(){
    while(1) { //enquanto houver comandos
        int iNumber;
        char command[MAX_INPUT_SIZE];

        iNumber = removeCommand(command); //pop do comando
        if (iNumber == COMMAND_NULL) continue;

        char token;
        char name1[MAX_INPUT_SIZE];
        char name2[MAX_INPUT_SIZE];

        int numTokens = sscanf(command, "%c %s %s", &token, name1, name2); //scanf formatado "comando nome"
        
        if (numTokens > 3 || numTokens < 1) { // qualquer comando que nao tenha 1,2,3 argumentos, e' invalido
            fprintf(stderr, "Error: invalid command in Queue\n");
            exit(EXIT_FAILURE);
        } else if ((numTokens == 1 && token != END_COMMAND) || (numTokens == 3 && token != 'r')) { //teste para caso comando 'x' e 'r' 
            fprintf(stderr, "Error: invalid command in Queue\n");
            exit(EXIT_FAILURE);
        }


        int searchResult;
        tecnicofs fs;

        if (numTokens > 1 && name1 != NULL) fs = hash_tab[searchHash(name1, numberBuckets)];

        switch (token) {
            case 'c':
                wClosed(fs);    // bloqueia leituras e escritas do fs
                create(fs, name1, iNumber);
                wOpen(fs);
                break;

            case 'l':
                rClosed(fs);    // permite leituras simultaneas, impede escrita
                searchResult = lookup(fs, name1); //procura por nome, devolve inumber
                rOpen(fs);
                if(!searchResult)
                    printf("%s not found\n", name1);
                else
                    printf("%s found with inumber %d\n", name1, searchResult);
                break;

            case 'd':
                wClosed(fs);    // bloqueia leituras e escritas do fs
                delete(fs, name1); 
                wOpen(fs);
                break;

            case 'r':
                renameCommand(fs, name1, name2);
                break;

            case END_COMMAND:
                //antes de sair da funcao applyCommands(), a tarefa atual coloca um novo comando de finalizacao no vetor, 
                //para que a proxima tarefa a aceder ao vetor de comandos tambem possa terminar
                pushEndCommand(); 
                return;
                
            default: { /* error */
                fprintf(stderr, "Error: command to apply\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}




int main(int argc, char* argv[]) {
    parseArgs(argc, argv);                                  // verifica numero de argumentos

    initHashTable(numberBuckets);
    
    threads_init(argv[1]);                                  // inicializa as tarefas e chama a funcao applyCommands()
    print_tree_outfile(argv[2]);                            // imprime o conteudo final da fs para o ficheiro de saida
    
    freeHashTab(numberBuckets);
    exit(EXIT_SUCCESS);
}