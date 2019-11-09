#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <ctype.h>
#include <sys/time.h>
#include "fs.h"
#include "sync.h"



#define MAX_COMMANDS 10 //vetor de comandos tem no maximo 10 comandos num determinado momento
#define INITVAL_COMMAND_READER 0 //inicialmente o vetor de comandos esta vazio logo nao ha comandos para consumir0 
#define MAX_INPUT_SIZE 100
#define MILLION 1000000
#define N_ARGC 5
#define COMMAND_NULL -1
#define END_COMMAND 'x'

int numberThreads = 0;
int numberBuckets = 1;
int nextINumber = 0;
tecnicofs* hash_tab;
pthread_mutex_t mutex_rm;   //bloqueio para removeCommand()

pthread_mutex_t mutex_HasTab; // lock para acessos a HashTab

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
    strcpy(inputCommands[index_prod], data); 
    index_prod = (index_prod + 1) % MAX_COMMANDS;

    numberCommands++; //APAGAAAAAAAR????????

    wOpen_rc(&mutex_rm);
    assinalar(&sem_cons);
}

int removeCommand(char *command) {
    // pop do comando
    int iNumber = 0;
    esperar(&sem_cons);
    wClosed_rc(&mutex_rm); // impede acessos simultaneos ao vetor de comandos
        
    strcpy(command, inputCommands[index_cons]);  //incrementa o indice
    index_cons = (index_cons + 1) % MAX_COMMANDS;

    if (command == NULL)    //salvaguarda
        iNumber = COMMAND_NULL; //nova iteracao do while

    if(command[0] == 'c')
        iNumber = ++nextINumber; //obter novo inumber (sequencial)

        
    wOpen_rc(&mutex_rm);
    assinalar(&sem_prod);

    return iNumber;
        
}

void errorParse(){
    fprintf(stderr, "Error: command invalid\n");
    exit(EXIT_FAILURE);
}



void initSemaforos(int initVal1, int initVal2){
    cria_semaforo(&sem_prod,initVal1);
    cria_semaforo(&sem_cons,initVal2);
}

void pushEndCommand(){
    char endCommand[2] = {END_COMMAND, '\0'};
    insertCommand(endCommand);
}

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
                if(numTokens != 3)
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


void applyCommands(){
    while(1) { //enquanto houver comando
        int iNumber;
        char command[MAX_INPUT_SIZE];

        iNumber = removeCommand(command); //pop do comando
        if (iNumber == COMMAND_NULL) continue;

        char token;
        char name1[MAX_INPUT_SIZE];
        char name2[MAX_INPUT_SIZE];

        int numTokens = sscanf(command, "%c %s %s", &token, name1, name2); //scanf formatado "comando nome"
        
        if (numTokens > 3 || numTokens < 1) { 
            fprintf(stderr, "Error: invalid command in Queue\n");
            exit(EXIT_FAILURE);
        } else if ((numTokens == 1 && token != END_COMMAND) || (numTokens == 3 && token != 'r')) {
            fprintf(stderr, "Error: invalid command in Queue\n");
            exit(EXIT_FAILURE);
        }


        int searchResult;
        tecnicofs fs;
        tecnicofs fs2;

        // wClosed_rc(&mutex_HasTab);
        if (numTokens > 1 && name1 != NULL) fs = hash_tab[searchHash(name1, numberBuckets)];
        if (numTokens == 3 && name2 != NULL) fs2 = hash_tab[searchHash(name2, numberBuckets)];
        // wOpen_rc(&mutex_HasTab);


        switch (token) {
            case 'c':
                wClosed(fs);    // bloqueia leituras e escritas do fs
                create(fs, name1, iNumber);
                wOpen(fs);
                break;
            case 'l':

                puts("l");

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

                // wClosed_rc(&mutex_HasTab);

                while (1) {
                    puts("r");
                    if (TryLock(fs2)) {
                        if (lookup(fs2, name2)) {
                            // printf("%s ja existe\n", name2);
                            Unlock(fs2);
                            break; //se ja existir, a operacao e' cancelada sem devolver erro
                        } else if (TryLock(fs)) {
                            if ((searchResult = lookup(fs, name1)) == 0) {
                                Unlock(fs);
                                Unlock(fs2);
                                break;
                            }

                            // wClosed_rc(&mutex_HasTab); //DEBUG

                            delete(fs, name1);
                            Unlock(fs);
                            create(fs2, name2, searchResult);
                            Unlock(fs2);

                            // wOpen_rc(&mutex_HasTab);  //DEBUG


                            break;
                        } else 
                            Unlock(fs2);
                    }
                }

                // wOpen_rc(&mutex_HasTab);
                break;
            case END_COMMAND:
                pushEndCommand();
                return;
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

void startInput(char *pwd) {
    if (pthread_create(&tid_prod, NULL, (void *)processInput, pwd)){    // carrega o vetor global com comandos
        fprintf(stderr, "Error: not able to create thread.\n");
        exit(EXIT_FAILURE);
    }
}

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


/* Inicializa a pool de tarefas que chamam a funcao applyCommands()
   Apos terminar as tarefas imprime o tempo decorrido no STDOUT */
void threads_init(char *pwd) {
    struct timeval start, end; //tempo
    tid_cons = (pthread_t*) malloc(sizeof(pthread_t*)*(numberThreads)); // inicializa as threads consumidoras
    initMutex(&mutex_rm);
    initMutex(&mutex_HasTab);

    if(gettimeofday(&start, NULL))  errnoPrint(); //erro

    startInput(pwd);
    startCommands();
    joinAllThreads();
    

    if(gettimeofday(&end, NULL)) errnoPrint(); //erro
    printf("TecnicoFS completed in %0.04f seconds.\n", time_taken(start, end));

    destroyMutex(&mutex_rm);
    destroyMutex(&mutex_HasTab);
    free(tid_cons);
}



int main(int argc, char* argv[]) {
    parseArgs(argc, argv);                                  // verifica numero de argumentos

    hash_tab = new_tecnicofs(numberBuckets);                // cria o fs (vazio)
    initSemaforos(MAX_COMMANDS, INITVAL_COMMAND_READER);
    
    threads_init(argv[1]);                                  // inicializa as tarefas e chama a funcao applyCommands()
    print_tree_outfile(argv[2]);                            // imprime o conteudo final da fs para o ficheiro de saida
    
    free_hashTab(hash_tab, numberBuckets);
    exit(EXIT_SUCCESS);
}