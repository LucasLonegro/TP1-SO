// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
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
        // Check whether the given file can be read in order to calculate its md5 hash
        struct stat fileData;
        if (!stat(stdinBuffer, &fileData) && !S_ISDIR(fileData.st_mode))
        {
            // Write the verified readable file into an md5sum command
            snprintf(cmd, sizeof(cmd), "md5sum %s", stdinBuffer);

            // Call md5sum
            FILE *md5sum = popen(cmd, "r");

            // Parse the md5sum output and write it into stdout (which has been dupped into a pipe)
            if (fgets(cmd, sizeof(cmd), md5sum))
            {
                removeNewLine(cmd);
                write(STDOUT_FILENO, cmd, strlen(cmd) + 1);
            }
            pclose(md5sum);
        }
        else
        {
            // Inform that the path given did not refer to a file this process could read
            int len = snprintf(cmd, sizeof(cmd), "Could not read: %s", stdinBuffer);
            write(STDOUT_FILENO, cmd, len + 1);
        }
    }

    exit(0);
}
