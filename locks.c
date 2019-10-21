#include "locks.h"

/* Optou-se por criar duas funcoes write_open e write_close:
    - uma implementacao para do vetor de comandos
    - outra para a execucao dos comandos c (create) e d (delete)
   Assim permite-se que haja uma tarefa a carregar do vetor e outra ja a executar um comando

   Por outro lado, o lock para os comandos c e d e' o mesmo visto que ambos os comandos editam a arvore.
   Desta forma impedimos que sejam executados em simultaneo.
*/

/* Erro caso um lock ou unlock nao seja sucedido 
    (isto e', caso a respetiva funcao retorne um valor diferente de 0) */
void erroCheck(int returnval){
    if(returnval != 0){
        fprintf(stderr, "Error: lock malfunction\n");
        exit(EXIT_FAILURE);
    }
}
/* Fecha o write_lock do acesso ao vetor de comandos */ 
void wClosed_rc(pthread_mutex_t *mutex) {
    #if defined(MUTEX) || defined(RWLOCK)
        erroCheck(pthread_mutex_lock(mutex));
    #endif
}

/* Abre o write_lock do acesso ao vetor de comandos */
void wOpened_rc(pthread_mutex_t *mutex) { 
    #if defined(MUTEX) || defined(RWLOCK)
        erroCheck(pthread_mutex_unlock(mutex));
    #endif
}


/* Fecha o write_lock dos comandos que editam a arvore */
void wClosed(tecnicofs fs) {
    #ifdef MUTEX
        erroCheck(pthread_mutex_lock(&fs->mutex_ap));
    #elif RWLOCK
        erroCheck(pthread_rwlock_wrlock(&fs->rwlock));
    #endif
}

/* Fecha o read_lock do comando l*/
void rClosed(tecnicofs fs) {
    #ifdef MUTEX
        erroCheck(pthread_mutex_lock(&fs->mutex_ap));
    #elif RWLOCK
        erroCheck(pthread_rwlock_rdlock(&fs->rwlock));
    #endif
}

/* Abre o write_lock dos comandos que editam a arvore */
void wOpened(tecnicofs fs) {
    #ifdef MUTEX
        erroCheck(pthread_mutex_unlock(&fs->mutex_ap));
    #elif RWLOCK
        erroCheck(pthread_rwlock_unlock(&fs->rwlock));
    #endif
}

/* Abre o read_lock do comando l*/
void rOpened(tecnicofs fs) {
    #ifdef MUTEX
        erroCheck(pthread_mutex_unlock(&fs->mutex_ap));
    #elif RWLOCK
        erroCheck(pthread_rwlock_unlock(&fs->rwlock));
    #endif
}

