#ifndef SYNC_H
#define SYNC_H

#include <stdlib.h>
#include "erro.h"

#include "fs.h"

void initLock(lock l);
void destroyLock(lock l); 
void closeWriteLock(lock l); 
void closeReadLock(lock l); 
void openLock(lock l); 

#endif /* SYNC_H */