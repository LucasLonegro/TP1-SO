// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include <fcntl.h>
#include <semaphore.h>
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
#define BUFFER_SIZE 8192
#define SHM_SIZE 0x4000000

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
        case 5:                               \
            fclose(outputFile);               \
            /* fallthrough */                 \
        case 4:                               \
            sem_destroy(&data->semExit);      \
            /* fallthrough */                 \
        case 3:                               \
            sem_destroy(&data->semData);      \
            /* fallthrough */                 \
        case 2:                               \
            munmap(data, SHM_SIZE);           \
            /* fallthrough */                 \
        case 1:                               \
            shm_unlink(shmName);              \
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

/**
 * @brief Create a child process and return the read and write ends of the pipes
 *
 * @param write Returns the write end of the pipe for the parent
 * @param read Returns the read end of the pipes for the parent
 * @return int The child pid, no return if child, 0 if error
 */
pid_t makeChild(int *write, int *read);
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
 * @param nfds one more than maximum fd expected in childrenReadFD
 * @param childrenReadFD array of all fds to be read
 * @param readCount dim of childrenReadFD
 * @param ready return parameter. An array of all fds in childrenReadFD that are ready
 * @return int the number of read fds, -1 if error
 */
ssize_t awaitPipes(int nfds, int *childrenReadFD, int readCount, int *ready);
/**
 * @brief Remove the child at index i from the arrays write and read
 *
 * @param i Child index
 * @param children The children pids array
 * @param write The children write fds array
 * @param read The children read fds array
 * @param childs The number of children (dim of children, write and read)
 * @return int The new number of children
 */
int removeChildFromArrays(int i, pid_t *children, int *write, int *read, int childs);

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        puts("Usage: ./md5 <files_to_process>");
        exit(1);
    }

    char shmName[255] = "/md5_shm_0";
    shared_data *data;
    FILE *outputFile;

    // Open a shared memory with an unique name
    snprintf((char *)shmName, sizeof(shmName) - 1, "/md5_shm_%d", getpid());

    int shmid = shm_open(shmName, O_RDWR | O_CREAT | O_EXCL, 0666);
    if (shmid < 0)
    {
        perror("shm_open");

        // Nothing to free

        exit(1);
    }

    if (ftruncate(shmid, SHM_SIZE))
    {
        perror("ftruncate");

        shm_unlink(shmName);

        exit(1);
    }

    data = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmid, 0);
    if (data == MAP_FAILED)
    {
        perror("mmap");

        shm_unlink(shmName);

        exit(1);
    }

    // Create the two anonymous semaphores
    if (sem_init(&data->semData, 1, 0) < 0)
    {
        perror("sem_init");

        munmap(data, SHM_SIZE);
        shm_unlink(shmName);

        exit(1);
    }

    if (sem_init(&data->semExit, 1, 1) < 0)
    {
        perror("sem_init");

        sem_destroy(&data->semData);
        munmap(data, SHM_SIZE);
        shm_unlink(shmName);

        exit(1);
    }

    puts(shmName);
    fflush(stdout);

    // Create output file
    outputFile = fopen("./bin/output.txt", "w");
    if (!outputFile)
    {
        perror("fopen");

        sem_destroy(&data->semExit);
        sem_destroy(&data->semData);
        munmap(data, SHM_SIZE);
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

    pid_t children[MAX_CHILDREN];
    int childrenWriteFD[MAX_CHILDREN];
    int childrenReadFD[MAX_CHILDREN];
    /**
     * @brief Highest file descriptor for the select() function
     * @see man 2 select
     */
    int nfds = 0;

    for (int i = 0; i < childrenCount; i++)
    {
        int *wp = childrenWriteFD + i;
        int *rp = childrenReadFD + i;

        if (!(children[i] = makeChild(wp, rp)))
        {
            perror("makeChild");

            fclose(outputFile);
            sem_destroy(&data->semExit);
            sem_destroy(&data->semData);
            munmap(data, SHM_SIZE);
            shm_unlink(shmName);

            exit(1);
        }

        if (*rp > nfds)
        {
            nfds = *rp;
        }

        // Give each child its initial file to handle
        ssize_t written = write(*wp, argv[i + 1], strlen(argv[i + 1]));
        if (written < 0)
        {
            perror("write");

            fclose(outputFile);
            sem_destroy(&data->semExit);
            sem_destroy(&data->semData);
            munmap(data, SHM_SIZE);
            shm_unlink(shmName);

            exit(1);
        }
    }

    if (isatty(STDOUT_FILENO))
    {
        sleep(10);
    }

    /**
     * @brief The index (in argv) of the next file to be processed by a child
     */
    int nextFile = childrenCount + 1;
    /**
     * @brief The number of files processed by children
     */
    int processCount = 0;
    /**
     * @brief The shared memory content index
     */
    int writtenCount = 0;

    // Read from file descriptors as they become ready
    // Write to each ready read file descriptor's associated write file descriptor the next file for the child to manage
    // Then write the child's previous output to the results file
    while (processCount < fileCount)
    {
        int childrenReady[MAX_CHILDREN];
        int count = awaitPipes(nfds + 1, childrenReadFD, childrenCount, childrenReady);
        if (count < 0)
        {
            perror("awaitPipes");

            fclose(outputFile);
            sem_destroy(&data->semExit);
            sem_destroy(&data->semData);
            munmap(data, SHM_SIZE);
            shm_unlink(shmName);

            exit(1);
        }

        processCount += count;

        char buffer[BUFFER_SIZE];

        for (int i = 0; i < count; i++)
        {
            int child = childrenReady[i];
            pid_t cpid = children[child];

            // Read each child's message and write it to the results files
            int ctop = childrenReadFD[child];

            ssize_t n = read(ctop, buffer, sizeof(buffer) - 1);
            if (n < 0)
            {
                perror("read");

                fclose(outputFile);
                sem_destroy(&data->semExit);
                sem_destroy(&data->semData);
                munmap(data, SHM_SIZE);
                shm_unlink(shmName);

                exit(1);
            }

            // Assert the null terminator
            // (the buffer _should_ include an \n at the end)
            buffer[n] = 0;

            // Write to the output file
            if (fprintf(outputFile, "%s", buffer) < 0)
            {
                perror("fprintf");

                fclose(outputFile);
                sem_destroy(&data->semExit);
                sem_destroy(&data->semData);
                munmap(data, SHM_SIZE);
                shm_unlink(shmName);

                exit(1);
            }

            // Write to the shared memory
            writtenCount += sprintf(data->content + writtenCount, "%d: %s", cpid, buffer);

            // Raise the semaphore for the view to read the shared memory
            if (sem_post(&data->semData))
            {
                perror("sem_post");

                fclose(outputFile);
                sem_destroy(&data->semExit);
                sem_destroy(&data->semData);
                munmap(data, SHM_SIZE);
                shm_unlink(shmName);

                exit(1);
            }

            // Send the next file to each child ready while there are files to process
            int ptoc = childrenWriteFD[child];

            if (nextFile > fileCount)
            {
                close(ptoc);
                childrenCount = removeChildFromArrays(child, children, childrenWriteFD, childrenReadFD, childrenCount);
                continue;
            }

            char *filename = argv[nextFile++];

            ssize_t written = write(ptoc, filename, strlen(filename));
            if (written < 0)
            {
                perror("write");

                fclose(outputFile);
                sem_destroy(&data->semExit);
                sem_destroy(&data->semData);
                munmap(data, SHM_SIZE);
                shm_unlink(shmName);

                exit(1);
            }
        }
    }

    if (sem_post(&data->semData))
    {
        perror("sem_post");

        fclose(outputFile);
        sem_destroy(&data->semExit);
        sem_destroy(&data->semData);
        munmap(data, SHM_SIZE);
        shm_unlink(shmName);

        exit(1);
    }

    // Wait for vista to finish and free opened resources
    if (sem_wait(&data->semExit) < 0)
    {
        perror("sem_wait");

        fclose(outputFile);
        sem_destroy(&data->semExit);
        sem_destroy(&data->semData);
        munmap(data, SHM_SIZE);
        shm_unlink(shmName);

        exit(1);
    }

    fclose(outputFile);
    sem_destroy(&data->semExit);
    sem_destroy(&data->semData);
    munmap(data, SHM_SIZE);
    close(shmid);

    exit(0);
}

