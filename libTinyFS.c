#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "error.h"
#include "libDisk.h"

#define BLOCKSIZE 256
#define DEFAULT_DISK_SIZE 10240
#define DEFAULT_DISK_NAME “tinyFSDisk”
typedef int fileDescriptor;

int tfs_mkfs(char *filename, int nBytes){
	int disk = openDisk(filename, nBytes);
	int numBlocks = nBytes/BLOCKSIZE;
	if(disk<0){
		return -1; //DECLARE ERROR	
	}
		
	char superblock[BLOCKSIZE];
	char inode[BLOCKSIZE];


	superblock[0] = 1;
	superblock[1] = 0x44;
	superblock[2] = 1; //bc inode will be block 1
	//need to implement free block listsuperblock[3] = ;
	//superblock[3] = free block list pointer <= numBlocks - 2;
	writeBlock(disk, 1, superblock);
	
	inode[0] = 2;
	inode[1] = 0x44;
	//linked list of structs? 
	//use writeFile to...
	
	//leave block 3 blank?? why?

	//for loop for teh rest of the blocks to initilize as free blocks
	//char freeBlock[BLOCKSIZE]
	//freeBlock[0] = 4;
	//freeBlock[1] = 0x44;
	//freeBlock[2] = 4?
	

	//first block is superblock
	//second block is inode
	//thirds block is file extent
	//fourth block is free block
	
	//in each block:
	//byte 0 to show what type of block it is(check page 3)
	//byte 1 is 0x44 (what is this?? idk)
	//2nd block is address of another block(what this mean?)
	//3nd empty
	//4 first byte of data, says to initialize all data to 0x00 sooo
	return 0;
}
int tfs_mount(char *diskname);

int tfs_unmount(void);

fileDescriptor tfs_openFile(char *name);

int tfs_closeFile(fileDescriptor FD){
	closeDisk(FD);
}
int tfs_writeFile(fileDescriptor FD, char *buffer, int size);

int tfs_deleteFile(fileDescriptor FD);

int tfs_readByte(fileDescriptor FD, char *buffer);

int tfs_seek(fileDescriptor FD, int offset){
	//lseek
}
