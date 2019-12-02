#ifndef CONSTANTS_H
#define CONSTANTS_H

#define SOCKETDIR "/tmp/"

#define END_COMMAND 'z'     //Comando designado para terminar a ligacao cliente-servidor
#define CREATE_COMMAND 'c'
#define DELETE_COMMAND 'd'
#define RENAME_COMMAND 'r'
#define OPEN_COMMAND 'o'
#define CLOSE_COMMAND 'x'
#define READ_COMMAND 'l'
#define WRITE_COMMAND 'w'


#define CHAR_SIZE 1
#define INT_SIZE 4

//mensagens de erro do servidor
#define SUCCESS 0
#define ALREADY_EXISTS -1
#define DOESNT_EXIST -2
#define PERMISSION_DENIED -3
#define MAX_OPENED_FILES -4
#define NOT_OPENED -5
#define INDEX_OUT_OF_RANGE -6
#define FILE_IS_OPENED -7
#define MAX_FILES_INODE_TABLE -8
#define INVALID_ARGS -9
#define ALREADY_OPENED -10


#endif /* CONSTANTS_H */