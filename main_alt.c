#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include "fs.h"

#define MAX_COMMANDS 150000
#define MAX_INPUT_SIZE 100

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
    if (argc != 1) {
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
        numberCommands--;
        return inputCommands[headQueue++];  //incrementa o indice
    }
    return NULL;
}

void errorParse(){
    fprintf(stderr, "Error: command invalid\n");
    //exit(EXIT_FAILURE);
}

void processInput(){
    char line[MAX_INPUT_SIZE];

    while (fgets(line, sizeof(line)/sizeof(char), stdin)) {
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
}

void applyCommands(){
    while(numberCommands > 0){ //enquanto houver comandos
        const char* command = removeCommand(); 
        if (command == NULL){ //salvaguarda
            continue; //nova iteracao do while
        }

        char token;
        char name[MAX_INPUT_SIZE];
        int numTokens = sscanf(command, "%c %s", &token, name);
        if (numTokens != 2) {
            fprintf(stderr, "Error: invalid command in Queue\n");
            exit(EXIT_FAILURE);
        }

        int searchResult;
        int iNumber;
        switch (token) {
            case 'c':
                iNumber = obtainNewInumber(fs);
                create(fs, name, iNumber);
                break;
            case 'l':
                searchResult = lookup(fs, name);
                if(!searchResult)
                    printf("%s not found\n", name);
                else
                    printf("%s found with inumber %d\n", name, searchResult);
                break;
            case 'd':
                delete(fs, name);
                break;
            default: { /* error */
                fprintf(stderr, "Error: command to apply\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

int main(int argc, char* argv[]) {
    parseArgs(argc, argv);

    fs = new_tecnicofs(); //cria o fs (vazio)
    processInput();
    applyCommands();
    print_tecnicofs_tree(stdout, fs);

    free_tecnicofs(fs);
    exit(EXIT_SUCCESS);
}
