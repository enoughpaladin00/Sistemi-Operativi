#include "FAT_file_system.h"

#define MAX_CMD_SIZE 32

int main(int argc, char** argv){
    if(argc < 2){
        fprintf(stderr, "USAGE: ./FAT_main [fileSystemName]");
        exit(1);
    }

    init_fs(argv[1]);

    char* input = (char*)malloc(sizeof(char)*MAX_BUFFER_SIZE);
    while(1){
        if(!fgets(input, MAX_BUFFER_SIZE, stdin)){
            free(input);
            error_handle("fgets");
        }

        char* cmd = (char*)malloc(sizeof(char)*MAX_CMD_SIZE);
        char** args;
        int argsNum = input_tokenize(input, cmd, args);

        if(!strcmp(cmd, "\n")){
            free(cmd);
            return 0;
        }
        else if(!strcmp(cmd, "createFile") && argsNum != 1){
            
        }
        else if(!strcmp(cmd, "eraseFile") && argsNum != 1){
            
        }
        else if(!strcmp(cmd, "write") && argsNum != 1){
            
        }
        else if(!strcmp(cmd, "read") && argsNum != 1){
            
        }
        else if(!strcmp(cmd, "seek") && argsNum != 1){
            
        }
        else if(!strcmp(cmd, "createDir") && argsNum != 1){
            
        }
        else if(!strcmp(cmd, "eraseDir") && argsNum != 1){
            
        }
        else if(!strcmp(cmd, "changeDir") && argsNum != 1){
            
        }
        else if(!strcmp(cmd, "listDir\n")){
            if(argsNum != 0){
                printf("USAGE: listDir\n");
                continue;
            }
            
        }
        else{
            printf("USAGE: [command] [filename]\n");
        }
    }
}