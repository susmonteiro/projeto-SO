#ifndef SYNC_H
#define SYNC_H

#include <stdlib.h>
#include <semaphore.h> 
#include <errno.h>
#include <string.h>

#include "fs.h"

void erroCheck(int returnval);
void errnoPrint();
void initMutex(pthread_mutex_t *mutex);
void destroyMutex(pthread_mutex_t *mutex);
void initRWLock(pthread_rwlock_t *rwlock);
void destroyRWLock(pthread_rwlock_t *rwlock);
void initLock(tecnicofs fs);
void destroyLock(tecnicofs fs);
void wOpened(tecnicofs fs);
void rOpened(tecnicofs fs);
void wClosed(tecnicofs fs);
void rClosed(tecnicofs fs);
void wOpened_rc(pthread_mutex_t *mutex);
void wClosed_rc(pthread_mutex_t *mutex);
void cria_semaforo(sem_t *sem, int initVal);
void esperar(sem_t *sem);
void assinalar(sem_t *sem);

#endif /* SYNC_H */