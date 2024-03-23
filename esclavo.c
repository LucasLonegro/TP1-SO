// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_PATH 4096

int readStdio(char *buffer, int maxBuffer)
{
    return read(STDIN_FILENO, buffer, maxBuffer);
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
    char buffer[MAX_PATH + strlen("md5sum ")], stdinBuffer[MAX_PATH];
    int lastFileFlag = 0;
    while (!lastFileFlag)
    {
        int n;
        if ((n = readStdio(stdinBuffer, sizeof(stdinBuffer))) == sizeof(buffer) || n == -1)
        {
            write(STDOUT_FILENO, "", 1);

            // parent is responsible for ensuring only one filename is on the buffer at a time
            fflush(STDIN_FILENO);
        }

        // on running out of new files to hand children, parent process sends the null string
        if (hasTerminator(stdinBuffer))
        {
            if (!strcmp(stdinBuffer, ""))
                break;
            lastFileFlag = 1;
        }

        snprintf(buffer, sizeof(buffer), "md5sum %s", stdinBuffer);
        FILE *md5sum = popen(buffer, "r");
        fgets(buffer, 4096, md5sum);
        write(STDOUT_FILENO, buffer, strlen(buffer));

        pclose(md5sum);
    }

    exit(0);
}
