#ifndef SOCKET_H
#define SOCKET_H

#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <limits.h>
#include <unistd.h>

#include "fs.h"
#include "sync.h"
#include "erro.h"
#include "constants.h"

#define MAXCONNECTIONS SOMAXCONN



#endif /* SOCKET_H */