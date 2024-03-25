// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

#define MIN_CHILDREN 5
#define MAX_CHILDREN 20
#define READ_END 0
#define WRITE_END 1
#define BUFFER_SIZE 4096

/**
 * @brief Create a child process and return the read and write ends of the pipes
 *
 * @param write Returns the write end of the pipe for the parent
 * @param read Returns the read end of the pipes for the parent
 * @param ppid The parent's process id, used to ensure the child dies upon parent's termination
 * @return int 0 if success, 1 if error
 */
int makeChild(int *write, int *read, pid_t ppid);
/**
 * @brief Set the file descriptors in fdVector in a fd_set
 *
 * @param fdVector The file descriptors to be set
 * @param dim The number of file descriptors in fdVector
 * @return fd_set
 */
fd_set makeFdSet(int *fdVector, int dim);
/**
 * @brief Get the buffer of fds with status ready and write the outputs to dumpFd
 *
 * @param nfds one more than maximum fd expected in readFds
 * @param readFds array of all fds to be read
 * @param readCount dim of readFds
 * @param dumpFd file where buffers will be pasted
 * @param readFrom return parameter. An array of all fds in readFds that were ready and read from. It may be NULL.
 * @return int the number of read fds, -1 if error
 */
ssize_t forwardPipes(int nfds, int *readFds, int readCount, FILE *dumpFd, int *readFrom);

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

        // Nothing to free

        exit(1);
    }

    puts(shmName);

    // Create output file
    FILE *output = fopen("./bin/output.txt", "w");
    if (!output)
    {
        perror("fopen");

        shm_unlink(shmName);

        exit(1);
    }

    const int fileCount = argc - 1;

    int childrenCount = fileCount;
    if (fileCount > MIN_CHILDREN)
    {
        int math = fileCount / 10;
        childrenCount = MIN(MAX_CHILDREN, MAX(MIN_CHILDREN, math));
    }

    int fdWrite[MAX_CHILDREN];
    int fdRead[MAX_CHILDREN];
    /**
     * @brief Highest file descriptor for the select() function
     * @see man 2 select
     */
    int nfds = 0;
    const pid_t ppid = getpid();

    for (int i = 0; i < childrenCount; i++)
    {
        int *wp = fdWrite + i;
        int *rp = fdRead + i;

        if (makeChild(wp, rp, ppid))
        {
            perror("makeChild");

            shm_unlink(shmName);
            fclose(output);

            exit(1);
        }

        if (*wp > nfds)
        {
            nfds = *wp;
        }

        // Give each child its initial file to handle
        ssize_t written = write(*wp, argv[i + 1], strlen(argv[i + 1]) + 1);
        if (written < 0)
        {
            perror("write");

            shm_unlink(shmName);
            fclose(output);

            exit(1);
        }
    }

    /**
     * @brief The index (in argv) of the next file to be processed by a child
     */
    int nextFile = childrenCount + 1;
    /**
     * @brief The number of files processed by children
     */
    int processCount = 0;

    // Read from file descriptors as they become ready
    // Write to each ready read file descriptor's associated write file descriptor the next file for the child to manage
    // Then write the child's previous output to the results file
    while (processCount < fileCount)
    {
        int childrenReady[MAX_CHILDREN];
        int count = forwardPipes(nfds + 1, fdRead, childrenCount, output, childrenReady);
        if (count < 0)
        {
            perror("forwardPipes");

            shm_unlink(shmName);
            fclose(output);

            exit(1);
        }

        processCount += count;

        // Send the next file to each child ready while there are files to process
        for (int i = 0; i < count && nextFile <= fileCount; i++, nextFile++)
        {
            char *filename = argv[nextFile];

            ssize_t written = write(fdWrite[childrenReady[i]], filename, strlen(filename) + 1);
            if (written < 0)
            {
                perror("write");

                shm_unlink(shmName);
                fclose(output);

                exit(1);
            }
        }
    }

    // close results file
    fclose(output);

    // Unlink the shared memory
    if (shm_unlink(shmName) < 0)
    {
        perror("shm_unlink");
        exit(1);
    }

    exit(0);
}

int makeChild(int *write, int *read, pid_t ppid)
{
    int ptoc[2];
    int ctop[2];

    if (pipe(ctop) || pipe(ptoc))
    {
        return 1;
    }

    pid_t pid = fork();
    if (pid < 0)
    {
        return 1;
    }

    if (pid)
    {
        // Close the parent's unused ends of the pipes
        if (close(ptoc[READ_END]) || close(ctop[WRITE_END]))
        {
            return 1;
        }

        // Set return values
        *write = ptoc[WRITE_END];
        *read = ctop[READ_END];
    }
    else
    {
        // Ensure child dies upon father's termination
        if (prctl(PR_SET_PDEATHSIG, SIGKILL) < 0)
        {
            return 1;
        }

        // Check if the parent is still alive
        if (getppid() != ppid)
        {
            return 1;
        }

        // Close the child's unused ends of the pipes
        if (close(ptoc[WRITE_END]) || close(ctop[READ_END]))
        {
            return 1;
        }

        // Dup stdin and stdout to parent's write and read pipes, respectively
        if (dup2(ptoc[READ_END], STDIN_FILENO) == -1 || dup2(ctop[WRITE_END], STDOUT_FILENO) == -1)
        {
            return 1;
        }

        char *const argv[] = {"./esclavo", NULL};
        if (execv("./bin/esclavo", argv))
        {
            return 1;
        }
    }

    return 0;
}

ssize_t forwardPipes(int nfds, int *readFds, int readCount, FILE *dumpFd, int *readFrom)
{
    int index = 0;
    char buffer[BUFFER_SIZE] = {0};

    fd_set reading = makeFdSet(readFds, readCount);

    if (select(nfds, &reading, NULL, NULL, NULL) == -1)
    {
        return -1;
    }

    for (int i = 0; i < readCount; i++)
    {
        // Check which read fd are ready (the child has finished processing a file)
        if (FD_ISSET(readFds[i], &reading))
        {
            if (readFrom != NULL)
            {
                readFrom[index++] = i;
            }

            // read the child's output
            if (read(readFds[i], buffer, sizeof(buffer)) == -1)
            {
                return -1;
            }

            // Write to the output file
            if (fprintf(dumpFd, "%s\n", buffer) < 0)
            {
                return -1;
            }
        }
    }

    return index;
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
