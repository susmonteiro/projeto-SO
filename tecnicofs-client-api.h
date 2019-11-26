#ifndef TECNICOFS_CLIENT_API_H
#define TECNICOFS_CLIENT_API_H

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "tecnicofs-api-constants.h"

#define NOT_CONNECTED -1
#define PADDING_COMMAND_C 6
#define SUCCESS 0
#define FAIL 1

int tfsCreate(char *filename, permission ownerPermissions, permission othersPermissions);
int tfsDelete(char *filename);
int tfsRename(char *filenameOld, char *filenameNew);
int tfsOpen(char *filename, permission mode);
int tfsClose(int fd);
int tfsRead(int fd, char *buffer, int len);
int tfsWrite(int fd, char *buffer, int len);
int tfsMount(char * address);
int tfsUnmount();

#endif /* TECNICOFS_CLIENT_API_H */
