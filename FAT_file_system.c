#include "FAT_file_system.h"

/* Funzione che inizializza la mappatura del buffer e le variabili del file system */
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

/* Funzione che gestisce gli errori */
void error_handle(const char* error){
    dprintf(2, "ERRORE %s: %d\n",error, errno);
    exit(-1);
};

/* Funzione che prende una stringa, ne conta le parole e ci riempie il vettore args */
int input_tokenize(char* input, char* cmd, char** args){
    int count = 0;

    char temp[MAX_INPUT_SIZE];
    strcpy(temp, input);
    strcpy(cmd, strtok(input, " "));
    while(strtok(NULL, " ") || strtok(NULL, "\n")){
        count++;
    }
    strtok(temp, " ");
    int i = 0;
    while(i < count){
        strcpy(args[i], strtok(NULL, " "));
        i++;
    }

    if(i > 0){
        char* word = args[i-1];
        int change;
        for(int j = 0; word[j] != '\0'; j++){
            change = j;
        }
        word[change] = '\0';
    }
    return count;
}

/* Funzione ausiliaria che cerca un blocco libero. Ritorna il numero del blocco se lo trova, -1 altrimenti */
int find_block(){
    for(int i=0; i < MAX_BLOCKS; i++){
        if(fs.fat[i] == FAT_FREE) return i;
    }
    return -1;
}

/* Funzione ausiliaria che cerca un file in una directory. Restituisce l'indice del file se lo trova, -1 altrimenti */
int find_file(const char* name, Directory *dir){
    for(int i=0; i < dir->elemCount; i++){
        if(strcmp(dir->head[i].name, name) == 0) return  i;
    }
    return -1;
}

/* Funzione che crea un file, nella directory corrente dato il suo nome */
int createFile(char* fileName){
    if(fs.current_dir->elemCount >= MAX_DIR_SIZE){
        puts("Directory full");
        return -1;
    }
    if(find_file(fileName, fs.current_dir) != -1){
        puts("Esiste un file con quel nome!");
        return -1;
    }
    int block = find_block();
    if(block == -1) return -1;

    fs.fat[block] = FAT_EOF;

    DirectoryEntry *entry = &fs.current_dir->head[fs.current_dir->elemCount++];
    strncpy(entry->name, fileName, MAX_DIRNAME_SIZE);
    entry->start = block;
    entry->size = 0;
    entry->is_dir = 0;
    return 0;
}

/*  Funzione che dato il nome di un file, lo cancella se lo trova nella directory corrente */
int eraseFile(char* fileName){
    int file = find_file(fileName, fs.current_dir);
    if(file == -1){
        puts("File non trovato o non esistente");
        return -1;
    }
    DirectoryEntry* entry = &fs.current_dir->head[file];
    if(entry->is_dir){
        puts("Per cancellare una directory usa il comando eraseDir [dirName]");
        return -1;
    }

    int block = entry->start;
    while(block != FAT_EOF){
        int next_block = fs.fat[block];
        fs.fat[block] = FAT_FREE;
        block = next_block;
    }
    
    for(int i = file; i < fs.current_dir->elemCount - 1; i++){
        fs.current_dir->head[i] = fs.current_dir->head[i + 1];
    }
    fs.current_dir->elemCount--;
    return 0;
}

/*  Funzione che prende in input un nome di un file e se lo trova nella cartella corrente lo apre */
FileHandle* openFile(const char *fileName){
    int file = find_file(fileName, fs.current_dir);
    if(file == -1){
        puts("File non trovato o non esistente");
        return NULL;
    }

    DirectoryEntry* entry = &fs.current_dir->head[file];
    if(entry->is_dir){
        puts("Non puoi aprire una directory come se fosse un file");
        return NULL;
    }

    FileHandle* fh = (FileHandle*)malloc(sizeof(FileHandle));
    fh->data = fs_buffer + entry->start * BLOCK_SIZE;
    fh->pointer = 0;
    fh->size = entry->size;
    fh->entry = entry;
    return fh;
}

/*  Funzione che prende in input un file e lo chiude */
void closeFile(FileHandle* fh){
    free(fh);
}

/*  Funzione che prende in ingresso il FileHandle di un file, un buffer e la sua lunghezza e scrive nel file il contenuto del buffer.
    Restituisce 0 se si ha esito positivo, -1 se si ha esito negativo */
int writeOnFile(FileHandle* fh, const char* data, int length){
    while(length > 0){
        int curr_block = fh->pointer / BLOCK_SIZE;
        int block_cursor = fh->pointer % BLOCK_SIZE;
        int free_block = BLOCK_SIZE - block_cursor;
        //Nella parte di blocco libera entra tutto quello che devo scrivere? Sì -> devo scrivere $(length) bytes : No -> devo scrivere tanti bytes quanti ne rimangono sul blocco
        int to_write = length < free_block ? length : free_block;
    
        memcpy(fs_buffer + curr_block * BLOCK_SIZE + block_cursor, data, to_write);

        //Se la parte rimanente del blocco non bastava, aggiungo un altro blocco al file
        if(!(length < block_cursor)){
            int next_block = find_block();
            if(next_block == -1) return -1;
            fs.fat[curr_block] = next_block;
            fs.fat[next_block] = FAT_EOF;
        }

        //Aggiorno le info del FileHandle e le info di ciò che devo scrivere
        fh->pointer += to_write;
        fh->size += to_write;
        length -= to_write;
        data += to_write;
    }
    puts("Scrittura eseguita");
    return 0;
}