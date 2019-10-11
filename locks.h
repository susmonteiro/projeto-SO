#ifndef LOCKS_H
#define LOCKS_H

#include <stdlib.h>
#include "fs.h"

void wOpened(tecnicofs *fs);
void rOpened(tecnicofs *fs);
void wClosed(tecnicofs *fs);
void rClosed(tecnicofs *fs);
void wOpened_rc(tecnicofs *fs);
void wClosed_rc(tecnicofs *fs);

#endif /* LOCKS_H */