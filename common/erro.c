#include "erro.h"


void errorParse() {
    fprintf(stderr, "Error: command invalid\n");
    exit(EXIT_FAILURE);
}

void sysError(const char* msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

/* Erro caso um lock ou unlock nao seja sucedido 
    (isto e', caso a respetiva funcao retorne um valor diferente de 0) */
void erroCheck(int returnval){
    if(returnval != 0){
        fprintf(stderr, "Error: lock malfunction\n");
        exit(EXIT_FAILURE);
    }
}

/* funcao que imprime erro em funcoes que tenham errno definido */
void errnoPrint(){
    fprintf(stderr, "Error: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
}

