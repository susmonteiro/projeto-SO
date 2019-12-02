#include "tecnicofs-client-api.h"

int sockfd = NOT_CONNECTED; //inicialmente o cliente nao esta conectado

/*  Funcao utilizada pelo cliente para criar um novo ficheiro no servidor
*
*   Representacao do comando para criar um ficheiro:
*       "c 'filename' 'permissions'\0"
*   Retorno:
*       - Erro se nao existir conexao (TECNICOFS_ERROR_NO_OPEN_SESSION)
*       - Erro se o nome passado for vazio (TECNICOFS_ERROR_OTHER)
*       - Erro de preexistencia de ficheiro (TECNICOFS_ERROR_FILE_ALREADY_EXISTS)
*       - Erro se nao for possivel criar o ficheiro (TECNICOFS_ERROR_OTHER)
*       - Caso contrario (SUCCESS) 
*/
int tfsCreate(char *filename, permission ownerPermissions, permission othersPermissions){
    if(!filename) return TECNICOFS_ERROR_OTHER; //verifica se filename e' vazio

    int size = strlen(filename) + PADDING_COMMAND_C;
    char command[size];
    int answer;

    CHECK_CONNECTED();

    if (sprintf(command, "%c %s %d%d", CREATE_COMMAND, filename, ownerPermissions, othersPermissions) != size-1) sysError("tfsCreate(sprintf)");
    // escreve o comando
    if (write(sockfd, command, size) != size) sysError("tfsCreate(write)");
    // le resposta ao comando
    if (read(sockfd, &answer, INT_SIZE) != INT_SIZE) sysError("tfsCreate(read)");
    
    if (answer == ALREADY_EXISTS) return TECNICOFS_ERROR_FILE_ALREADY_EXISTS; 
    if (answer == MAX_FILES_INODE_TABLE) return TECNICOFS_ERROR_OTHER;
    
    return SUCCESS;
}

/*  Funcao utilizada pelo cliente para apagar um ficheiro no servidor
*
*   Representacao do comando para criar um ficheiro:
*       "d 'filename'\0"
*   Retorno:
*       - Erro se nao existir conexao (TECNICOFS_ERROR_NO_OPEN_SESSION)
*       - Erro se o nome passado for vazio (TECNICOFS_ERROR_OTHER)
*       - Erro de inexistencia de ficheiro (TECNICOFS_ERROR_FILE_NOT_FOUND)
*       - Erro se o ficheiro estiver aberto (TECNICOFS_ERROR_FILE_IS_OPEN)
*       - Erro se os argumentos nao forem validos (TECNICOFS_ERROR_OTHER)
*       - Erro se o cliente nao tiver permissao para renomear o ficheiro (TECNICOFS_ERROR_PERMISSION_DENIED)
*       - Caso contrario (SUCCESS) 
*/
int tfsDelete(char *filename) {
    if(!filename) return TECNICOFS_ERROR_OTHER; //verifica se filename e' vazio

    int size = strlen(filename) + PADDING_COMMAND_D;
    char command[size];
    int answer;

    CHECK_CONNECTED();

    if (sprintf(command, "%c %s", DELETE_COMMAND, filename) != size-1) sysError("tfsDelete(sprintf)");

    // escreve o comando
    if (write(sockfd, command, size) != size) sysError("tfsDelete(write)");
    // le resposta ao comando
    if (read(sockfd, &answer, INT_SIZE) != INT_SIZE) sysError("tfsDelete(read)");

    if (answer == DOESNT_EXIST) return TECNICOFS_ERROR_FILE_NOT_FOUND; 
    if (answer == FILE_IS_OPENED) return TECNICOFS_ERROR_FILE_IS_OPEN;
    if (answer == PERMISSION_DENIED) return TECNICOFS_ERROR_PERMISSION_DENIED; 
    if (answer == INVALID_ARGS) return TECNICOFS_ERROR_OTHER;

    return SUCCESS;
}


