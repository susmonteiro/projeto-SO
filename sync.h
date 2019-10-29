#ifndef SYNC_H
#define SYNC_H

#include <stdlib.h>
#include <semaphore.h> 
#include "fs.h"


void wOpened(tecnicofs fs);
void rOpened(tecnicofs fs);
void wClosed(tecnicofs fs);
void rClosed(tecnicofs fs);
void wOpened_rc(pthread_mutex_t *mutex);
void wClosed_rc(pthread_mutex_t *mutex);
int cria_semaforo(sem_t *sem, int initVal);

#endif /* SYNC_H */