// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

#define MIN_CHILDREN 5
#define MAX_CHILDREN 20

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        puts("Usage: ./md5 <files_to_process>");
        exit(1);
    }

    // Open a shared memory with an unique name
    const char shmName[255] = "/md5_shm_0";
    snprintf((char *)shmName, sizeof(shmName) - 1, "/md5_shm_%d", getpid());

    int shmid = shm_open(shmName, O_RDWR | O_CREAT | O_EXCL, 0666);
    if (shmid < 0)
    {
        perror("shm_open");
        exit(1);
    }

    puts(shmName);

    const int fileCount = argc - 1;

    int childrenCount = fileCount;
    if (fileCount > MIN_CHILDREN)
    {
        int math = fileCount / 10;
        childrenCount = MIN(MAX_CHILDREN, MAX(MIN_CHILDREN, math));
    }

    // Unlink the shared memory
    if (shm_unlink(shmName) < 0)
    {
        perror("shm_unlink");
        exit(1);
    }

    exit(0);
}
