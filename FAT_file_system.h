#ifndef __FAT_H__
#define __FAT_H__

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>

void error_handle(const char* error);
void init_fs(const char *filename);
int input_tokenize(char* input, char* cmd, char** args);

#define MAX_DIRNAME_SIZE 256
#define MAX_DIR_SIZE 64
#define MAX_BLOCKS 1024
#define BLOCK_SIZE 512
#define MAX_INPUT_SIZE 1024
#define MAX_NUM_ARGS 3

#define FAT_FREE 0
#define FAT_EOF -1

typedef struct{
    char name[MAX_DIRNAME_SIZE];
    int start;
    int size;
    int is_dir;
} DirectoryEntry;

typedef struct{
    DirectoryEntry head[MAX_DIR_SIZE];
    int elemCount;
} Directory;

typedef struct{
    int fat[MAX_BLOCKS];
    Directory root;
    Directory *current_dir;
} FileSystem;

static FileSystem fs;
static char* fs_buffer;

typedef struct{
    char* data;
    int pointer;
    int size;
    DirectoryEntry* entry;
} FileHandle;

//Funzioni da implementare
int createFile(char* fileName);
int eraseFile(char* fileName);
FileHandle* openFile(const char *fileName);
void closeFile(FileHandle* fh);
int writeOnFile(FileHandle* fh, const char* data, int length);
int readFromFile(char* fileName, int maxBytes, char* buffer);
int seek(char* fileName);
int createDir(const char* dirName);
int eraseDir(char* dirName);
int changeDir(const char* path);
int listDir(const char* dirPath);
#endif