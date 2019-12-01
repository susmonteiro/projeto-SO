#ifndef ERRO_H
#define ERRO_H

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>


void errorParse();
void sysError(const char* msg);
void erroCheck(int returnval);
void errnoPrint();

#endif /* FS_H */