#include "../tecnicofs-api-constants.h"
#include "../tecnicofs-client-api.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>


int main(int argc, char** argv) {
     if (argc != 2) {
        printf("Usage: %s sock_path\n", argv[0]);
        exit(0);
    }
    char readBuffer[20] = {0};
    assert(tfsMount(argv[1]) == 0);
    assert(tfsCreate("abc", RW, READ) == 0);
    assert(tfsCreate("bla", RW, READ) == 0);
    assert(tfsCreate("a", READ, READ) == 0);
    assert(tfsCreate("b", RW, READ) == 0);
    assert(tfsCreate("c", RW, READ) == 0);
    assert(tfsCreate("ab", RW, READ) == 0);
    assert(tfsCreate("ac", RW, READ) == 0);
    assert(tfsCreate("bc", RW, READ) == 0);
    assert(tfsCreate("dudu", RW, READ) == 0);
    assert(tfsCreate("susu", RW, READ) == 0);
    assert(tfsCreate("hiii", RW, READ) == 0); 
// 
    int fd = -1;
    assert((fd = tfsOpen("abc", RW)) == 0);
    assert((fd = tfsOpen("bla", RW)) == 1);
    assert((fd = tfsOpen("a", READ)) == 2);
    assert((fd = tfsOpen("c", RW)) == 3);
    // fgets(readBuffer, 5, stdin);

    assert(tfsWrite(fd, "12345", 5) == 0);
    assert(tfsClose(7) == TECNICOFS_ERROR_OTHER);
    assert(tfsWrite(fd, "blica", 5) == 0);

    assert(tfsRead(fd, readBuffer, 6) == 5);
    printf("Content read: %s\n", readBuffer);

    memset(readBuffer, 0, 10*sizeof(char));
    assert(tfsClose(fd) == 0);

    assert(tfsDelete("ab")==0);

    assert(tfsRename("susu", "bubu")==0);
    assert((fd = tfsOpen("bubu", RW)) == 3);
    assert(tfsWrite(fd, "puli", 5) == 0);
    assert(tfsRead(fd, readBuffer, 6) == 4);
    printf("Content read: %s\n", readBuffer);

    memset(readBuffer, 0, 10*sizeof(char));
    assert(tfsClose(fd) == 0);
    assert(tfsClose(fd) == TECNICOFS_ERROR_FILE_NOT_OPEN);

    assert((fd = tfsOpen("c", RW)) == 3);
    assert(tfsRead(fd, readBuffer, 6) == 5);
    printf("REABERTOContent read: %s\n", readBuffer);
    memset(readBuffer, 0, 10*sizeof(char));

    assert(tfsClose(fd) == 0);
    assert((fd = tfsOpen("dudu", RW)) == 3);
    //abrir duas vezes o mesmo
    printf("atualfd:%d\n", tfsOpen("dudu", RW));
    // assert((fd = tfsOpen("dudu", RW)) == TECNICOFS_ERROR_FILE_IS_OPEN);
    
    // assert(tfsOpen("bc", RW) == TECNICOFS_ERROR_MAXED_OPEN_FILES);





    //     // assert((fd = tfsOpen("bla", RW)) == 1);
//     // assert(tfsClose(fd) == SUCCESS);
//     // assert((fd = tfsOpen("a", READ)) == 1);

// /*     assert(tfsClose(fd) == TECNICOFS_ERROR_FILE_NOT_OPEN);
//     assert(tfsClose(7) == TECNICOFS_ERROR_OTHER);
//  */
    // assert(tfsWrite(fd, "12345", 5) == 0);
    // // assert((fd = tfsOpen("susu", RW)) == 1);

    // printf("Test: read full file content");
    // assert(tfsRead(fd, readBuffer, 6) == 5);
    // printf("Content read: %s\n", readBuffer);
    // printf("Test: read only first 3 characters of file content");
    // memset(readBuffer, 0, 10*sizeof(char));
    // assert(tfsRead(fd, readBuffer, 4) == 3);
    // printf("Content read: %s\n", readBuffer);
    
    // printf("Test: read with buffer bigger than file content");
    // memset(readBuffer, 0, 10*sizeof(char));
    // // assert(tfsRead(fd, readBuffer, 10) == 5);
    // printf("\n%d\n", tfsRead(fd, readBuffer, 10));
    // printf("Content read: %s\n", readBuffer);

    // assert(tfsClose(fd) == 0);

    // printf("Test: read closed file");
    // assert(tfsRead(fd, readBuffer, 6) == TECNICOFS_ERROR_FILE_NOT_OPEN);

    // printf("Test: read file open in write mode");
    // assert((fd = tfsOpen("abc", WRITE)) == 0);
    // assert(tfsRead(fd, readBuffer, 6) == TECNICOFS_ERROR_INVALID_MODE);

    // assert(tfsClose(fd) == 0);

    // assert(tfsDelete("abc") == 0);
    assert(tfsUnmount() == 0);

    return 0;
}