// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <signal.h>
#include <sys/types.h>

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

#define MIN_CHILDS 5
#define MAX_CHILDS 20
#define READ_END 0
#define WRITE_END 1

int makeChild(int *write, int *read, int *childPid);
fd_set makeFdSet(int *fdVector, int dim);
void forwardPipes(int nfds, int *readFds, int readCount, int dumpFd, int *readFrom);
void waitForAllChildren();

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        puts("Usage: ./md5 <files_to_process>");
        exit(1);
    }
    printf("Received %d arguments.\n", argc);

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

    int fdWrite[childsCount], fdRead[childsCount], currentFile = 1, nfds = 0;
    pid_t childPids[childsCount];
    for (int i = 0; i < childsCount; i++, currentFile++)
    {
        // fork, create read and write pipes, store associated fds, execute child program
        if (makeChild(((int *)fdWrite) + i, ((int *)fdRead) + i, ((int *)childPids) + i))
            exit(1);
        // update nfds, maximum read fd, for select function
        if (fdWrite[i] > nfds)
            nfds = fdWrite[i];
        // give each child its initial file to handle
        if (write(fdWrite[i], argv[currentFile], strlen(argv[currentFile])) == -1)
            exit(1);
    }

    // create results file
    int resultado = open("./bin/resultado.txt", O_WRONLY | O_CREAT);
    if (resultado == -1)
        exit(1);
    /** Read from file descriptors as they become ready
     *  Write to each ready read file descriptor's associated write file descriptor the next file for the child to manage
     *  Then write the child's previous output to the results file
     */
    while (currentFile < argc)
    {
        // gather outputs from all children into results file
        int readFrom[childsCount + 1];
        forwardPipes(nfds, fdRead, childsCount, resultado, readFrom);

        // give each child that produced an output a new file to process
        for (int i = 0; readFrom[i] != -1; i++)
        {
            if (write(fdWrite[readFrom[i]], argv[currentFile], strlen(argv[currentFile]) + 1) == -1)
                exit(1);
            currentFile++;
        }
    }

    // Once all files have been handed off, signal children to terminate by writing the null string
    for (int i = 0; i < childsCount; i++)
    {
        char terminator = 1;
        if (write(fdWrite[i], &terminator, 1) == -1)
            exit(1);
    }

    // wait for all children to terminate
    waitForAllChildren();

    // process last outputs created by children still with files assigned
    forwardPipes(nfds, fdRead, childsCount, resultado, NULL);

    // close results file
    close(resultado);

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
int makeChild(int *write, int *read, pid_t *childPid)
{
    int pipeRead[2], pipeWrite[2], pid;
    if (pipe(pipeRead) || pipe(pipeWrite) || ((pid = fork()) == -1))
        return 1;

    if (pid)
    {
        // close read end of write pipe, close write end of read pipe
        close(pipeWrite[READ_END]);
        close(pipeRead[WRITE_END]);

        // set return values to write and read ends of the two pipes produced
        *write = pipeWrite[1];
        *read = pipeRead[0];
    }
    else
    {
        // set child's pid
        *childPid = getpid();
        // close write end of parent's write pipe, close read end of parent's read pipe
        close(pipeWrite[WRITE_END]);
        close(pipeRead[READ_END]);

        // dup stdin and stdout to parent's write and read pipes, respectively
        close(STDOUT_FILENO);
        close(STDIN_FILENO);
        dup(pipeWrite[READ_END]);
        dup(pipeRead[WRITE_END]);

        char *const argv[] = {"./esclavo", NULL};
        if (execv("./bin/esclavo", argv))
        {
            return 1;
        }
    }

    return 0;
}
int callCount = 0;
/**
 * @brief Copy buffer from all ready fds in readFds paste on dumpFd return indices of read fds
 *
 * @param nfds is the maximum fd expected in readFds
 * @param readFds is an array of all fds to be read
 * @param readCount is the dim of readFds
 * @param dumpFd is the fd where buffers are to be pasted
 * @param readFrom return parameter. Function produces an array of all fds in readFds that were ready and were read from, terminated with -1. If NULL, will not be produced
 */
void forwardPipes(int nfds, int *readFds, int readCount, int dumpFd, int *readFrom)
{
    char buffer[4096] = {0};
    int index = 0;
    fd_set reading = makeFdSet(readFds, readCount);
    if (select(nfds, &reading, NULL, NULL, NULL) == -1)
        exit(1);
    for (int i = 0; i < readCount; i++)
    {
        // check which read file descriptors are ready, indicating the child has finished processing a file
        if (FD_ISSET(readFds[i], &reading))
        {
            if (readFrom != NULL)
                readFrom[index++] = i;
            // read the child's output
            if (read(readFds[i], buffer, sizeof(buffer)) == -1)
                exit(1);
            // pass the child's output to the results file
            if (write(dumpFd, buffer, strlen(buffer)) == -1)
                exit(1);
            if (write(dumpFd, "\n", 1) == -1)
                exit(1);
        }
    }
    if (readFrom != NULL)
        readFrom[index] = -1;
}
fd_set makeFdSet(int *fdVector, int dim)
{
    fd_set ans;
    FD_ZERO(&ans); // good practice
    for (int i = 0; i < dim; i++)
    {
        FD_SET(fdVector[i], &ans);
    }
    return ans;
}
void waitForAllChildren()
{
    while (wait(NULL) > 0)
        ;
}