pid_t makeChild(int *write, int *read)
{
    int ptoc[2];
    int ctop[2];

    if (pipe(ctop) || pipe(ptoc))
    {
        return 0;
    }

    pid_t pid = fork();
    if (pid < 0)
    {
        return 0;
    }

    if (pid)
    {
        // Close the parent's unused ends of the pipes
        if (close(ptoc[READ_END]) || close(ctop[WRITE_END]))
        {
            return 0;
        }

        // Set return values
        *write = ptoc[WRITE_END];
        *read = ctop[READ_END];

        return pid;
    }
    else
    {
        // Close the child's unused ends of the pipes
        if (close(ptoc[WRITE_END]) || close(ctop[READ_END]))
        {
            return 0;
        }

        // Dup stdin and stdout to parent's write and read pipes, respectively
        if (dup2(ptoc[READ_END], STDIN_FILENO) < 0 || dup2(ctop[WRITE_END], STDOUT_FILENO) < 0)
        {
            return 0;
        }

        char *const argv[] = {"./esclavo", NULL};
        if (execv("./bin/esclavo", argv))
        {
            return 0;
        }
    }

    return 0;
}

ssize_t awaitPipes(int nfds, int *childrenReadFD, int readCount, int *ready)
{
    int index = 0;

    fd_set reading = makeFdSet(childrenReadFD, readCount);

    if (select(nfds, &reading, NULL, NULL, NULL) < 0)
    {
        return -1;
    }

    for (int i = 0; i < readCount; i++)
    {
        // Check which read fd are ready (the child has finished processing a file)
        if (FD_ISSET(childrenReadFD[i], &reading))
        {
            ready[index++] = i;
        }
    }

    return index;
}

fd_set makeFdSet(int *fdVector, int dim)
{
    fd_set ans;
    FD_ZERO(&ans); // Good practice
    for (int i = 0; i < dim; i++)
    {
        FD_SET(fdVector[i], &ans);
    }
    return ans;
}

int removeChildFromArrays(int i, pid_t *children, int *write, int *read, int childs)
{
    children[i] = children[childs - 1];
    write[i] = write[childs - 1];
    read[i] = read[childs - 1];
    return childs - 1;
}
