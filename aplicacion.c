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

char makeChild(int *fdWrite, int *fdRead)
{
    int pipeRead[2], pipeWrite[2], pid;
    if (pipe2(pipeRead, __O_DIRECT) || (pipe2(pipeWrite, __O_DIRECT)) || (pid = fork()))
        return -1;
    if (pid)
    {
        // parent process

        // close read end of write pipe, close write end of read pipe
        close(pipeWrite[0]);
        close(pipeRead[1]);

        // set return values to write and read ends of the two pipes produced
        *fdWrite = pipeWrite[1];
        *fdRead = pipeRead[0];
    }
    else
    {
        // child process

        // close write end of parent's write pipe, close read end of parent's read pipe
        close(pipeWrite[1]);
        close(pipeRead[0]);

        // dup stdin and stdout to parent's write and read pipes, respectively
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        dup(pipeWrite[0]);
        dup(pipeRead[1]);

        char *null = NULL;
        if (execve("esclavo", &null, &null))
            return -1;
    }
    return 0;
}