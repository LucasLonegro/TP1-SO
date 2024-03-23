// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_PATH 4096

int readStdio(char *commandBuffer, int maxBuffer)
{
    return read(STDIN_FILENO, commandBuffer, maxBuffer);
}
void terminateOnNewLine(char *s)
{
    while (*s != '\n' && *s)
        s++;
    *s = 0;
}
int hasTerminator(char *s)
{
    while (*s)
    {
        if (*s == 1)
        {
            *s = 0;
            return 1;
        }
        s++;
    }
    return 0;
}

int main(int argc, char *argv[])
{
    char commandBuffer[MAX_PATH + strlen("md5sum ")], stdinBuffer[MAX_PATH] = {0};
    int lastFileFlag = 0;
    while (!lastFileFlag)
    {
        int n;
        *stdinBuffer = 0;
        *commandBuffer = 0;
        if ((n = readStdio(stdinBuffer, sizeof(stdinBuffer))) == sizeof(stdinBuffer) || n == -1)
        {
            write(STDOUT_FILENO, "", 1);

            // parent is responsible for ensuring only one filename is on the commandBuffer at a time
            fflush(STDIN_FILENO);
        }

        // on running out of new files to hand children, parent process sends the null string
        if (hasTerminator(stdinBuffer))
        {
            printf("Terminating\n\n");
            if (!strcmp(stdinBuffer, ""))
                break;
            lastFileFlag = 1;
        }

        snprintf(commandBuffer, sizeof(commandBuffer), "md5sum %s", stdinBuffer);
        FILE *md5sum = popen(commandBuffer, "r");
        fgets(commandBuffer, sizeof(commandBuffer), md5sum);
        terminateOnNewLine(commandBuffer);
        write(STDOUT_FILENO, commandBuffer, strlen(commandBuffer) + 1);

        pclose(md5sum);
    }

    exit(0);
}
