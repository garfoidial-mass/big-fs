//super simple generic data tree implementation
//modify to do whatever you want

#ifndef BIG_FS_H
#define BIG_FS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "ut/utlist.h"

typedef enum bool{true = (uint8_t)1,false  = (uint8_t)0} bool;

//                                                                          headersize      headerloc        datasize          dataloc       nextdisc
#define SET_FILE_HEADER_SIZE(file) (file.headersize = strlen(file.name)+sizeof(uint32_t)+sizeof(uint32_t)+sizeof(uint32_t)+sizeof(uint32_t)+sizeof(uint32_t))

typedef struct file_t{
    //in header
    uint32_t headersize;
    char* name;
    uint32_t headerloc;
    uint32_t datasize; //bytes
    uint32_t dataloc; //location on disk
    uint32_t next; //location of next file header on disk
    
    //not in header, on disk
    void* data; //null terminated
} FsFile;

typedef struct partition_t{
    char name;
    uint32_t size; //megabytes
    uint32_t numfiles;
    FsFile* files;
}FsPartition;

FsPartition* createpartition(name, size) {
    FsPartition* partition = malloc(sizeof(FsPartition));
    (*partition) = (FsPartition){name, size, 0, (FsFile*)NULL,(FsBlock*)NULL};
}
void destroypartition(FsPartition* partition) //in ram, not disk
{

}

FsFile* createfile(char* name,uint32_t size,void* data,FsPartition* fs) {
    fs->numfiles += 1;
    fs->files = realloc(fs->files,sizeof(FsFile)*fs->numfiles);
    FsFile* file = &(fs->files[fs->numfiles-1]);
    *file = (FsFile){0,name,0,strlen((char*)data),0,0,NULL,NULL,data};
    SET_FILE_HEADER_SIZE((*file));
    return file;
}

typedef struct block_t{
    bool full; // 1 if yes, 0 if no
    uint32_t start;
    uint32_t end;
}FsBlock;

int fileloccompare(FsFile* file1, FsFile* file2)
{
    return file1->dataloc - file2->dataloc;
}

FsBlock* findemptyblocks(FsPartition* partition)
{
    FsFile* filescopy = malloc(sizeof(FsFile)*partition->numfiles);
    memcpy(filescopy,partition->files,sizeof(FsFile)*partition->numfiles);

    qsort(filescopy,partition->numfiles,sizeof(FsFile),fileloccompare);

    FsBlock* emptyblocks = NULL;

    for(int x = 0; x < partition->numfiles; x++)
    {
        FsBlock nextblock = {false,0,0};
        if(x < partition->numfiles-2)
        {
            nextblock.start = partition->files[x].dataloc+partition->files[x].datasize;
            nextblock.start = partition->files[x+1].dataloc;
        }
        else
        {
            nextblock.start = partition->files[x].dataloc+partition->files[x].datasize;
            nextblock.end = partition->size;
        }
        emptyblocks = realloc(emptyblocks,x+1);
        emptyblocks[x] = nextblock;
    }

    return emptyblocks;
}

FsPartition* readpartition(FILE* disk)
{
    fseek(disk,0,SEEK_SET);
    FsPartition* partition = malloc(sizeof(FsPartition));
    partition->name = fgetc(disk);
    fread(partition->size,sizeof(uint32_t),1,disk);
    fread(partition->numfiles,sizeof(uint32_t),1,disk);
}

int writepartition(FsPartition partition, FILE* disk)
{
    char nullchar = '\0';
    fwrite((void*)&nullchar,sizeof(char),partition.size,disk);
    fwrite(partition.name,sizeof(char),1,disk);
    fwrite(partition.size,sizeof(uint32_t),1,disk);
    fwrite(partition.numfiles,sizeof(uint32_t),1,disk);
    return 0;
}


int writefile(FILE* disk, FsFile* file, FsPartition* fs)
{
    //PROC: run through all files, find filled and empty spots on disk
    // run through empty spots, check each ones size and compare it to the file's size
    //if smaller skip it, if larger write the file there

}

#endif