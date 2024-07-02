#ifndef __FAT_H__
#define __FAT_H__

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
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
#define MAX_BUFFER_SIZE 1024

typedef struct{
    char name[MAX_DIRNAME_SIZE];
    int start;
    int size;
    int is_dir;
    //mode_t permissions;
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
    int start;
    int size;
} FileHandle;

//Funzioni da implementare
int createFile(char* fileName);
int eraseFile(char* fileName);
int writeOnFile(char* fileName, int byteNum, char* buffer);
int readFromFile(char* fileName);
int seek(char* fileName);
int createDir(const char* dirName);
int eraseDir(char* dirName);
int changeDir(const char* path);
int listDir(const char* dirPath);

#endif