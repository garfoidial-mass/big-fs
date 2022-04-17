#include <stdio.h>

#include "../include/bigfs.h"

#define MB_SIZE_BYTES 1000000

int main(int argc, char** argv)
{
    if(argc < 4)
    {
        printf("usage: format <filename> <partition name (char)> <partition size in mb>\n");
        return 1;
    }
    FILE* fs = fopen(argv[1],"w");

    //useful later in the program probably
    char* filename = argv[1];
    char partitionname = argv[2][0];
    uint32_t size = (atoi(argv[3])*MB_SIZE_BYTES);

    FsPartition* partition = createpartition(partitionname,size);
    writepartition(partition,fs,true);
    printf("formatted disk\n");

    char* filedata = "Testing Testing text file woohoo!!!!!!";

    FsFile* testfile = createfile("test.txt",(void*)filedata,partition);
    printf("created file\n");
    writefile(testfile,partition,fs);
    printf("wrote file\n");

    //writepartition(partition,fs,false);
    //printf("rewrote partition (no format)\n");

    destroypartition(partition);

    fclose(fs);
}