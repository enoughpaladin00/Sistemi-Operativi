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

    if(ftruncate(fd, sizeof(FileSystem) + BLOCK_SIZE * MAX_BLOCKS)) error_handle("ftruncate");

    fs_map = mmap(NULL, sizeof(FileSystem) + BLOCK_SIZE * MAX_BLOCKS, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(fs_map == MAP_FAILED) error_handle("mmap");
    fs_buffer = fs_map + sizeof(FileSystem);
    fs.root = (DirectoryEntry*)fs_buffer;

    
    if(close(fd)) error_handle("close");

    if(fs.root->elemCount == 0){
        fs.root->start = 0;
        fs.root->is_dir = 1;
        fs.root->is_open = 1;
        fs.root->parent = NULL;
        fs.fat[0] = FAT_EOF;
        fs.current_dir = fs.root;

        for(int i = 1; i < MAX_BLOCKS; i++){
            fs.fat[i] = FAT_FREE;
        }
    }else{
        fs = *(FileSystem*)fs_map;
        fs.root = (DirectoryEntry*) fs_buffer;
        fs.current_dir = fs.root;
    }
}

void reset_fs(const char* filename){
    memset(fs_map, 0, sizeof(FileSystem) + BLOCK_SIZE * MAX_BLOCKS);
    fs.root->elemCount = 0;
}

void close_fs(const char* filename){
    memcpy(fs_map, &fs, sizeof(FileSystem));
    syncro(fs_map);
}

