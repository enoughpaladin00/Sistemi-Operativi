#include "FAT_file_system.h"

FileSystem fs;
char* fs_buffer = NULL;
char* fs_map = NULL;
/*
Funzione che inizializza la mappatura del buffer e le variabili del file system
*/
void launch_fs(const char *filename){
    
    int fd = open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if(fd == -1) error_handle("open");

    if(ftruncate(fd, BLOCK_SIZE * MAX_BLOCKS)) error_handle("ftruncate");

    fs_map = mmap(NULL, sizeof(FileSystem) + BLOCK_SIZE * MAX_BLOCKS, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(fs_map == MAP_FAILED) error_handle("mmap");
    fs_buffer = fs_map + sizeof(FileSystem);

    if(close(fd)) error_handle("close");

    if(((FileSystem*)fs_map)->root.elemCount == 0){
        fs.current_dir = &fs.root;
        fs.root.elemCount = 0;
        fs.root.size = STARTING_DIR_SIZE;
        fs.root.parent = NULL;
        fs.root.head = (int*)fs_buffer;

        for(int i = 0; i < MAX_BLOCKS; i++){
            fs.fat[i] = FAT_FREE;
        }
    }else{
        fs = *(FileSystem*)fs_map;
        fs.current_dir = &fs.root;
        fs.root.head = (int*)fs_buffer;
    }
}

void close_fs(const char* filename){
    memcpy(fs_map, &fs, sizeof(FileSystem));
    syncro(fs_map);
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
int find_file(const char* name, DirectoryEntry *dir){
    for(int i=0; i < dir->elemCount; i++){
        if(strcmp(fs_buffer + (dir->head[i] * BLOCK_SIZE), name) == 0) return  i;
    }
    return -1;
}

/*
Funzione ausiliaria che conta i caratteri di una frase, escludendone il newline e il delimitatore
*/
int aux(char* buffer){
    int i = 0;
    while(buffer[i] != '\n' && buffer[i] != '\0'){
        i++;
    }/*
    if(buffer[i] == '\n')
        buffer[i] = '\0';
    */
    return i;
}

void syncro(void* pointer){
    if (msync(pointer, sizeof(FileSystem) + BLOCK_SIZE * MAX_BLOCKS, MS_SYNC) == -1) {
        error_handle("msync");
    }
}

/*int resizeDir(DirectoryEntry* dir, int newSize){
    if(newSize < dir->elemCount){
        fputs("ERRORE: La grandezza dell'array non basta\n", stderr);
        return -1;
    }
    int* newHead = (int*)realloc(dir->head, sizeof(int)*newSize);
    if(!newHead){
        fputs("Errore: Impossibile ridimensionare directory\n", stderr);
        return -1;
    }
    dir->head = newHead;
    dir->size = newSize;
    return 0;
}*/

/*
Funzione che crea un file, nella directory corrente dato il suo nome
*/
int createFile(char* fileName){

    if(find_file(fileName, fs.current_dir) != -1){
        fputs("ERRORE: Esiste un elemento con quel nome!", stderr);
        return -1;
    }
    int block = find_block();
    if(block == -1) return -1;

    fs.fat[block] = FAT_EOF;
    fs.current_dir->head[fs.current_dir->elemCount++] =  block;

    DirectoryEntry *entry = (DirectoryEntry*)(fs_buffer + (fs.current_dir->head[fs.current_dir->elemCount++] * BLOCK_SIZE));
    strncpy(entry->name, fileName, MAX_DIRNAME_SIZE);
    entry->start = block;
    entry->elemCount = 0;
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
    DirectoryEntry *entry = (DirectoryEntry*)(fs_buffer + fs.current_dir->head[file] * BLOCK_SIZE);
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
        char* i = fs_buffer + (block * BLOCK_SIZE);
        while(i < fs_buffer + ((block + 1) * BLOCK_SIZE)){
            i = 0;
        }
        block = next_block;
    }
    
    for(int i = file; i < fs.current_dir->elemCount - 1; i++){
        /////////////////////////////////////////////////////////ATTENTO
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

    DirectoryEntry *entry = (DirectoryEntry*)(fs_buffer + fs.current_dir->head[file] * BLOCK_SIZE);
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
    fh->size = entry->elemCount;
    fh->entry = entry;
    return fh;
}

/*
Funzione che prende in input un fileHandle e lo chiude, scrivendo alla fine del suo buffer un carattere di delimitazione
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
        if(to_write == free_block){
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
        memcpy(buffer, fs_buffer + curr_block * BLOCK_SIZE + block_cursor, to_read);

        fh->pointer += to_read;
        maxSize -= to_read;
        buffer += to_read;
        read += to_read;

        if(fs.fat[curr_block] == FAT_EOF) break;
    }
    return read;
}

/*
Funzione che prende in ingresso un puntatore a fileHandle e un intero e sposta il cursore del file nella posizione dell'intero
*/
int seek(FileHandle* fh, int pos){
    if(pos >= 0 && pos<=fh->size){
        fh->pointer = pos;
    }
}

/*
Funzione che prende in ingresso una stringa e crea una cartella con quel nome
*/
int createDir(const char* dirName){

    if(find_file(dirName, fs.current_dir) != -1){
        fputs("ERRORE: Esiste un elemento con quel nome!", stderr);
        return -1;
    }
    int block = find_block();
    if(block == -1) return -1;

    fs.fat[block] = FAT_EOF;
    fs.current_dir->head[fs.current_dir->elemCount++] = block;

    DirectoryEntry *entry = (DirectoryEntry*)(fs_buffer + fs.current_dir->head[fs.current_dir->elemCount] * BLOCK_SIZE);
    strncpy(entry->name, dirName, MAX_DIRNAME_SIZE);
    entry->start = block;
    entry->is_dir = 1;
    entry->is_open = 0;
    entry->elemCount = 0;
    entry->parent = (struct DirectoryEntry*)fs.current_dir;
    //Imposto tutti gli altri bit della cartella a 0
    int i = sizeof(char) * MAX_DIRNAME_SIZE;
    i += sizeof(int) * 4;
    i += sizeof(DirectoryEntry);
    i += sizeof(int);
    for(;i < BLOCK_SIZE; i++){
        entry->head[i]= 0;
    }
    return 0;
}

/*
Funzione che prende in ingresso un percorso del file system (stringa) e, se esiste, la imposta come cartella corrente
*/
int changeDir(char* path){
    char* token;
    while(token = strsep(&path, "/")){
        if(!strcmp(token, "..")){
            if(fs.current_dir->parent)
                fs.current_dir = (DirectoryEntry*)fs.current_dir->parent;
            else fputs("ERRORE: Già nella directory root", stderr);
        }
        else if(!strcmp(token, ".")){
            continue;
        }
        else{
            int index = find_file(token, fs.current_dir);
            if(index == -1 || !((DirectoryEntry*)(fs_buffer + fs.current_dir->head[index] * BLOCK_SIZE))->is_dir){
                fputs("ERRORE: percorso non trovato", stderr);
                return -1;
            }

            DirectoryEntry *entry = (DirectoryEntry*)(fs_buffer + fs.current_dir->head[index] * BLOCK_SIZE);
            fs.current_dir = (DirectoryEntry*)(fs_buffer + entry->start * BLOCK_SIZE);
        }
    }
    return 0;
}

/*
Funzione che stampa in output tutte le entry della cartella attuale
*/
int listDir(){
    if(fs.current_dir->parent)
        printf("[DIR]\t.\n[DIR]\t..\n");
    else
        printf("[DIR]\t.\n");
    for(int i=0; i < fs.current_dir->elemCount; i++){
        DirectoryEntry *entry = (DirectoryEntry*)(fs_buffer + (fs.current_dir->head[i] * BLOCK_SIZE));
        if(entry->is_dir) printf("[DIR]\t%s\n", entry->name);
        else printf("[FILE]\t%s\t%dB\n", entry->name, entry->elemCount);
    }
}

/*/
Funzione che prende in ingresso il nome di una sottocartella e, se esiste, la cancella solo se vuota
*/
int eraseDir(char* dirName){
    int index = find_file(dirName, fs.current_dir);
    if(index == -1){
        fputs("ERRORE: La cartella che stai cercando di cancellare non esiste", stderr);
        return -1;
    }
    DirectoryEntry *dir = (DirectoryEntry*)(fs_buffer + fs.current_dir->head[index] * BLOCK_SIZE);
    if(!dir->is_dir){
        fputs("ERRORE: Stai cercando di cancellare un file nel modo sbagliato", stderr);
        return -1;
    }

    if(dir->elemCount != 0){
        fputs("ERRORE: Non puoi eliminare una cartella piena", stderr);
        return -1;
    }

    fs.fat[dir->start] = FAT_FREE;

    for(int i = 0; i < fs.current_dir->elemCount - 1; i++){
        fs.current_dir->head[i] = fs.current_dir->head[i+1];
    }
    fs.current_dir->elemCount--;

    return 0;
}