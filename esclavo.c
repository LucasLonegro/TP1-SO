// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include <unistd.h>
#include <stdio.h>
#include <string.h>

char *truncateString(char *s)
{
    int i = 0;
    while (s[i] && s[i] != ' ' && s[i] != '\n')
        i++;
    s[i] = 0;
    return s;
}

int main(int argc, char **argv)
{
    if (argc < 2)
        return 1;
    char buffer[4096] = "md5sum ";
    snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), "%s", argv[1]);
    FILE *md5sum = popen(buffer, "r");
    fgets(buffer, 4096, md5sum);
    write(1, buffer, strlen(buffer));
    pclose(md5sum);
    return 0;
}
