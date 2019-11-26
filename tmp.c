#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>

int main() {
    size_t oito;
    int fd = fopen("tmp1","r");
    int size = recv(fd, &oito, sizeof(int*), 0);
    printf("%d %d", oito, size);
    if (oito == size) printf("it works");
    return 0;
}