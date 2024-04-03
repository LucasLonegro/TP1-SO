// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include <fcntl.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#define BUFFER_SIZE 8193
#define SHM_SIZE 0x4000000
#define SHM_NAME_SIZE 255

#ifndef DEBUG
#define D(...)
#else
#define D(...) fprintf(stderr, __VA_ARGS__)
#endif

#define CATCH_IF(eval, stage, strerr, status) \
    if (eval)                                 \
    {                                         \
        perror(strerr);                       \
        switch (stage)                        \
        {                                     \
        case 2:                               \
            munmap(data, SHM_SIZE);           \
            /* fallthrough */                 \
        case 1:                               \
            close(shmid);                     \
            /* fallthrough */                 \
        default:                              \
            break;                            \
        }                                     \
        exit(status);                         \
    }

typedef struct shared_data
{
    sem_t semData, semExit;
    char content[SHM_SIZE - 2 * sizeof(sem_t)];
} shared_data;

int main(int argc, char *argv[])
{
    int shmid;
    shared_data *data;

    char shmName[SHM_NAME_SIZE] = {0};

    if (argc > 1)
    {
        strncpy(shmName, argv[1], sizeof(shmName) - 1);
    }
    else
    {
        int n = read(STDIN_FILENO, shmName, sizeof(shmName) - 1);
        CATCH_IF(n < 0, 0, "read", 1);
        CATCH_IF(n < 1, 0, "stdin EOF", 0);

        // Assert null terminator
        shmName[n - 1] = 0;
    }

    shmid = shm_open(shmName, O_RDWR, 0666);
    CATCH_IF(shmid < 0, 0, "shm_open", 1);

    data = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmid, 0);
    CATCH_IF(data == MAP_FAILED, 1, "mmap", 1);

    CATCH_IF(sem_wait(&data->semExit) < 0, 2, "sem_wait", 1);

    int werr;
    size_t n = 0;
    char buffer[BUFFER_SIZE];
    while (!(werr = sem_wait(&data->semData)))
    {
        if (data->content[n] == 0)
        {
            break;
        }

        size_t len = 0;
        while (data->content[n + len] != '\n')
        {
            len++;
        }

        memcpy(buffer, data->content + n, ++len);
        buffer[len] = 0;
        n += len;

        printf("%s", buffer);
    }

    CATCH_IF(werr < 0, 2, "sem_wait", 1);

    CATCH_IF(sem_post(&data->semExit) < 0, 2, "sem_post", 1);

    munmap(data, SHM_SIZE);
    close(shmid);

    exit(0);
}
