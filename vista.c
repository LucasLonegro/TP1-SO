// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef DEBUG
#define D(...)
#else
#define D(...) fprintf(stderr, __VA_ARGS__)
#endif

int main(int argc, char *argv[])
{
    sleep(3);

    char shmName[255] = {0};

    if (argc > 1)
    {
        strncpy(shmName, argv[1], sizeof(shmName) - 1);
    }
    else
    {
        int n = read(STDIN_FILENO, shmName, sizeof(shmName) - 1);
        if (n < 1)
        {
            perror("read");

            // Nothing to free

            exit(8);
        }

        shmName[n - 1] = 0;
    }

    int shmid = shm_open(shmName, O_RDONLY, 0666);
    if (shmid < 0)
    {
        perror("shm_open");

        // Nothing to free

        exit(9);
    }

    char buffer[4096];
    while (read(shmid, buffer, 4096))
    {
        printf("%s", buffer);
    }

    exit(0);
}
