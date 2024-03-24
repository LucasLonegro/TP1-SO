// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_PATH 4096

int readStdIO(char *commandBuffer, int maxBuffer)
{
    return read(STDIN_FILENO, commandBuffer, maxBuffer);
}

void removeNewLine(char *s)
{
    while (*s != '\n' && *s)
        s++;
    *s = 0;
}

int main(int argc, char *argv[])
{
    char stdinBuffer[MAX_PATH] = {0};
    char cmd[MAX_PATH + 7 /* strlen("md5sum ") */];

    while (1)
    {
        int n = readStdIO(stdinBuffer, sizeof(stdinBuffer));
        if (n == sizeof(stdinBuffer) || n < 0)
        {
            // Unexpected error
            write(STDOUT_FILENO, "", 1);

            // Parent is responsible for ensuring only one filename is on STDIN at a time
            fflush(STDIN_FILENO);
        }

        snprintf(cmd, sizeof(cmd), "md5sum %s", stdinBuffer);
        FILE *md5sum = popen(cmd, "r");

        if (fgets(cmd, sizeof(cmd), md5sum))
        {
            removeNewLine(cmd);
            write(STDOUT_FILENO, cmd, strlen(cmd) + 1);
        }

        pclose(md5sum);
    }

    exit(0);
}
