//super simple generic data tree implementation
//modify to do whatever you want

#ifndef BIG_FS_H
#define BIG_FS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "ut/uthash.h"

typedef enum Bool{true = (uint8_t)1,false  = (uint8_t)0} bool;

typedef struct file_t{
    //in header on disk
    uint32_t headersize;
    char* name;
    uint32_t datasize; //bytes
    uint32_t dataloc; //location on disk
    uint32_t next; //location of next file header on disk

    //not on disk
    uint32_t headerloc;
    
    //for files hash map in partition, not on disk
    UT_hash_handle hh;

    //not in header, on disk
    void* data;
} FsFile;

#define FILE_HEADER_SIZE(file) (strlen(file.name)+1+sizeof(file.headersize)+sizeof(file.datasize)+sizeof(file.dataloc)+sizeof(file.next))

typedef struct partition_t{
    char name;
    uint32_t size; //megabytes
    uint32_t numfiles;
    FsFile* files;
}FsPartition;

typedef struct block_t{
    bool full; // 1 if yes, 0 if no
    uint32_t start;
    uint32_t end;
    uint32_t count;
}FsBlock;

int compareblockstarts(FsBlock* block1, FsBlock* block2)
{
    return block1->start-block2->start;
}

FsBlock* findfullblocks(FsPartition* partition)
{
    FsBlock* allblocks = NULL;
    int numblocks = 0;
    FsFile *file, *tmp;
    HASH_ITER(hh,partition->files,file,tmp)
    {
        FsBlock headerblock = (FsBlock){true,file->headerloc,file->headerloc+file->headersize};
        FsBlock datablock = (FsBlock){true,file->dataloc,file->dataloc+file->datasize};
        numblocks+=2;
        allblocks = realloc(allblocks,numblocks);
    }

    qsort(allblocks,numblocks,sizeof(FsBlock),compareblockstarts);
    
    FsBlock* combinedblocks = malloc(sizeof(FsBlock)*numblocks);
    int numcombined = 0;
    for(int x = 1; x < numblocks; x++)
    {
        FsBlock newblock;
        newblock.full = true;
        if( (allblocks[x].start-allblocks[x-1].end) == 1)
        {
            newblock.start = allblocks[x-1].start;
            newblock.end = allblocks[x].end;
        }
        else
        {
            newblock = allblocks[x-1];
        }
        numcombined += 1;
    }
    combinedblocks = realloc(combinedblocks,numcombined);

    free(allblocks);

    if(numcombined != 0)
    {
        combinedblocks[0].count = numcombined;
    }

    return combinedblocks;
}

FsBlock* findemptyblocks(FsPartition* partition)
{
    FsBlock* fullblocks = findfullblocks(partition);

    FsBlock* emptyblocks = malloc(sizeof(FsBlock)*fullblocks[0].count);

    int numempty = 0;

    for(int x = 1; x < fullblocks[0].count; x++)
    {
        FsBlock block = (FsBlock){false,fullblocks[x-1].end,fullblocks[x].start,0};
        emptyblocks[x-1] = block;
        numempty += 1;
    }

    if(fullblocks[fullblocks[0].count-1].end < partition->size)
    {
        emptyblocks[numempty-1].start = fullblocks[fullblocks[0].count-1].end;
        emptyblocks[numempty-1].end = partition->size;
        numempty += 1;
    }

    emptyblocks = realloc(emptyblocks,sizeof(FsBlock)*numempty);

    emptyblocks[0].count = numempty;

    free(fullblocks);

    return emptyblocks;
}

