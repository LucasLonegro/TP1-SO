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

#define MIN_CHILDS 5
#define MAX_CHILDS 20

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        puts("Usage: ./md5 <files_to_process>");
        exit(1);
    }

    // Open a shared memory with an unique name
    const char shm_name[255] = "/md5_shm_0";
    snprintf((char *)shm_name, sizeof(shm_name) - 1, "/md5_shm_%d", getpid());
    int shmid = shm_open(shm_name, O_RDWR | O_CREAT | O_EXCL, 0666);

    if (shmid < 0)
    {
        perror("shm_open");
        exit(1);
    }

    puts(shm_name);

    int file_count = argc - 1;

    int childs_count = file_count;
    if (file_count > MIN_CHILDS)
    {
        int math = file_count / 10;
        childs_count = MIN(MAX_CHILDS, MAX(MIN_CHILDS, math));
    }

    printf("Max childs: %d\n", childs_count);
    for (int i = 1; i < argc; i++)
    {
        printf("%s\n", argv[i]);
    }

    // Unlink the shared memory
    if (shm_unlink(shm_name) < 0)
    {
        perror("shm_unlink");
        exit(1);
    }

    exit(0);
}
