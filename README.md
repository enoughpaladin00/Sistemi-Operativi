# FAT File System

Il progetto implementa un file system di tipo FAT (File Allocation Table) con memoria mappata su buffer in C.

## Struttura del progetto

Il progetto è composto dai seguenti file:

- 'FAT_file_system.h': Dichiarazione delle strutture dati, delle librerie e delle funzione adoperate all'interno del progetto.
- 'FAT_file_system.c': Implementazione delle funzioni del progetto.
- 'FAT_main.c': Implementazione della funzione 'main' e gestione dell'interfaccia utente.
- 'makefile': Script per compilare il progetto.

## Uso

### Compilazione

Per compilare il progetto, eseguire i seguenti comandi nella directory del progetto:

'''bash
make
'''

Dopo la compilazione, se vuoi eliminare file superflui, esegui:

'''bash
make clean
'''

###Esecuzione

Per eseguire il file system, esegui il seguente comando:

'''bash
./FAT_proj <fileSystemName>
'''

Dove '<fileSystemName>' è il nome del file che verrà utilizzato come memoria del file system.

### Comandi disponibili

Di seguito sono elencati i comandi disponibili nell'interfaccia utente del file system:

- **createFile <fileName>**: Crea un nuovo file con il nome specificato nella cartella corrente.
- **eraseFile <fileName>**: Cancella il file con il nome specificato nella cartella corrente.
- **open <fileName>**: Apre il file con il nome specificato nella cartella corrente.
- **close <fileName>**: Chiude il file attualmente aperto.
- **write**: Scrive dati nel file aperto.
- **read <numytes>**: Legge il numero di byte specificato dal file aperto.
- **seek <pos>**: Sposta il puntatore di lettura/scrittura alla posizione indicata.
- **createDir <dirName>**: Crea una nuova sottocartella con il nome specificato in quella corrente.
- **eraseDir <dirName>**: Cancella, se vuota, una sottocartella con il nome specificato da quella corrente.
- **changeDir <dirName>**: Cambia la cartella corrente con il percorso specificato.
- **listDir**: Elenca i contenuti della cartella corrente.

### Strutture dati impiegate

- **'DirectoryEntry'**: Struttura che rappresenta un file o una cartella.
- **'Directory'**: Struttura che rappresenta una cartella.
- **'FileSystem'**: Struttura che rappresenta un file system.
- **'FileHandle'**: Struttura che rappresenta un file aperto.

## Informazioni generali

Il progetto è stato realizzato dallo studente Jacopo Rossi per il corso di Sistemi Operativi.