#include "FAT_file_system.h"

/*
Funzione che inizializza la mappatura del buffer e le variabili del file system
*/
void launch_fs(const char *filename){
    
    int fd = open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if(fd == -1) error_handle("open");

    if(ftruncate(fd, BLOCK_SIZE * MAX_BLOCKS)) error_handle("ftruncate");

    fs_buffer = mmap(NULL, BLOCK_SIZE * MAX_BLOCKS, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(fs_buffer == MAP_FAILED) error_handle("mmap");

    if(close(fd)) error_handle("close");

    fs.current_dir = &fs.root;
    fs.root.elemCount = 0;

    for(int i = 0; i < MAX_BLOCKS; i++){
        fs.fat[i] = FAT_FREE;
    }
}

/*
Funzione che gestisce gli errori
*/
void error_handle(const char* error){
    dprintf(2, "ERRORE %s: %d\n",error, errno);
    exit(-1);
};

/*
Funzione che prende una stringa, ne conta le parole e ci riempie il vettore args
*/
int input_tokenize(char* input, char* cmd, char** args){
    int count = 0;

    char* token = strsep(&input, " \n");
    strcpy(cmd, token);

    while((token = strsep(&input, " \n")) != NULL){
        strcpy(args[count], token);
        count++;
    }
    return count-1;
}

/*
Funzione ausiliaria che cerca un blocco libero. Ritorna il numero del blocco se lo trova, -1 altrimenti
*/
int find_block(){
    for(int i=0; i < MAX_BLOCKS; i++){
        if(fs.fat[i] == FAT_FREE) return i;
    }
    return -1;
}

/*
Funzione ausiliaria che cerca un file in una directory. Restituisce l'indice del file se lo trova, -1 altrimenti
*/
int find_file(const char* name, Directory *dir){
    for(int i=0; i < dir->elemCount; i++){
        if(strcmp(dir->head[i].name, name) == 0) return  i;
    }
    return -1;
}

int aux(char* buffer){
    int i = 0;
    while(buffer[i] != '\n' && buffer[i] != '\0'){
        i++;
    }
    if(buffer[i] == '\n')
        buffer[i] = '\0';
    return i;
}

int printBuffer(const char* buffer, int maxSize){
    int i = 0;
    while(buffer[i] != '\0' || buffer[i] != '\n'){
        printf("%c", buffer[i]);
        i++;
    }
    return i;
}

/*
Funzione che crea un file, nella directory corrente dato il suo nome
*/
int createFile(char* fileName){
    if(fs.current_dir->elemCount >= MAX_DIR_SIZE){
        fputs("ERRORE: La cartella e' piena", stderr);
        return -1;
    }
    if(find_file(fileName, fs.current_dir) != -1){
        fputs("ERRORE: Esiste un elemento con quel nome!", stderr);
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
    entry->is_open = 0;
    return 0;
}

/*
Funzione che dato il nome di un file, lo cancella se lo trova nella directory corrente
*/
int eraseFile(char* fileName){
    int file = find_file(fileName, fs.current_dir);
    if(file == -1){
        fputs("ERRORE: File non trovato o non esistente", stderr);
        return -1;
    }
    DirectoryEntry* entry = &fs.current_dir->head[file];
    if(entry->is_dir){
        fputs("ERRORE: Per cancellare una directory usa il comando eraseDir [dirName]", stderr);
        return -1;
    }
    if(entry->is_open){
        fputs("ERRORE: Non puoi eliminare un file aperto", stderr);
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

/*
Funzione che prende in input un nome di un file e se lo trova nella cartella corrente lo apre
*/
FileHandle* openFile(const char *fileName){
    int file = find_file(fileName, fs.current_dir);
    if(file == -1){
        fputs("ERRORE: File non trovato o non esistente", stderr);
        return NULL;
    }

    DirectoryEntry* entry = &fs.current_dir->head[file];
    if(entry->is_dir){
        fputs("ERRORE: Non puoi aprire una directory come se fosse un file", stderr);
        return NULL;
    }

    if(entry->is_open){
        fputs("ERRORE: Non puoi aprire un file già aperto", stderr);
        return NULL;
    }

    entry->is_open = 1;

    FileHandle* fh = (FileHandle*)malloc(sizeof(FileHandle));
    fh->data = fs_buffer + entry->start * BLOCK_SIZE;
    fh->pointer = 0;
    fh->size = entry->size;
    fh->entry = entry;
    return fh;
}

/*
Funzione che prende in input un file e lo chiude
*/
void closeFile(FileHandle* fh){
    writeOnFile(fh, "\0", 1);
    fh->entry->is_open = 0;
    free(fh);
}

/* 
Funzione che prende in ingresso il FileHandle di un file, un buffer e la sua lunghezza e scrive nel file il contenuto del buffer.
Restituisce il numero di byte scritti se si ha esito positivo, -1 se si ha un errore
*/
int writeOnFile(FileHandle* fh, const char* data, int length){
    int written = 0;
    while(length > 0){
        int curr_block = fh->pointer / BLOCK_SIZE;
        int block_cursor = fh->pointer % BLOCK_SIZE;
        int free_block = BLOCK_SIZE - block_cursor;
        //Nella parte di blocco libera entra tutto quello che devo scrivere? Sì -> devo scrivere $(length) bytes : No -> devo scrivere tanti bytes quanti ne rimangono sul blocco
        int to_write = length < free_block ? length : free_block;
    
        printf("Writing %d bytes at block %d, cursor %d\n", to_write, curr_block, block_cursor);
        memcpy(fs_buffer + curr_block * BLOCK_SIZE + block_cursor, data, to_write);
        printf("HO SCRITTO: %s\n", fs_buffer + curr_block * BLOCK_SIZE + block_cursor);
        //Se la parte rimanente del blocco non basta, aggiungo un altro blocco al file
        if(!(length < free_block)){
            int next_block = find_block();
            if(next_block == -1) return -1;
            fs.fat[curr_block] = next_block;
            fs.fat[next_block] = FAT_EOF;
        }

        DirectoryEntry* entry = fh->entry;
        //Aggiorno le info del FileHandle e le info di ciò che devo scrivere
        fh->pointer += to_write;
        fh->size += to_write;
        length -= to_write;
        data += to_write;
        written += to_write;
        entry->size += to_write;
    }
    puts("INFO: Scrittura eseguita");
    return written;
}

/*  
Funzione che prende in ingresso il FileHandle di un file, la dimensione massima di byte da leggere e un buffer e scrive sul buffer ciò che ha letto.
Restituisce il numero di byte letti oppure -1 se si ha un errore
*/
int readFromFile(FileHandle* fh, int maxSize, char* buffer){
    int read = 0;
    while(maxSize > 0){
        int curr_block = fh->pointer / BLOCK_SIZE;
        int block_cursor = fh->pointer % BLOCK_SIZE;
        int free_block_space = BLOCK_SIZE - block_cursor;
        int to_read = maxSize < free_block_space ? maxSize : free_block_space;

        printf("Reading %d bytes at block %d, cursor %d\n", to_read, curr_block, block_cursor);
        memcpy(buffer, fs_buffer + curr_block * BLOCK_SIZE + block_cursor, maxSize);

        fh->pointer += to_read;
        maxSize -= to_read;
        buffer += to_read;
        read += to_read;

        if(fs.fat[curr_block] == FAT_EOF) break;
    }
    return read;
}

int seek(FileHandle* fh, int pos){
    if(pos >= 0 && pos<=fh->size){
        fh->pointer = pos;
    }
}

int createDir(const char* dirName){
    if(fs.current_dir->elemCount >= MAX_DIR_SIZE){
        fputs("ERRORE: La cartella e' piena", stderr);
        return -1;
    }
    if(find_file(dirName, fs.current_dir) != -1){
        fputs("ERRORE: Esiste un ekemento con quel nome!", stderr);
        return -1;
    }
    int block = find_block();
    if(block == -1) return -1;

    fs.fat[block] = FAT_EOF;

    DirectoryEntry *entry = &fs.current_dir->head[fs.current_dir->elemCount++];
    strncpy(entry->name, dirName, MAX_DIRNAME_SIZE);
    entry->start = block;
    entry->size = 0;
    entry->is_dir = 1;
    entry->is_open = 0;

    Directory* newDir = (Directory*)(fs_buffer + block * BLOCK_SIZE);
    newDir->elemCount = 0;
    newDir->parent = (struct Directory*)fs.current_dir;
    return 0;
}

int changeDir(char* path){
    char* token;
    while(token = strsep(&path, "/")){
        if(!strcmp(token, "..")){
            fs.current_dir = (Directory*)fs.current_dir->parent;
        }
        else if(!strcmp(token, ".")){
            continue;
        }
        else{
            int index = find_file(token, fs.current_dir);
            if(index == -1 || !fs.current_dir->head[index].is_dir){
                fputs("ERRORE: percorso non trovato", stderr);
                return -1;
            }

            DirectoryEntry* entry = &fs.current_dir->head[index];
            fs.current_dir = (Directory*)(fs_buffer + entry->start * BLOCK_SIZE);
        }
    }
    return 0;
}

int listDir(){
    printf("[DIR]\t.\n[DIR]\t..\n");
    for(int i=0; i < fs.current_dir->elemCount; i++){
        DirectoryEntry* entry = &fs.current_dir->head[i];
        if(entry->is_dir) printf("[DIR]\t%s\t%dB\n", entry->name, entry->size);
        else printf("[FILE]%s\t%dB\n", entry->name, entry->size);
    }
}

int eraseDir(char* dirName){
    int index = find_file(dirName, fs.current_dir);
    if(index == -1){
        fputs("ERRORE: La cartella che stai cercando di cancellare non esiste", stderr);
        return -1;
    }
    DirectoryEntry* entry = &fs.current_dir->head[index];
    if(entry->is_dir){
        fputs("ERRORE: Stai cercando di cancellare un file nel modo sbagliato", stderr);
        return -1;
    }
    Directory* dir = (Directory*)(fs_buffer + entry->start * BLOCK_SIZE);

    if(dir->elemCount != 0){
        fputs("ERRORE: Non puoi eliminare una cartella piena", stderr);
        return -1;
    }

    fs.fat[entry->start] = FAT_FREE;

    for(int i = 0; i < fs.current_dir->elemCount - 1; i++){
        fs.current_dir->head[i] = fs.current_dir->head[i+1];
    }
    fs.current_dir->elemCount--;

    return 0;
}