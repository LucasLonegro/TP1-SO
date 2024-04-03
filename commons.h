#ifndef COMMONS_H
#define COMMONS_H

#include <semaphore.h>

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

#define MIN_CHILDREN 5
#define MAX_CHILDREN 20

#define READ_END 0
#define WRITE_END 1

#define BUFFER_SIZE 8192
#define SHM_SIZE 0x4000000
#define SHM_NAME_SIZE 255

#define MAX_PATH 4096
#define HASH_LENGTH 32

typedef struct shared_data
{
    sem_t semData;
    sem_t semExit;
    char content[SHM_SIZE - 2 * sizeof(sem_t)];
} shared_data;

#ifndef DEBUG
#define D(...)
#else
#define D(...) fprintf(stderr, __VA_ARGS__)
#endif

#endif
