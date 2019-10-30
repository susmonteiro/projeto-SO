#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include "fs.h"
#include "sync.h"



#define MAX_COMMANDS 10 //vetor de comandos tem no maximo 10 comandos num determinado momento
#define INITVAL_COMMAND_READER 0 //inicialmente o vetor de comandos esta vazio logo nao ha comandos para consumir0 
#define MAX_INPUT_SIZE 100
#define MILLION 1000000
#define N_ARGC 5

int numberThreads = 0;
int numberBuckets = 1;
int nextINumber = 0;
tecnicofs* hash_tab;
pthread_mutex_t mutex_rm;   //bloqueio para removeCommand()
sem_t sem_prod, sem_cons;   //semaforo para controlo do vetor comandos
int index_prod = 0;
int index_cons = 0;
pthread_t *tid_cons;
pthread_t tid_prod;


char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE]; //array de comandos
int numberCommands = 0; //numero de comandos no array
int headQueue = 0;  //indice do comando atual (processamento) 

static void displayUsage (const char* appName){
    printf("Usage: %s\n", appName);
    exit(EXIT_FAILURE);
}

static void parseArgs (long argc, char* const argv[]){
    if (argc != N_ARGC) {
        fprintf(stderr, "Invalid format:\n");
        displayUsage(argv[0]);
    } else {
        numberThreads = atoi(argv[3]); //testar erro
        numberBuckets = atoi(argv[4]);
    }

}

void insertCommand(char* data) {
    esperar(&sem_prod);
    wClosed_rc(&mutex_rm);
    strcpy(inputCommands[numberCommands++], data); 
    wOpened_rc(&mutex_rm);
    assinalar(&sem_cons);
}

char* removeCommand() {
    // pop do comando
    if((numberCommands + 1)){ //o primeiro comando e de indice 0
        if(numberCommands > 0){
            numberCommands--;
            return inputCommands[headQueue++];  //incrementa o indice
        }
    }
    return NULL;
}

void errorParse(){
    fprintf(stderr, "Error: command invalid\n");
    exit(EXIT_FAILURE);
}



void initSemaforos(int initVal1, int initVal2){
    cria_semaforo(&sem_cons,initVal1) != 0);
    cria_semaforo(&sem_prod,initVal2) != 0);
}


void processInput(const char *pwd){
    char line[MAX_INPUT_SIZE];
    FILE *fp = fopen(pwd, "r");

    if (!fp)
        errnoPrint(); // se o ficheiro nao for aberto
    
    while (fgets(line, sizeof(line)/sizeof(char), fp)){
        char token;
        char name[MAX_INPUT_SIZE];

        int numTokens = sscanf(line, "%c %s", &token, name);

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

    //pthread exit????

}


void applyCommands(){
    while(1) { //enquanto houver comando
        int iNumber;

        esperar(&sem_cons);
        wClosed_rc(&mutex_rm); // impede acessos simultaneos ao vetor de comandos
        



        //AGORA PODEM SER ESCRITOS POR CIMA! NAO PODEMOS TER O POINTER, TEMOS QUE TER O COMANDO EM SI




        const char* command = removeCommand(); //pop do comando
        if (command == NULL){ //salvaguarda
            wOpened_rc(&mutex_rm);
            abrir(&sem_prod);
            continue; //nova iteracao do while
        }
        /* com base nas duvidas do piazza, caso o atual comando seja 'c' (create), e' necessario atribuir imediatamente um inumber 
        para que o mesmo ficheiro tenha sempre o mesmo inumber associado independentemente da ordem de execucao */
        if(command[0] == 'c')
            iNumber = ++nextINumber; //obter novo inumber (sequencial)
        wOpened_rc(&mutex_rm);
        abrir(&sem_prod);

        char token;
        char name[MAX_INPUT_SIZE];
        int numTokens = sscanf(command, "%c %s", &token, name); //scanf formatado "comando nome"
        if (numTokens != 2) { //todos os comandos levam 1 input
            fprintf(stderr, "Error: invalid command in Queue\n");
            exit(EXIT_FAILURE);
        }


        int searchResult;
        tecnicofs fs = hash_tab[searchHash(name, numberBuckets)];
        switch (token) {
            case 'c':
                wClosed(fs);    // bloqueia leituras e escritas do fs
                create(fs, name, iNumber);
                wOpened(fs);
                break;
            case 'l':
                rClosed(fs);    // permite leituras simultaneas, impede escrita
                searchResult = lookup(fs, name); //procura por nome, devolve inumber
                rOpened(fs);
                if(!searchResult)
                    printf("%s not found\n", name);
                else
                    printf("%s found with inumber %d\n", name, searchResult);
                break;
            case 'd':
                wClosed(fs);    // bloqueia leituras e escritas do fs
                delete(fs, name); 
                wOpened(fs);
                break;
            default: { /* error */
                fprintf(stderr, "Error: command to apply\n");
                exit(EXIT_FAILURE);
            }
        }
    }

    //pthread exit????
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

/* A funcao devolve a diferenca entre tempo inicial e fical, em segundos*/
float time_taken(struct timeval start, struct timeval end) {
    float secs;
    float microseconds;
    
    secs = end.tv_sec - start.tv_sec; 
    microseconds = (end.tv_usec - start.tv_usec)/(float)MILLION;
    return secs + microseconds;
}

/* Inicializa a pool de tarefas que chamam a funcao applyCommands()
   Apos terminar as tarefas imprime o tempo decorrido no STDOUT */
void threads_init(const char *pwd) {
    struct timeval start, end; //tempo
    tid_cons = (pthread_t*) malloc(sizeof(pthread_t*)*(numberThreads)); // inicializa as threads consumidoras

    if(gettimeofday(&start, NULL))  errnoPrint(); //erro

    startInput(pwd);
    startCommands();
    joinAllThreads();
    

    if(gettimeofday(&end, NULL)) errnoPrint(); //erro
    printf("TecnicoFS completed in %0.04f seconds.\n", time_taken(start, end));
}

void startInput(const char *pwd) {
    if (pthread_create(tid_prod, NULL, (void *)processInput, pwd)){    // carrega o vetor global com comandos
        fprintf(stderr, "Error: not able to create thread.\n");
        exit(EXIT_FAILURE);
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

void commands_threads_init(){
    int i;
    for (i = 0; i < numberThreads; i++) { // inicializar n tarefas, com applyCommands()
        if (pthread_create(&tid_cons[i], NULL, (void *)applyCommands, NULL)) { //"+1" a thread[0] e' a produtora
            fprintf(stderr, "Error: not able to create thread.\n");
            exit(EXIT_FAILURE);
        }
    }

}

void joinAllThreads(){
    int i;
    pthread_join (tid_prod, NULL){ // mata produtora)
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


int main(int argc, char* argv[]) {
    parseArgs(argc, argv);  // verifica numero de argumentos

    hash_tab = new_tecnicofs(numberBuckets);           // cria o fs (vazio)
    initSemaforos(MAX_COMMANDS, INITVAL_COMMAND_READER);
    
    threads_init(argv[1]);             // inicializa as tarefas e chama a funcao applyCommands()
    print_tree_outfile(argv[2]);    // imprime o conteudo final da fs para o ficheiro de saida
    
    free_hashTab(hash_tab, numberBuckets);
    exit(EXIT_SUCCESS);
}