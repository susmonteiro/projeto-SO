#include "locks.h"

void wOpened(tecnicofs *fs) {
    #if defined(MUTEX) || defined(RWLOCK)
        pthread_mutex_unlock(&fs->mutex_ap);
    #endif
}

void rOpened(tecnicofs *fs) {
    #ifdef MUTEX
        pthread_mutex_unlock(&fs->mutex_ap);
    #elif RWLOCK
        pthread_rwlock_unlock(&fs->rwlock);
    #endif
}

void wClosed(tecnicofs *fs) { //lock do mutex e wlock
    #if defined(MUTEX) || defined(RWLOCK)
        pthread_mutex_lock(&fs->mutex_ap);
    #endif
}

void rClosed(tecnicofs *fs) {
    #ifdef MUTEX
        pthread_mutex_lock(&fs->mutex_ap);
    #elif RWLOCK
        pthread_rwlock_rdlock(&fs->rwlock);
    #endif
}

void wOpened_rc(tecnicofs *fs) {
    #if defined(MUTEX) || defined(RWLOCK)
        pthread_mutex_unlock(&fs->mutex_rm);
    #endif
}

void wClosed_rc(tecnicofs *fs) { //lock do mutex e wlock
    #if defined(MUTEX) || defined(RWLOCK)
        pthread_mutex_lock(&fs->mutex_rm);
    #endif
}