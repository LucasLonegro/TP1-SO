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

typedef struct shared_data
{
    sem_t semData, semExit;
    char content[SHM_SIZE - sizeof(sem_t)];
} shared_data;

int main(int argc, char *argv[])
{
    char shmName[SHM_NAME_SIZE] = {0};

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

            exit(1);
        }

        // Assert null terminator
        shmName[n - 1] = 0;
    }

    int shmid = shm_open(shmName, O_RDWR, 0666);
    if (shmid < 0)
    {
        perror("shm_open");

        // Nothing to free

        exit(1);
    }

    shared_data *data = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmid, 0);
    if (data == MAP_FAILED)
    {
        perror("mmap");

        close(shmid);

        exit(1);
    }

    if (sem_wait(&data->semExit) < 0)
    {
        perror("sem_wait");

        munmap(data, SHM_SIZE);
        close(shmid);

        exit(1);
    }

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

    if (werr < 0)
    {
        perror("sem_wait");

        munmap(data, SHM_SIZE);
        close(shmid);

        exit(0);
    }

    if (sem_post(&data->semExit) < 0)
    {
        perror("sem_post");

        munmap(data, SHM_SIZE);
        close(shmid);

        exit(1);
    }

    D("Unmapping\n");
    munmap(data, SHM_SIZE);
    close(shmid);

    exit(0);
}
