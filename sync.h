#ifndef SYNC_H
#define SYNC_H

#include <stdlib.h>
#include <errno.h>
#include <semaphore.h> 
#include <string.h>

#include "fs.h"

void erroCheck(int returnval);
void errnoPrint();
/* void initMutex(pthread_mutex_t *mutex);
void initRWLock(pthread_rwlock_t *rwlock); */ 
void initLock(lock l);
/* void destroyMutex(pthread_mutex_t *mutex); 
void destroyRWLock(pthread_rwlock_t *rwlock);  */
void destroyLock(lock l); 
/* void closeMutex(pthread_mutex_t *mutex); 
void openMutex(pthread_mutex_t *mutex);  */
void closeWriteLock(lock l); 
void closeReadLock(lock l); 
void openLock(lock l); 
/* void fsCloseWriteLock(tecnicofs fs); 
void fsCloseReadLock(tecnicofs fs); 
void fsOpenLock(tecnicofs fs);  */
int TryLock(lock l);
void Unlock(lock l); 

#endif /* SYNC_H */