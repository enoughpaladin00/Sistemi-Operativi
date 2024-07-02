#include "FAT_file_system.h"

/*
Funzione che inizializza la mappatura del buffer e le variabili del file system
*/
void init_fs(const char *filename){
    
    int fd = open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if(fd == -1) error_handle("open");

    if(ftruncate(fd, BLOCK_SIZE * MAX_BLOCKS)) error_handle("ftruncate");

    fs_buffer = mmap(NULL, BLOCK_SIZE * MAX_BLOCKS, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(fs_buffer == MAP_FAILED) error_handle("mmap");

    if(close(fd)) error_handle("close");

    fs.current_dir = &fs.root;
    fs.root.elemCount = 0;

    for(int i = 0; i < MAX_BLOCKS; i++){
        fs.fat[i] = 0;
    }
}

/*

*/
void error_handle(const char* error){
    dprintf(2, "ERRORE %s: %d\n",error, errno);
    exit(-1);
};

int input_tokenize(char* input, char* cmd, char** args){
    int count = 0;

    char* temp = strcpy(temp, input);
    strcpy(cmd, strtok(input, " "));
    while(strtok(NULL, " ") || strtok(NULL, "\n")){
        count++;
    }

    args = (char**) malloc(sizeof(char*)*count);
    strtok(temp, " ");
    for(int i = 0; i < count; i++){
        args[i] = (char*)calloc(MAX_BUFFER_SIZE, sizeof(char));
        strcpy(args[i], strtok(NULL, " "));
    }
    return count;
}