/*  Funcao utilizada pelo cliente para renomear um ficheiro no servidor
*
*   Representacao do comando para criar um ficheiro:
*       "r 'filenameOld 'filenameNew'"
*   Retorno:
*       - Erro se nao existir conexao (TECNICOFS_ERROR_NO_OPEN_SESSION)
*       - Erro se um dos nomes passados for vazio (TECNICOFS_ERROR_OTHER)
*       - Erro de inexistencia de ficheiro antigo (TECNICOFS_ERROR_FILE_NOT_FOUND)
*       - Erro se o novo nome ja existir (TECNICOFS_ERROR_ALREADY_EXISTS)
*       - Erro se o cliente nao tiver permissao para renomear o ficheiro (TECNICOFS_ERROR_PERMISSION_DENIED)
*       - Erro se os argumentos nao forem validos (TECNICOFS_ERROR_OTHER)
*       - Caso contrario (SUCCESS) 
*/
int tfsRename(char *filenameOld, char *filenameNew){
    if(!filenameNew || !filenameOld) return TECNICOFS_ERROR_OTHER;  //verifica se filenameOld e/ou filenameNew sao vazios

    int size = strlen(filenameOld) + strlen(filenameNew) + PADDING_COMMAND_R;
    char command[size];
    int answer;

    CHECK_CONNECTED();


    if (sprintf(command, "%c %s %s", RENAME_COMMAND, filenameOld, filenameNew) != size-1) sysError("tfsRename(sprintf)");

    // escreve o comando
    if (write(sockfd, command, size) != size) sysError("tfsRename(write)");
    // le resposta ao comando
    if (read(sockfd, &answer, INT_SIZE) != INT_SIZE) sysError("tfsRename(read)");

    if (answer == DOESNT_EXIST) return TECNICOFS_ERROR_FILE_NOT_FOUND; 
    if (answer == ALREADY_EXISTS) return TECNICOFS_ERROR_FILE_ALREADY_EXISTS; 
    if (answer == PERMISSION_DENIED) return TECNICOFS_ERROR_PERMISSION_DENIED; 
    if (answer == INVALID_ARGS) return TECNICOFS_ERROR_OTHER;

    return SUCCESS;
}

/*  Funcao utilizada pelo cliente para abrir um ficheiro
*
*   Representacao do comando para criar um ficheiro:
*       “o 'filename' mode”
*   Retorno:
*       - Erro se nao existir conexao (TECNICOFS_ERROR_NO_OPEN_SESSION)
*       - Erro caso o ficheiro ja esteja aberto (TECNICOFS_ERROR_FILE_IS_OPEN)
*       - Erro se um dos nomes passados for vazio (TECNICOFS_ERROR_OTHER)
*       - Erro de inexistencia de ficheiro (TECNICOFS_ERROR_FILE_NOT_FOUND)
*       - Erro se o cliente nao tiver permissao para abrir o ficheiro (TECNICOFS_ERROR_PERMISSION_DENIED)
*       - Erro se nao houver posicoes livres na tabela de ficheiros para abrir um novo ficheiro (TECNICOFS_ERROR_MAXED_OPEN_FILES)
*       - Erro se os argumentos nao forem validos (TECNICOFS_ERROR_OTHER)
*       - Caso contrario, retorna o fd onde o ficheiro ficou aberto 
*/
int tfsOpen(char *filename, permission mode){
    if(!filename) return TECNICOFS_ERROR_OTHER; //verifica se filename e' vazio


    int size = strlen(filename) + PADDING_COMMAND_O;
    char command[size];
    int answer;

    CHECK_CONNECTED();

    if (sprintf(command, "%c %s %d", OPEN_COMMAND, filename, mode) != size-1) sysError("tfsOpen(sprintf)");
    
    // escreve o comando
    if (write(sockfd, command, size) != size) sysError("tfsOpen(write)");
    // le resposta ao comando
    if (read(sockfd, &answer, INT_SIZE) != INT_SIZE) sysError("tfsOpen(read)");

    if (answer == DOESNT_EXIST) return TECNICOFS_ERROR_FILE_NOT_FOUND; 
    if (answer == ALREADY_OPENED) return TECNICOFS_ERROR_FILE_IS_OPEN;
    if (answer == PERMISSION_DENIED) return TECNICOFS_ERROR_PERMISSION_DENIED;
    if (answer == MAX_OPENED_FILES) return TECNICOFS_ERROR_MAXED_OPEN_FILES;
    if (answer == INVALID_ARGS) return TECNICOFS_ERROR_OTHER; 
        
    return answer;
}

