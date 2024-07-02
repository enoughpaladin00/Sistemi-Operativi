#include <stdio.h>
#include <stdlib.h>

#define MAX_DIRNAME_SIZE 256
#define MAX_DIR_SIZE 64


typedef struct{
    char name[MAX_DIRNAME_SIZE];
    int is_dir;
} DirectoryEntry;

typedef struct{
    DirectoryEntry head[MAX_DIR_SIZE];
    int elemCount;
} Directory;