#include <unistd.h>
#include <stdio.h>

int main(void)
{
    printf("ID:", getpid());
    printf("parentid", getppid());
    return 0;
}