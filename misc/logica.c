#include <stdio.h>

int main() {
    int i, j;
    for (i=1;i<4;i++) {
        for(j=1;j<4;j++)
            printf("%d-%d: %d\t", i, j, i&j);
        printf("\n");
    }
    return 0;
}