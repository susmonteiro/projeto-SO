#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/time.h>
#include "fs.h"
#include "locks.h"


#define MAX_COMMANDS 150000
#define MAX_INPUT_SIZE 100
#define MILLION 1000000
#define N_ARGC 4


int numberThreads = 0;
tecnicofs* fs;

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
    }
}

int insertCommand(char* data) {
    if(numberCommands != MAX_COMMANDS) { //verifica o maximo
        strcpy(inputCommands[numberCommands++], data); 
        //passa para array e incrementa global var numberCommands
        return 1;
    }
    return 0;
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

/* funcao que imprime erro em funcoes que tenham errno definido */
void errnoPrint(){
    fprintf(stderr, "Error: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
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
                if(insertCommand(line))  
                /* se for comando valido, insere o comando no vetor de comandos */
                    break;
                return;
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
}


void applyCommands(){
    while(numberCommands > 0){ //enquanto houver comando
        int iNumber;

        wClosed_rc(fs); // impede acessos simultaneos ao vetor de comandos
        const char* command = removeCommand(); //pop do comando
        if (command == NULL){ //salvaguarda
            wOpened_rc(fs);
            continue; //nova iteracao do while
        }
        /* com base nas duvidas do piazza, caso o atual comando seja 'c' (create), e' necessario atribuir imediatamente um inumber 
        para que o mesmo ficheiro tenha sempre o mesmo inumber associado independentemente da ordem de execucao */
        if(command[0] == 'c')
            iNumber = obtainNewInumber(fs); //obter novo inumber (sequencial)
        wOpened_rc(fs);

        char token;
        char name[MAX_INPUT_SIZE];
        int numTokens = sscanf(command, "%c %s", &token, name); //scanf formatado "comando nome"
        if (numTokens != 2) { //todos os comandos levam 1 input
            fprintf(stderr, "Error: invalid command in Queue\n");
            exit(EXIT_FAILURE);
        }

        int searchResult;
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
}

/* Abre o ficheiro de output e escreve neste a arvore */
void print_tree_outfile(const char *pwd) {
    FILE *fpout = fopen(pwd, "w");

    if (!fpout) {
        errnoPrint();
        return;
    }

    print_tecnicofs_tree(fpout, fs); //imprimir a arvore
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
void threads_init() {
    struct timeval start, end; //tempo
    int i;
    pthread_t tid[numberThreads]; // lista das tarefas

    if(gettimeofday(&start, NULL))  errnoPrint(); //erro

    for (i = 0; i < numberThreads; i++) { // inicializar n tarefas, com applyCommands()
        if (pthread_create(&tid[i], NULL, (void *)applyCommands, NULL)) {
            fprintf(stderr, "Error: not able to create thread.\n");
            exit(EXIT_FAILURE);
        }
    }

    for (i = 0; i < numberThreads; i++){ //terminar todas as tarefas
        if (pthread_join (tid[i], NULL)) {
            fprintf(stderr, "Error: not able to terminate thread.\n");
            exit(EXIT_FAILURE);
        }
    }

    if(gettimeofday(&end, NULL))    errnoPrint(); //erro
    printf("TecnicoFS completed in %0.04f seconds.\n", time_taken(start, end));
}

/* Define o numero de tarefas que vao ser criadas na pool, [threads_init()]
   Numero de threads e' uma variavel global */
void startCommands(const char* nThreads){
    #if defined(MUTEX) || defined(RWLOCK)
        if((numberThreads = atoi(nThreads)) < 1) {
            fprintf(stderr, "Error: number of threads invalid.\n");
            exit(EXIT_FAILURE); 
        }
    #else   //para versao nosync (sem threads)
        if(atoi(nThreads) != 1) {
            fprintf(stderr, "Error: number of threads invalid.\n");
            exit(EXIT_FAILURE); 
        }
        numberThreads = 1; // variavel global
    #endif
    threads_init(); // inicia pool de tarefas
}

int main(int argc, char* argv[]) {
    parseArgs(argc, argv);  // verifica numero de argumentos
    
    fs = new_tecnicofs();           // cria o fs (vazio)
    processInput(argv[1]);          // carrega o vetor global com comandos
    startCommands(argv[3]);         // inicializa as tarefas e chama a funcao applyCommands()
    print_tree_outfile(argv[2]);    // imprime o conteudo final da fs para o ficheiro de saida
    
    free_tecnicofs(fs);
    exit(EXIT_SUCCESS);
}