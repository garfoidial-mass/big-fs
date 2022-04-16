#include <stdio.h>

#include "../include/bigfs.h"

#define MB_SIZE_BYTES 1000000

int main(int argc, char** argv)
{
    if(argc < 4)
    {
        printf("usage: format <filename> <partition name (char)> <partition size in mb>");
    }

    //useful later in the program probably
    char* filename = argv[1];
    char partitionname = argv[2][0];
    uint32_t size = (atoi(argv[3])*MB_SIZE_BYTES);

    FsPartition partition = createpartition(partitionname,size);

    char* filedata = "Testing Testing text file woohoo!!!!!!";

    FsFile testfile = createfile("test.txt",strlen(filedata),(void*)filedata,&partition);

    //open file and resize it to proper size
    FILE* fs = fopen(argv[1],"w");
    writepartition(partition,fs);

    fclose(fs);
}