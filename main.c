#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/time.h>
#include "fs.h"


#define MAX_COMMANDS 150000
#define MAX_INPUT_SIZE 100
#define MILLION 1000000


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
    if (argc != 4) {
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
    //exit(EXIT_FAILURE);
}

void errorFile(){
    fprintf(stderr, "Error: %s\n", strerror(errno));
    //exit(EXIT_FAILURE);
}

void processInput(const char *pwd){
    char line[MAX_INPUT_SIZE];
    FILE *fp = fopen(pwd, "r");

    if (!fp){
        errorFile();
        return;
    }
    
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
                break; /* se for um comentario, ignora essa linha? */
            default: { /* error */
                errorParse();
            }
        }
    }

    fclose(fp);
}

void opened() {
    pthread_mutex_unlock(&fs->mutex);
}

void closed() {
    pthread_mutex_lock(&fs->mutex);

}

void applyCommands(){
    closed();
    while(numberCommands > 0){ //enquanto houver comandos
        const char* command = removeCommand(); //pop do comando
        if (command == NULL){ //salvaguarda
            continue; //nova iteracao do while
        }

        char token;
        char name[MAX_INPUT_SIZE];
        int numTokens = sscanf(command, "%c %s", &token, name); //scanf formatado "comando nome"
        if (numTokens != 2) { //todos os comandos levam 1 input
            fprintf(stderr, "Error: invalid command in Queue\n");
            exit(EXIT_FAILURE);
        }

        int searchResult;
        int iNumber;
        switch (token) {
            case 'c':
            //w lock
                iNumber = obtainNewInumber(fs); //obter novo inumber (sequencial)
                create(fs, name, iNumber); // :))) adiciona um novo no (bst style) 
            // unlock
                break;
            case 'l':
            // r lock
                searchResult = lookup(fs, name); //procura por nome, devolve inumber
            // unlock
                if(!searchResult)
                    printf("%s not found\n", name);
                else
                    printf("%s found with inumber %d\n", name, searchResult);
                break;
            case 'd':
            // w lock
                delete(fs, name); // delete (bst style)
            // unlock
                break;
            default: { /* error */
                fprintf(stderr, "Error: command to apply\n");
                exit(EXIT_FAILURE);
            }
        }
    }
    opened();
}

void print_tree_outfile(const char *pwd) {
    FILE *fpout = fopen(pwd, "w");

    if (!fpout) {
        errorFile();
        return;
    }

    print_tecnicofs_tree(fpout, fs); //imprimir a arvore
    fclose(fpout);
}

float time_taken(struct timeval start, struct timeval end) {
    float secs;
    float microseconds;
    
    secs = end.tv_sec - start.tv_sec; 
    microseconds = (end.tv_usec - start.tv_usec)/(float)MILLION;
    return secs + microseconds;
} 

void threads_init(int n) {
    int i;
    if (n < 1)
    // erroooo
        exit(EXIT_FAILURE);
    pthread_t tid[n];
    for (i = 0; i < n; i++) {
        if (pthread_create(&tid[i], NULL, (void *)applyCommands, NULL) != 0) {
            // ERROOOOOOO
            exit(EXIT_FAILURE);
        }
    }

    for (i = 0; i < n; i++){ //terminam todas as tarefas
        pthread_join (tid[i], NULL);
    }
}

int main(int argc, char* argv[]) {
    struct timeval start, end;

    parseArgs(argc, argv);
    
    fs = new_tecnicofs(); //cria o fs (vazio)
    processInput(argv[1]);

    gettimeofday(&start, NULL); //ERROOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO
    threads_init(atoi(argv[3]));
    gettimeofday(&end, NULL);
    print_tree_outfile(argv[2]);
    printf("TecnicoFS completed in %0.4f seconds.\n", time_taken(start, end));

    free_tecnicofs(fs);
    exit(EXIT_SUCCESS);
}
