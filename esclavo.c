#include <unistd.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char ** argv){
    if(argc < 2)
        return 1;
    char buffer[4096] = "md5sum ";
    FILE * md5sum = popen(strcat(buffer,argv[1]),"r");
    fgets(buffer,4096,md5sum);
    write(1,buffer,strlen(buffer));
    pclose(md5sum);
    return 0;
}