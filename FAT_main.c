#include "FAT_file_system.h"

#define MAX_CMD_SIZE 32

int main(int argc, char** argv){
    if(argc < 2){
        fprintf(stderr, "USAGE: ./FAT_main [fileSystemName]");
        exit(1);
    }

    printf("Inizializzo il file system... ");
    init_fs(argv[1]);
    puts("FATTO");
    char* input = (char*)malloc(sizeof(char)*MAX_INPUT_SIZE);
    char* cmd = (char*)malloc(sizeof(char)*MAX_CMD_SIZE);
    char** args = (char**)malloc(sizeof(char*) * MAX_NUM_ARGS);
    int argsNum;

    do{
        printf("$: ");
        if(!fgets(input, MAX_INPUT_SIZE, stdin)){
            printf("%d %p %s %s", argsNum, args, cmd, input);
            free(input);
            error_handle("fgets");
        }

        for(int i=0; i < MAX_NUM_ARGS; i++){
            args[i] = (char*)calloc(MAX_DIRNAME_SIZE, sizeof(char));
        }
        argsNum = input_tokenize(input, cmd, args);

        if(!strcmp(cmd, "\n")){
            break;
        }
        else if(!strcmp(cmd, "createFile") && argsNum == 1){
            if(!createFile(args[0])) printf("File %s creato\n", args[0]);
        }
        else if(!strcmp(cmd, "eraseFile") && argsNum == 1){
            if(!eraseFile(args[0])) printf("File %s cancellato\n", args[0]);
        }
        else if(!strcmp(cmd, "open") && argsNum == 1){
            FileHandle* fh = openFile(args[0]);
        }
        else if(!strcmp(cmd, "write") && argsNum == 1){
            FileHandle* fh = openFile(args[0]);
            if(!fh){
                puts("Impossibile scrivere sul file");
                continue;
            }
            char* data = (char*)malloc(sizeof(char) * MAX_INPUT_SIZE);
            puts("Cosa vorresti scrivere sul file?");
            fgets(data, MAX_INPUT_SIZE, stdin);
            writeOnFile(fh, data, MAX_INPUT_SIZE);
            free(data);
            closeFile(fh);
        }
        else if(!strcmp(cmd, "read") && argsNum == 1){
            FileHandle* fh = openFile(args[0]);
            if(!fh){
                puts("Impossibile scrivere sul file");
                continue;
            }
            char* data = (char*)malloc(sizeof(char) * MAX_INPUT_SIZE);
            free(data);
            closeFile(fh);
        }
        else if(!strcmp(cmd, "seek") && argsNum == 1){
            
        }
        else if(!strcmp(cmd, "createDir") && argsNum == 1){
            
        }
        else if(!strcmp(cmd, "eraseDir") && argsNum == 1){
            
        }
        else if(!strcmp(cmd, "changeDir") && argsNum == 1){
            
        }
        else if(!strcmp(cmd, "listDir\n")){
            if(argsNum != 0){
                printf("USAGE: listDir\n");
                continue;
            }
            
        }
        else{
            printf("USAGE: [command] [param]\n");
        }
    }while(1);
    puts("Sto terminando il programma");
    fflush(stdin);
    for(int i = 0; i < MAX_NUM_ARGS; i++){
        free(args[i]);
    }
    free(args);
    free(cmd);
    free(input);
}