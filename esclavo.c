// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_PATH 4096

#ifndef DEBUG
#define D(...)
#else
#define D(...) fprintf(stderr, __VA_ARGS__)
#endif

/**
 * @brief Search for a newline character in the string and replace it with a null terminator
 *
 * @param s The string to modify
 */
void removeNewLine(char *s);

int main()
{
    /**
     * @brief The input string
     */
    char input[MAX_PATH] = {0};
    /**
     * @brief The command string to execute
     */
    char cmd[sizeof("md5sum ") /* String with \0 */ + MAX_PATH];
    /**
     * @brief The output string
     */
    char output[32 /* Hash */ + 2 /* Spaces */ + MAX_PATH + 1 /* \0 */];

    ssize_t n;
    while ((n = read(STDIN_FILENO, input, sizeof(input))))
    {
        D("Received %ld bytes\n", n);

        if (n == sizeof(input) || n < 0)
        {
            // Unexpected error
            write(STDOUT_FILENO, "", 1);

            // Parent is responsible for ensuring only one filename is on STDIN at a time
            fflush(stdin);

            continue;
        }

        // Check whether the given file can be read in order to calculate its md5 hash
        struct stat fileData;
        if (stat(input, &fileData) || S_ISDIR(fileData.st_mode))
        {
            // Inform that the path given did not refer to a file this process could read
            int len = snprintf(cmd, sizeof(cmd), "Could not read: %s", input);
            write(STDOUT_FILENO, cmd, len + 1);

            continue;
        }

        // Write the verified readable file into an md5sum command
        snprintf(cmd, sizeof(cmd), "md5sum %s", input);

        // Call md5sum
        FILE *md5sum = popen(cmd, "r");
        if (!md5sum)
        {
            // Inform that the md5sum command could not be executed
            int len = snprintf(cmd, sizeof(cmd), "Could not execute: %s", input);
            write(STDOUT_FILENO, cmd, len + 1);

            continue;
        }

        // Parse the md5sum output and write it into stdout
        if (fgets(output, sizeof(output), md5sum))
        {
            removeNewLine(output);
            write(STDOUT_FILENO, output, strlen(output) + 1);
            D("Wrote %s\n", output);
        }
        else
        {
            int len = snprintf(cmd, sizeof(cmd), "Could not handle: %s", input);
            write(STDOUT_FILENO, cmd, len + 1);
        }

        pclose(md5sum);
    }

    D("Exiting\n");

    exit(0);
}

void removeNewLine(char *s)
{
    while (*s != '\n' && *s)
        s++;
    *s = 0;
}
