#include "sync.h"

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



/*Inicializa lock (Mutex|RWlock)*/
void initLock(lock l){
    #ifdef MUTEX
        if (pthread_mutex_init(&l.mutex, NULL)) errnoPrint();  //ERROOOOOOOOOOOOOOOO
    #elif RWLOCK
        if(pthread_rwlock_init(&l.rwlock, NULL)) errnoPrint();
    #endif
}


/*Destroi lock (Mutex|RWlock)*/
void destroyLock(lock l) {
    #ifdef MUTEX
        if (pthread_mutex_destroy(&l.mutex)) errnoPrint();
    #elif RWLOCK
        if (pthread_rwlock_destroy(&l.rwlock)) errnoPrint();
    #endif
}

/* Fecha o lock para escrita */
void closeWriteLock(lock l) {
    #ifdef MUTEX
        erroCheck(pthread_mutex_lock(&l.mutex));
    #elif RWLOCK
        erroCheck(pthread_rwlock_wrlock(&l.rwlock));
    #endif
}

/* Fecha o lock para leitura */
void closeReadLock(lock l) {
    #ifdef MUTEX
        erroCheck(pthread_mutex_lock(&l.mutex));
    #elif RWLOCK
        erroCheck(pthread_rwlock_rdlock(&l.rwlock));
    #endif
}

/* Abre o lock, quer para escrita, quer para leitura */
void openLock(lock l) {
    #ifdef MUTEX
        erroCheck(pthread_mutex_unlock(&l.mutex));
    #elif RWLOCK
        erroCheck(pthread_rwlock_unlock(&l.rwlock));
    #endif
}

/*  */
int TryLock(lock l) {
    #if defined(MUTEX)
        return !pthread_mutex_trylock(&l.mutex);
    #elif RWLOCK
        return !pthread_rwlock_trywrlock(&l.rwlock);
    #endif
        return 1; 
}