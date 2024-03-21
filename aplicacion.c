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
    const char shmName[255] = "/md5_shm_0";
    snprintf((char *)shmName, sizeof(shmName) - 1, "/md5_shm_%d", getpid());
    int shmid = shm_open(shmName, O_RDWR | O_CREAT | O_EXCL, 0666);

    if (shmid < 0)
    {
        perror("shm_open");
        exit(1);
    }

    puts(shmName);

    int fileCount = argc - 1;

    int childsCount = fileCount;
    if (fileCount > MIN_CHILDS)
    {
        int math = fileCount / 10;
        childsCount = MIN(MAX_CHILDS, MAX(MIN_CHILDS, math));
    }

    printf("Max childs: %d\n", childsCount);
    for (int i = 1; i < argc; i++)
    {
        printf("%s\n", argv[i]);
    }

    // Unlink the shared memory
    if (shm_unlink(shmName) < 0)
    {
        perror("shm_unlink");
        exit(1);
    }

    exit(0);
}

/**
 * @brief Create a child process and return the read and write ends of the pipes
 *
 * @param write Returns the write end of the pipe for the parent
 * @param read Returns the read end of the pipes for the parent
 * @return int 0 if success, 1 if error
 */
int makeChild(int *write, int *read)
{
    int pipeRead[2], pipeWrite[2], pid;
    if (pipe(pipeRead) || pipe(pipeWrite) || ((pid = fork()) == -1))
        return 1;

    if (pid)
    {
        // close read end of write pipe, close write end of read pipe
        close(pipeWrite[0]);
        close(pipeRead[1]);

        // set return values to write and read ends of the two pipes produced
        *write = pipeWrite[1];
        *read = pipeRead[0];
    }
    else
    {
        // close write end of parent's write pipe, close read end of parent's read pipe
        close(pipeWrite[1]);
        close(pipeRead[0]);

        // dup stdin and stdout to parent's write and read pipes, respectively
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        dup(pipeWrite[0]);
        dup(pipeRead[1]);

        char null[] = {NULL};
        if (execve("esclavo", null, null))
            return 1;
    }

    return 0;
}