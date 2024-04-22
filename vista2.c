// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "commons.h"

int main(void)
{
    char *path = "/tmp/myfifo";

    int namedPipeFd = open(path, O_RDONLY);

    size_t n = 0;
    char buffer[BUFFER_SIZE];
    while ((n = read(namedPipeFd, buffer, BUFFER_SIZE)))
    {
        if (n >= BUFFER_SIZE)
        {
            write(STDOUT_FILENO, "Input exceeded buffer size", sizeof("Input exceeded buffer size"));
        }
        else
            write(STDOUT_FILENO, buffer, n);
    }
    close(namedPipeFd);
    exit(0);
}
