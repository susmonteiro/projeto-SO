#ifndef CONSTANTS_H
#define CONSTANTS_H

#define END_COMMAND 'z' //Comando designado para terminar a ligacao cliente-servidor
#define CREATE_COMMAND 'c'
#define DELETE_COMMAND 'd'
#define RENAME_COMMAND 'r'
#define OPEN_COMMAND 'o'

#define CHAR_SIZE 1
#define INT_SIZE 4

#define SUCCESS 0
#define ALREADY_EXISTS -1
#define DOESNT_EXIST -2
#define PERMISSION_DENIED -3
#define MAX_OPENED_FILES -4

#endif /* CONSTANTS_H */