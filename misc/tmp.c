#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>

int main() {
    char buffer[0];
    printf("%d\n", sizeof(buffer));
    buffer[0]='\0';
    // strcpy(buffer, '\0');
    return 0;
}