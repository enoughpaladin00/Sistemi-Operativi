all: FAT_proj

FAT_main.o:	FAT_main.c FAT_file_system.h
	gcc -g -c -o FAT_main.o FAT_main.c

FAT_file_system.o:	FAT_file_system.c FAT_file_system.h
	gcc -g -c -o FAT_file_system.o FAT_file_system.c

FAT_proj:	FAT_main.o FAT_file_system.o
	gcc -g -o FAT_proj FAT_main.o FAT_file_system.o

clean:
	rm -rf *.o