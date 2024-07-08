#include "FAT_file_system.h"

#define MAX_CMD_SIZE 32

int main(int argc, char** argv){
    if(argc < 2){
        fprintf(stderr, "USAGE: ./FAT_main [fileSystemName]");
        exit(1);
    }

    printf("INFO: Inizializzo il file system... ");
    launch_fs(argv[1]);
    puts("FATTO");
    char* input = (char*)malloc(sizeof(char)*MAX_INPUT_SIZE);
    char* cmd = (char*)malloc(sizeof(char)*MAX_CMD_SIZE);
    char** args = (char**)malloc(sizeof(char*) * MAX_NUM_ARGS);
    for(int i=0; i < MAX_NUM_ARGS; i++){
        args[i] = (char*)malloc(MAX_DIRNAME_SIZE * sizeof(char));
    }
    int argsNum;
    FileHandle* fh = NULL;

    do{
        printf("--------------------------\n$: ");
        if(!fgets(input, MAX_INPUT_SIZE, stdin)){
            printf("%d %p %s %s", argsNum, args, cmd, input);
            free(input);
            error_handle("fgets");
        }

        argsNum = input_tokenize(input, cmd, args);

        if(!strcmp(cmd, "")){
            break;
        }
        else if(!strcmp(cmd, "createFile") && argsNum == 1){
            if(!createFile(args[0])) printf("File %s creato\n", args[0]);
        }
        else if(!strcmp(cmd, "eraseFile") && argsNum == 1){
            if(!eraseFile(args[0])) printf("File %s cancellato\n", args[0]);
        }
        else if(!strcmp(cmd, "open") && argsNum == 1){
            fh = openFile(args[0]);
            printf("INFO: Il file %s è stato aperto\n", fh->entry->name);
        }
        else if(!strcmp(cmd, "close") && argsNum == 1){
            closeFile(fh);
            fh = NULL;
        }
        else if(!strcmp(cmd, "write") && argsNum < 2){
            if(!fh){
                fputs("ERRORE: Impossibile scrivere sul file", stderr);
                continue;
            }
            char* data = (char*)malloc(sizeof(char) * MAX_INPUT_SIZE);
            puts("Cosa vorresti scrivere sul file?");
            fgets(data, MAX_INPUT_SIZE, stdin);
            int input_size = aux(data);

            if(writeOnFile(fh, data, input_size) == -1){
                fputs("ERRORE: write non riuscita", stderr);
            }
            free(data);
        }
        else if(!strcmp(cmd, "read") && argsNum < 2){
            if(!fh){
                fputs("ERRORE: Impossibile leggere dal file\n", stderr);
                continue;
            }
            int bytes_to_read = MAX_INPUT_SIZE;
            printf("%s\n", args[0]);
            if(strcmp(args[0], "")) bytes_to_read = atoi(args[0]);
            char* data = (char*)calloc(MAX_INPUT_SIZE, sizeof(char));
            printf("INFO: Leggo dal file %d bytes\n", readFromFile(fh, bytes_to_read, data));
            printf("STAMPO: %s\n", data);
            free(data);
        }
        else if(!strcmp(cmd, "seek") && argsNum == 1){
            if(!fh){
                fputs("ERRORE: Impossibile muovere il cursore sul file", stderr);
                continue;
            }
            seek(fh, atoi(args[0]));
        }
        else if(!strcmp(cmd, "createDir") && argsNum == 1){
            if(!createDir(args[0])) printf("Cartella %s creata\n", args[0]);
        }
        else if(!strcmp(cmd, "eraseDir") && argsNum == 1){
            if(!eraseDir(args[0])) printf("Cartella %s eliminata\n", args[0]);
        }
        else if(!strcmp(cmd, "changeDir") && argsNum == 1){
            if(!changeDir(args[0])) printf("Entro nella cartella %s\n", args[0]);
        }
        else if(!strcmp(cmd, "listDir")){
            listDir();
            
        }
        else{
            printf("USAGE: [command] [param]\n");
        }
    }while(1);
    puts("INFO: Sto terminando il programma");
    fflush(stdin);
    fflush(stdout);
    fflush(stderr);
    for(int j = 0; j < MAX_NUM_ARGS; j++){
        free(args[j]);
    }
    free(args);
    free(cmd);
    free(input);
    return 0;
}