/*  Funcao utilizada pelo cliente para fechar um ficheiro
*
*   Representacao do comando para criar um ficheiro:
*       “x fd”
*   Retorno:
*       - Erro se nao existir conexao (TECNICOFS_ERROR_NO_OPEN_SESSION)
*       - Erro se o ficheiro nao estiver aberto (TECNICOFS_ERROR_FILE_NOT_OPEN)
*       - Erro se fd nao for uma posicao valida da tabela de ficheiros (TECNICOFS_ERROR_OTHER)
*       - Erro se os argumentos nao forem validos (TECNICOFS_ERROR_OTHER)
*       - Caso contrario (SUCCESS)
*/
int tfsClose(int fd){
    int size_fd = snprintf(NULL, 0, "%d", fd);  //numero de caracteres que fd ocupa (dependendo se tem 1, 2 ou mais algarismos)
    int size = size_fd + PADDING_COMMAND_X;
    char command[size];
    int answer;

    CHECK_CONNECTED();

    if (sprintf(command, "%c %d", CLOSE_COMMAND, fd) != size-1) sysError("tfsClose(sprintf)");

    // escreve o comando
    if (write(sockfd, command, size) != size) sysError("tfsClose(write)");
    // le resposta ao comando
    if (read(sockfd, &answer, INT_SIZE) != INT_SIZE) sysError("tfsClose(read)");

    if (answer == NOT_OPENED) return TECNICOFS_ERROR_FILE_NOT_OPEN; 
    if (answer == INDEX_OUT_OF_RANGE) return TECNICOFS_ERROR_OTHER;
    if (answer == INVALID_ARGS) return TECNICOFS_ERROR_OTHER; 


    return SUCCESS;
}

/*  Funcao utilizada pelo cliente para ler o conteudo de um ficheiro
*
*   Representacao do comando para criar um ficheiro:
*       “l fd len”
*   Retorno:
*       - Erro se o tamanho daquilo que queremos ler do ficheiro for negativo (TECNICOFS_ERROR_OTHER)
*       - Erro se nao existir conexao (TECNICOFS_ERROR_NO_OPEN_SESSION)
*       - Erro se o ficheiro nao estiver aberto (TECNICOFS_ERROR_FILE_NOT_OPEN)
*       - Erro se fd nao for uma posicao valida da tabela de ficheiros (TECNICOFS_ERROR_OTHER)
*       - Erro se o cliente nao tiver permissao para ler do ficheiro (TECNICOFS_ERROR_INVALID_MODE)
*       - Erro se os argumentos nao forem validos (TECNICOFS_ERROR_OTHER)
*       - Caso contrario, retorna o numero de caracteres lidos
*/
int tfsRead(int fd, char *buffer, int len){
    int fd_size = snprintf(NULL, 0, "%d", fd);      //numero de caracteres que fd ocupa
    int len_size = snprintf(NULL, 0, "%d", len);    //numero de caracteres que len ocupa

    int size = fd_size + len_size + PADDING_COMMAND_L;
    char command[size];
    int answer;

    if (len < 0) return TECNICOFS_ERROR_OTHER; 

    CHECK_CONNECTED();

    if (sprintf(command, "%c %d %d", READ_COMMAND, fd, len) != size-1) sysError("tfsRead(sprintf)");
    // escreve o comando
    if (write(sockfd, command, size) != size) sysError("tfsRead(write)");
    // le resposta ao comando
    if (read(sockfd, &answer, INT_SIZE) != INT_SIZE) sysError("tfsRead(readAnswer)");

    if(answer >= 0) //se positivo, answer guarda o numero de caracteres lidos excepto o \0 dai o answer+1
        if(read(sockfd, buffer, answer+1) != answer+1) sysError("tfsRead(readBuffer)");


    if (answer == NOT_OPENED) return TECNICOFS_ERROR_FILE_NOT_OPEN; 
    if (answer == INDEX_OUT_OF_RANGE) return TECNICOFS_ERROR_OTHER;
    if (answer == PERMISSION_DENIED) return TECNICOFS_ERROR_INVALID_MODE;
    if (answer == INVALID_ARGS) return TECNICOFS_ERROR_OTHER; 

    
    return answer; // numero de caracteres lidos (exclui o '\0')
}

