#include "sync.h"
#include <pthread.h>

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



/*Inicializa Lock (Mutex|RWlock)*/
void initLock(lock l){
    #ifdef MUTEX
        if (pthread_mutex_init(&l.mutex, NULL)) errnoPrint();  //ERROOOOOOOOOOOOOOOO
    #elif RWLOCK
        if(pthread_rwlock_init(&l.rwlock, NULL)) errnoPrint();
    #endif
}

/* void initMutex(pthread_mutex_t *mutex) {
    if (pthread_mutex_init(mutex, NULL)) errnoPrint();
}

void initRWLock(pthread_rwlock_t *rwlock) {
    if(pthread_rwlock_init(rwlock, NULL)) errnoPrint();
    
} */

/*Destroi Lock (Mutex|RWlock)*/
void destroyLock(lock l) {
    #ifdef MUTEX
        if (pthread_mutex_destroy(&l.mutex)) errnoPrint();
    #elif RWLOCK
        if (pthread_rwlock_destroy(&l.rwlock)) errnoPrint();
    #endif
}

/* void destroyMutex(pthread_mutex_t *mutex) {
    if (pthread_mutex_destroy(mutex)) errnoPrint();
}

void destroyRWLock(pthread_rwlock_t *rwlock) {
    if(pthread_rwlock_destroy(rwlock)) errnoPrint();
} */

/* Fecha o write_lock do acesso ao vetor de comandos */ 
/* void closeMutex(pthread_mutex_t *mutex) {
    #if defined(MUTEX) || defined(RWLOCK)
        erroCheck(pthread_mutex_lock(mutex));
    #endif
} */

/* Abre o write_lock do acesso ao vetor de comandos */
/* void openMutex(pthread_mutex_t *mutex) { 
    #if defined(MUTEX) || defined(RWLOCK)
        erroCheck(pthread_mutex_unlock(mutex));
    #endif
} */

/* void closeWriteLock(pthread_rwlock_t *rwlock) {
    #if defined RWLOCK
        erroCheck(pthread_rwlock_wrlock(rwlock));
    #endif
}

void closeReadLock(pthread_rwlock_t *rwlock) {
    #if defined RWLOCK
        erroCheck(pthread_rwlock_rdlock(rwlock));
    #endif
}

void openLock(pthread_rwlock_t *rwlock) {
    #if defined RWLOCK
        erroCheck(pthread_rwlock_unlock(rwlock));
    #endif
} */

/* Fecha o write_lock dos comandos que editam a arvore */
void closeWriteLock(lock l) {
    #ifdef MUTEX
        erroCheck(pthread_mutex_lock(&l.mutex));
    #elif RWLOCK
        erroCheck(pthread_rwlock_wrlock(&l.rwlock));
    #endif
}

/* Fecha o read_lock do comando l*/
void closeReadLock(lock l) {
    #ifdef MUTEX
        erroCheck(pthread_mutex_lock(&l.mutex));
    #elif RWLOCK
        erroCheck(pthread_rwlock_rdlock(&l.rwlock));
    #endif
}

/* Abre o rw_lock dos comandos que editam a arvore */
void openLock(lock l) {
    #ifdef MUTEX
        erroCheck(pthread_mutex_unlock(&l.mutex));
    #elif RWLOCK
        erroCheck(pthread_rwlock_unlock(&l.rwlock));
    #endif
}


int TryLock(lock l) {
    #if defined(MUTEX)
        return !pthread_mutex_trylock(&l.mutex);
    #elif RWLOCK
        return !pthread_rwlock_trywrlock(&l.rwlock);
    #endif
        return 1; // para nosync, nunca se faz o lock 
}