FsFile* createfile(char* name,void* data,FsPartition* partition) {
    partition->numfiles += 1;
    FsFile* file = malloc(sizeof(FsFile));

    *file = (FsFile){0,name,strlen((char*)data)+1,0,0};
    file->data = data;
    file->headersize = FILE_HEADER_SIZE((*file));

    if(partition->numfiles > 1)
    {
        FsBlock* emptyblocks = findemptyblocks(partition);
        int numempty = emptyblocks[0].count;

        for(int x = 0; x < numempty; x++)
        {
            int blocksize = emptyblocks[x].end - emptyblocks[x].start;
            if(blocksize >= file->headersize)
            {
                file->headerloc = emptyblocks[x].start;
                break;
            }
        }
        for(int x = 0; x < numempty; x++)
        {
            int blocksize = emptyblocks[x].end - emptyblocks[x].start;
            if(blocksize >= file->datasize)
            {
                file->dataloc = emptyblocks[x].start;
                break;
            }
        }

        free(emptyblocks);

        ((FsFile*)(file->hh.prev))->next = file->headerloc;
    }
    else
    {
        file->headerloc = 15;
        file->dataloc = file->headerloc + file->headersize;
    }

    HASH_ADD_STR(partition->files,name,file);

    return file;
}

void destroyfilebyptr(FsFile* file, FsPartition* partition)
{
    HASH_DEL(partition->files,file);
    free(file);
}

void destroyfilebyname(char* name,FsPartition* partition)
{
    FsFile* file;
    HASH_FIND_STR(partition->files,name,file);
    HASH_DEL(partition->files, file);
    free(file);
}

FsPartition* createpartition(char name, uint32_t size) {
    FsPartition* partition = malloc(sizeof(FsPartition));
    (*partition) = (FsPartition){name, size, 0, (FsFile*)NULL};
    return partition;
}
void destroypartition(FsPartition* partition) //in ram, not disk
{
    FsFile *file, *tmp;
    HASH_ITER(hh,partition->files,file,tmp)
    {
        destroyfilebyptr(file,partition);
    }
    free(partition);
}

FsPartition* readpartition(FILE* disk)
{
    fseek(disk,0,SEEK_SET);
    FsPartition* partition = malloc(sizeof(FsPartition));
    partition->name = fgetc(disk);
    fread(&partition->size,sizeof(uint32_t),1,disk);
    fread(&partition->numfiles,sizeof(uint32_t),1,disk);
}

int writepartition(FsPartition* partition, FILE* disk, bool format )
{
    void* buffer = malloc(sizeof(char)*partition->size);
    if(format)
    {
        memset(buffer,'\0',sizeof(char)*partition->size);
    }
    else
    {
        fread(buffer,sizeof(char)*partition->size,1,disk);
    }
    *((char*)(buffer)) = partition->name;
    *((uint32_t*)(buffer+sizeof(char))) = partition->size;
    *((uint32_t*)(buffer+sizeof(char)+sizeof(uint32_t))) = partition->numfiles;
    fseek(disk,0,SEEK_SET);
    fwrite(buffer,sizeof(char)*partition->size,1,disk);
    free(buffer);
    return 0;
}

int writefile(FsFile* file, FsPartition* fs, FILE* disk)
{
    //PROC: run through all files, find filled and empty spots on disk
    // run through empty spots, check each ones size and compare it to the file's size
    //if smaller skip it, if larger write the file there

    fseek(disk,file->headerloc,SEEK_SET);

    char* buffer = malloc(file->headersize);
    void* bufptr = buffer;

    *((uint32_t*)(bufptr)) = file->headersize;
    bufptr+=sizeof(uint32_t);

    memcpy(bufptr,file->name,strlen(file->name)+1);
    bufptr+=strlen(file->name)+1;

    *(uint32_t*)(bufptr) = file->datasize;
    bufptr += sizeof(uint32_t);

    *(uint32_t*)(bufptr) = file->dataloc;
    bufptr += sizeof(uint32_t);
    
    *(uint32_t*)(bufptr) = file->next;
    bufptr += sizeof(uint32_t);

    fwrite(buffer,file->headersize,1,disk);
    free(buffer);

    fseek(disk,file->dataloc,SEEK_SET);
    fwrite(file->data,file->datasize,1,disk);
}

#endif