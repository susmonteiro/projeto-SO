#ifndef SYNC_H
#define SYNC_H

#include <stdlib.h>
#include <errno.h>
#include <semaphore.h> 
#include <string.h>

#include "fs.h"

void erroCheck(int returnval);
void errnoPrint();
void initLock(lock l);
void destroyLock(lock l); 
void closeWriteLock(lock l); 
void closeReadLock(lock l); 
void openLock(lock l); 

#endif /* SYNC_H */