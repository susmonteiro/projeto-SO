#include "sync.h"


/*Inicializa lock (Mutex|RWlock)*/
void initLock(lock l){
    #ifdef MUTEX
        if (pthread_mutex_init(&l->mutex, NULL)) errnoPrint();  
    #elif RWLOCK
        if(pthread_rwlock_init(&l->rwlock, NULL)) errnoPrint();
    #endif
}


/*Destroi lock (Mutex|RWlock)*/
void destroyLock(lock l) {
    #ifdef MUTEX
        if (pthread_mutex_destroy(&l->mutex)) errnoPrint();
    #elif RWLOCK
        if (pthread_rwlock_destroy(&l->rwlock)) errnoPrint();
    #endif
}

/* Fecha o lock para escrita */
void closeWriteLock(lock l) {
    #ifdef MUTEX
        erroCheck(pthread_mutex_lock(&l->mutex));
    #elif RWLOCK
        erroCheck(pthread_rwlock_wrlock(&l->rwlock));
    #endif
}

/* Fecha o lock para leitura */
void closeReadLock(lock l) {
    #ifdef MUTEX
        erroCheck(pthread_mutex_lock(&l->mutex));
    #elif RWLOCK
        erroCheck(pthread_rwlock_rdlock(&l->rwlock));
    #endif
}

/* Abre o lock quer para escrita, quer para leitura */
void openLock(lock l) {
    #ifdef MUTEX
        erroCheck(pthread_mutex_unlock(&l->mutex));
    #elif RWLOCK
        erroCheck(pthread_rwlock_unlock(&l->rwlock));
    #endif
}