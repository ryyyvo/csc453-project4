#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "error.h"
#include "libDisk.h"

#define BLOCKSIZE 256
#define DEFAULT_DISK_SIZE 10240
#define DEFAULT_DISK_NAME “tinyFSDisk”
#define VALID_BYTE 0x44

typedef int fileDescriptor;
int curr_mounted_disk = -1; //-1 means nothing mounted right now

int tfs_mkfs(char *filename, int nBytes){
	int disk = openDisk(filename, nBytes);
	int numBlocks = nBytes/BLOCKSIZE;
	if(disk<0){
		return -1; //DECLARE ERROR	
	}
		
	char superblock[BLOCKSIZE];

	superblock[0] = 1;
	superblock[1] = VALID_BYTE;
	superblock[2] = 1; 
	//need to implement free block list
	//superblock[3] = free block list pointer <= numBlocks - 2;
	writeBlock(disk, 0, superblock);

	char inode[BLOCKSIZE];	
	inode[0] = 2;
	inode[1] = VALID_BYTE;
	//linked list of structs?, need file name and metadata 
	
	writeBlock(disk, 1, superblock);

	//leave block 3 blank?? have to change i in for loop then
	
	char freeBlock[BLOCKSIZE];
	freeBlock[0] = 4;
	freeBlock[1] = VALID_BYTE;
	//freeBlock[2] = 4? 
	
	for(int i = 2; i<numBlocks; i++){
		writeBlock(disk, i,freeBlock);
	}

	//types of blocks
	//first block is superblock
	//second block is inode
	//thirds block is file extent - means block as file data
	//fourth block is free block
	
	//in each block:
	//byte 0 to show what type of block it is(check page 3)
	//byte 1 is 0x44 (what is this?? idk)
	//2nd block is address of another block(what this mean?)
	//3nd empty
	//4 first byte of data, says to initialize all data to 0x00 sooo
	return 0;
}
int tfs_mount(char *diskname){
	if(curr_mounted_disk != -1){
		return alreadyMounted;
	}
	int disk = openDisk(filename, nBytes);
	if(disk<0){
        return -1; //DECLARE ERROR  
    }

	curr_mounted = disk;
	//from instructions: tfs_mount should verify the file
	//system is the correct type
	//0x44 -second byte of every block- is how you check if file is valid
	
	char buffer[BLOCKSIZE];

	if(readBlock(disk, 0, buffer)<0){
		return -1; //add in correct error code
	}	
	if(buffer[1] != VALID_BYTE){
		printf("file not valid");
		return -1; //add in correct error code
	}

	return 0;	

}
int tfs_unmount(void){
	if(curr_mounted_disk == -1){ //nthg to unmount
		return -1; //add in correct error code
	}
	closeDisk(curr_mounted_disk);
	curr_mounted_disk = -1;
}


fileDescriptor tfs_openFile(char *name);

int tfs_closeFile(fileDescriptor FD){
	closeDisk(FD);
}
int tfs_writeFile(fileDescriptor FD, char *buffer, int size);

int tfs_deleteFile(fileDescriptor FD);

int tfs_readByte(fileDescriptor FD, char *buffer);

int tfs_seek(fileDescriptor FD, int offset){
	if(lseek(disk, offset, SEEK_SET) == -1){
		return -1; //add in correct code
	}
	return 0;
}