/*
Funzione che gestisce gli errori
*/
void error_handle(const char* error){
    fprintf(stderr, "ERRORE %s: %d\n",error, errno);
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
    int firstBlockSize = (fs_buffer + (dir->start + 1) * BLOCK_SIZE - (char*)&dir->head);
    for(int i=0; i < dir->elemCount; i++){
        int block;
        if(i < firstBlockSize) block = *(&dir->head + i);
        else{
            int curr_block = (i - firstBlockSize) / BLOCK_SIZE;
            int block_cursor = (i - firstBlockSize) % BLOCK_SIZE;
            block = *((int*)fs_buffer + curr_block * BLOCK_SIZE + block_cursor);
        }
        if(strcmp(fs_buffer + block * BLOCK_SIZE, name) == 0) return  i;
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
    *(&fs.current_dir->head + fs.current_dir->elemCount) = block;

    DirectoryEntry *entry = (DirectoryEntry*)(fs_buffer + block * BLOCK_SIZE);
    strncpy(entry->name, fileName, MAX_DIRNAME_SIZE);
    entry->start = block;
    entry->elemCount = 0;
    entry->is_dir = 0;
    entry->is_open = 0;
    fs.current_dir->elemCount++;

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
    int fileBlock = *(&fs.current_dir->head + file);
    DirectoryEntry *entry = (DirectoryEntry*)(fs_buffer + fileBlock * BLOCK_SIZE);
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
        memset(fs_buffer + block * BLOCK_SIZE, 0, BLOCK_SIZE);
        block = next_block;
    }
    
    for(int i = file; i < fs.current_dir->elemCount - 1; i++){
        *(&fs.current_dir->head + i) = *(&fs.current_dir->head + i+1);
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
    int block = *(&fs.current_dir->head + file);
    DirectoryEntry *entry = (DirectoryEntry*)(fs_buffer + block * BLOCK_SIZE);
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
    fh->data = (char*)&entry->head;
    fh->cursor = 0;
    fh->entry = entry;
    return fh;
}

/*
Funzione che prende in input un fileHandle e lo chiude, scrivendo alla fine del suo buffer un carattere di delimitazione
*/
void closeFile(FileHandle* fh){
    if(fh->entry->is_open){
        seek(fh, fh->entry->elemCount);
        writeOnFile(fh, "\0", 1);
        fh->entry->is_open = 0;
        free(fh);
    }
}

/* 
Funzione che prende in ingresso il FileHandle di un file, un buffer e la sua lunghezza e scrive nel file il contenuto del buffer.
Restituisce il numero di byte scritti se si ha esito positivo, -1 se si ha un errore
*/
int writeOnFile(FileHandle* fh, const char* buffer, int length){
    if (!fh || !buffer) {
        fputs("ERRORE: FileHandle o buffer nullo\n", stderr);
        return -1;
    }
    int block = fh->entry->start;
    int written = 0;
    char* writeFromHere = fh->data + fh->cursor;
    int to_write = length < (fs_buffer + (fh->entry->start + 1) * BLOCK_SIZE - fh->data)? length : fs_buffer + (fh->entry->start + 1) * BLOCK_SIZE - fh->data;
    while(length > 0){
        memcpy(writeFromHere, buffer, to_write);
        length -= to_write;
        buffer += to_write;
        fh->cursor += to_write;
        fh->entry->elemCount += to_write;
        written += to_write;
        if(length > 0){
            int index = find_block();
            fs.fat[block] = index;
            fs.fat[index] = FAT_EOF;
            block = index;
            writeFromHere = fs_buffer + index * BLOCK_SIZE;
        }
        to_write = length < BLOCK_SIZE? length : BLOCK_SIZE;
    }
    return written;
}

/*  
Funzione che prende in ingresso il FileHandle di un file, la dimensione massima di byte da leggere e un buffer e scrive sul buffer ciò che ha letto.
Restituisce il numero di byte letti oppure -1 se si ha un errore
*/
int readFromFile(FileHandle* fh, const char* buffer, int maxSize) {
    if (!fh || !buffer) {
        fputs("ERRORE: FileHandle o buffer nullo\n", stderr);
        return -1;
    }

    DirectoryEntry *entry = fh->entry;

    int read = 0;
    int block = entry->start;

    int bytesToRead = (maxSize < entry->elemCount)? maxSize : entry->elemCount;
    char firstBlockData = (fs_buffer + (entry->start + 1) * BLOCK_SIZE) - fh->data;
    char* readFromHere;
    int to_read;
    if(fh->cursor <= firstBlockData){
        readFromHere = (char*)&entry->head + fh->cursor;
        to_read = firstBlockData - fh->cursor;
    }
    else{
        int cursor = fh->cursor;
        cursor -= firstBlockData;
        block = fs.fat[entry->start];
        int block_cursor = cursor / BLOCK_SIZE;
        int curr_cursor = cursor % BLOCK_SIZE;
        for(int i = 0; i < block_cursor; i++){
            block = fs.fat[block];
        }
        readFromHere = fs_buffer + block * BLOCK_SIZE + curr_cursor;
        to_read = BLOCK_SIZE - curr_cursor;
    }

    while(bytesToRead > 0){
        memcpy((char*)buffer, readFromHere, to_read);
        bytesToRead -= to_read;
        read += to_read;
        buffer += to_read;
        fh->cursor += to_read;
        readFromHere = fs_buffer + fs.fat[block] * BLOCK_SIZE;
        block = fs.fat[block];
        to_read = BLOCK_SIZE;
    }
    return read;
}


/*
Funzione che prende in ingresso un puntatore a fileHandle e un intero e sposta il cursore del file nella posizione dell'intero
*/
int seek(FileHandle* fh, int pos){
    if(pos >= 0 && pos<=fh->entry->elemCount){
        fh->cursor = pos;
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

    int firstBlockSize = fs_buffer + (fs.current_dir->start + 1) * BLOCK_SIZE - (char*)&fs.current_dir->head;
    
    
    DirectoryEntry *entry = (DirectoryEntry*)(fs_buffer + block * BLOCK_SIZE);
    memset(entry, 0, BLOCK_SIZE);
    strncpy(entry->name, dirName, MAX_DIRNAME_SIZE);
    entry->start = block;
    entry->is_dir = 1;
    entry->is_open = 0;
    entry->elemCount = 0;
    entry->parent = (struct DirectoryEntry*)fs.current_dir;
    
    if(fs.current_dir->elemCount <= firstBlockSize)
        *(&fs.current_dir->head + fs.current_dir->elemCount) = block;
    else{
        int curr_block = (fs.current_dir->elemCount-firstBlockSize)/BLOCK_SIZE;
        int block_cursor = (fs.current_dir->elemCount - firstBlockSize)%BLOCK_SIZE;
        int index = fs.current_dir->start;
        for(int i = 0; i < curr_block; i++){
            index = fs.fat[index];
        }
        *((int*)fs_buffer + index * BLOCK_SIZE + block_cursor) = block;

    }

    fs.current_dir->elemCount++;
    
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
            int block = *(&fs.current_dir->head + index);
            if(index == -1 || !((DirectoryEntry*)(fs_buffer + block * BLOCK_SIZE))->is_dir){
                fputs("ERRORE: percorso non trovato", stderr);
                return -1;
            }
            DirectoryEntry *entry = (DirectoryEntry*)(fs_buffer + block * BLOCK_SIZE);
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
        int block = *(&fs.current_dir->head + i);
        DirectoryEntry *entry = (DirectoryEntry*)(fs_buffer + block * BLOCK_SIZE);
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
    int dirBlock = *(&fs.current_dir->head + index);
    DirectoryEntry *dir = (DirectoryEntry*)(fs_buffer + dirBlock * BLOCK_SIZE);
    if(!dir->is_dir){
        fputs("ERRORE: Stai cercando di cancellare un file nel modo sbagliato", stderr);
        return -1;
    }

    if(dir->elemCount != 0){
        fputs("ERRORE: Non puoi eliminare una cartella piena", stderr);
        return -1;
    }

    int block = dir->start;
    while(block != FAT_EOF){
        int next_block = fs.fat[block];
        fs.fat[block] = FAT_FREE;
        memset(fs_buffer + block * BLOCK_SIZE, 0, BLOCK_SIZE);
        block = next_block;
    }

    for(int i = 0; i < fs.current_dir->elemCount - 1; i++){
        *(&fs.current_dir->head + i) = *(&fs.current_dir->head + i+1);
    }
    fs.current_dir->elemCount--;

    return 0;
}