/*  Funcao utilizada pelo cliente para escrever o conteudo de buffer num ficheiro
*
*   Representacao do comando para criar um ficheiro:
*       “w fd 'dataInBuffer'”
*   Retorno:
*       - Erro se o tamanho daquilo que queremos ler do ficheiro for negativo (TECNICOFS_ERROR_OTHER)
*       - Erro se nao existir conexao (TECNICOFS_ERROR_NO_OPEN_SESSION)
*       - Erro se o ficheiro nao estiver aberto (TECNICOFS_ERROR_FILE_NOT_OPEN)
*       - Erro se fd nao for uma posicao valida da tabela de ficheiros (TECNICOFS_ERROR_OTHER)
*       - Erro se o cliente nao tiver permissao para ler do ficheiro (TECNICOFS_ERROR_INVALID_MODE)
*       - Erro se os argumentos nao forem validos (TECNICOFS_ERROR_OTHER)
*       - Caso contrario (SUCCESS)
*/
int tfsWrite(int fd, char *buffer, int len){
    int fd_size = snprintf(NULL, 0, "%d", fd); //numero de caracteres que fd ocupa
    int buf_size = strlen(buffer);

    // conteudo concreto que queremos escrever
    if (buf_size > len) buf_size = len; // no maximo queremos que sejam escritos "len" caracteres
    char text[buf_size+1];
    strncpy(text, buffer, buf_size);
    text[buf_size] = '\0';
    
    int size = fd_size + buf_size + PADDING_COMMAND_W;
    char command[size];
    int answer; 

    if (len < 0) return TECNICOFS_ERROR_OTHER; 

    CHECK_CONNECTED();

    if (sprintf(command, "%c %d %s", WRITE_COMMAND, fd, text) != size-1) sysError("tfsWrite(sprintf)");
    // escreve o comando
    if (write(sockfd, command, size) != size) sysError("tfsWrite(write)");
    // le resposta ao comando
    if (read(sockfd, &answer, INT_SIZE) != INT_SIZE) sysError("tfsWrite(read)");

    if (answer == NOT_OPENED) return TECNICOFS_ERROR_FILE_NOT_OPEN; 
    if (answer == INDEX_OUT_OF_RANGE) return TECNICOFS_ERROR_OTHER;
    if (answer == PERMISSION_DENIED) return TECNICOFS_ERROR_INVALID_MODE;
    if (answer == INVALID_ARGS) return TECNICOFS_ERROR_OTHER; 

    return SUCCESS; 
}

/*  Funcao utilizada pelo cliente para estabelecer uma sessao com o servidor
*
*   Retorno:
*       - Erro se ja existir uma conexao (TECNICOFS_ERROR_OPEN_SESSION)
*       - Erro na conexao (TECNICO_ERROR_CONNECTION_ERROR)
*       - Caso contrario (SUCCESS)
*/
int tfsMount(char * address){
    int servlen;
    struct sockaddr_un serv_addr;

    int str_size = strlen(SOCKETDIR) + strlen(address) + 1; //+1: '\0'  
    char pwd[str_size];
    
    if(sprintf(pwd, "%s%s", SOCKETDIR, address) != str_size-1) sysError("tfsMount(pwdName)");   

    CHECK_NOT_CONNECTED();

    if((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
        sysError("tfsMount(socket)");

    bzero((char*) &serv_addr, sizeof(serv_addr));
    serv_addr.sun_family = AF_UNIX;

    strncpy(serv_addr.sun_path, pwd, sizeof(serv_addr.sun_path)-1);
    servlen = strlen(serv_addr.sun_path) + sizeof(serv_addr.sun_family);
    
    if(connect(sockfd, (struct sockaddr*) &serv_addr, servlen) < 0)
        return TECNICOFS_ERROR_CONNECTION_ERROR;

    return SUCCESS;
}

/*  Funcao utilizada pelo cliente para terminar uma sessao ativa
*
*   Retorno:
*       - Erro se nao existir uma conexao (TECNICOFS_ERROR_NO_OPEN_SESSION)
*       - Caso contrario (SUCCESS)
*/
int tfsUnmount(){
    int size = CHAR_SIZE + PADDING_COMMAND_Z; //compensar pelo '\0'
    char command[size];
    
    CHECK_CONNECTED();

    if (sprintf(command, "%c", END_COMMAND) != size-1) sysError("tfsUnmount(sprintf)");
    
    // escreve o comando
    if (write(sockfd, command, size) != size) sysError("tfsUnmount(write)");
    // le resposta ao comando
    if(close(sockfd) == -1) sysError("tfsUnmount(close)");

    sockfd = NOT_CONNECTED; // o cliente volta ao estado de "sem sessao ativa"

    return SUCCESS;
}
