#pragma once

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
void launch_fs(const char *filename);
int input_tokenize(char* input, char* cmd, char** args);
int aux(char* data);
void syncro(void* pointer);

#define MAX_DIRNAME_SIZE 32
#define MAX_DIR_SIZE 64
#define MAX_BLOCKS 1024
#define BLOCK_SIZE 512
#define MAX_INPUT_SIZE 1024
#define MAX_NUM_ARGS 3
#define STARTING_DIR_SIZE 2

#define FAT_FREE 0
#define FAT_EOF -1

typedef struct{
    char name[MAX_DIRNAME_SIZE];
    int start;
    int elemCount;
    int size;
    int is_dir;
    struct DirectoryEntry* parent;
    int is_open;
    int *head;
} DirectoryEntry;

/*typedef struct{
    int elemCount;
    int size;
    struct Directory* parent;
    int* head;
} Directory;*/

typedef struct{
    int fat[MAX_BLOCKS];
    DirectoryEntry root;
    DirectoryEntry *current_dir;
} FileSystem;

extern FileSystem fs;
extern char* fs_buffer;
extern char* fs_map;

typedef struct{
    char* data;
    int pointer;
    int size;
    DirectoryEntry* entry;
} FileHandle;

//int resizeDir(DirectoryEntry* dir, int newSize);
void close_fs(const char* filename);

//Funzioni da implementare
int createFile(char* fileName);
int eraseFile(char* fileName);
FileHandle* openFile(const char *fileName);
void closeFile(FileHandle* fh);
int writeOnFile(FileHandle* fh, const char* data, int length);
int readFromFile(FileHandle* fh, int maxBytes, char* buffer);
int seek(FileHandle* fh, int pos);
int createDir(const char* dirName);
int eraseDir(char* dirName);
int changeDir(char* path);
int listDir();
