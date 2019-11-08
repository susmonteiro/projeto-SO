#include "sync.h"

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

/* funcao que imprime erro em funcoes que tenham errno definido */
void errnoPrint(){
    fprintf(stderr, "Error: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
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

int TryLock(tecnicofs fs) {
    #if defined(MUTEX)
        return pthread_mutex_trylock(&fs->mutex_ap);
    #elif RWLOCK
        return pthread_rwlock_trywrlock(&fs->rwlock);
    #endif
        return 1; // para nosync, nunca se faz o lock 
}

void Unlock(tecnicofs fs) {
    #if defined(MUTEX)
        pthread_mutex_unlock(&fs->mutex_ap);
    #elif RWLOCK
        pthread_rwlock_unlock(&fs->rwlock);
    #endif
}

void cria_semaforo(sem_t *sem, int initVal){
    if (sem_init(sem, 0, initVal)) errnoPrint();
}

void esperar(sem_t *sem) {
    if (sem_wait(sem)) errnoPrint();
}

void assinalar(sem_t *sem) {
    if (sem_post(sem)) errnoPrint();
}