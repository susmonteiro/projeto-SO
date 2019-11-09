#ifndef SYNC_H
#define SYNC_H

#include <stdlib.h>
#include <semaphore.h> 
#include <errno.h>
#include <string.h>

#include "fs.h"

void erroCheck(int returnval);
void errnoPrint();
void wOpen(tecnicofs fs);
void rOpen(tecnicofs fs);
void wClosed(tecnicofs fs);
void rClosed(tecnicofs fs);
void wOpen_rc(pthread_mutex_t *mutex);
void wClosed_rc(pthread_mutex_t *mutex);
int TryLock(tecnicofs fs);
void Unlock(tecnicofs fs);
void cria_semaforo(sem_t *sem, int initVal);
void esperar(sem_t *sem);
void assinalar(sem_t *sem);

#endif /* SYNC_H */