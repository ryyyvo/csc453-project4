#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "error.h"

#define BLOCKSIZE 256

int openDisk(char *filename, int nBytes) {
	FILE *fptr;
	if (nBytes < BLOCKSIZE) {
		printf("nbytes less than blocksize not allowed");
		return invalidNByteSize; //failure
	}
	if (nBytes == 0) {
		// existing disk open, but have to double check
		// content must NOT be overwritten
		// if file doesn't exist, return diskNotExist 
		fptr = fopen(filename, "r+"); //r+ only works if it exists
		if (!fptr){
			return diskNotExist;
		}
		return fileno(fptr);
	}
	if (nBytes % BLOCKSIZE != 0) {
		nBytes -= (nBytes%BLOCKSIZE);	
		fptr = fopen(filename, "w+");//w+ if file new
		if (!fptr){
			printf("can't create file");
			return -4; //make error for fopen failure

		}
	}

	char emptyData = 0;
	for(int i = 0; i<nBytes; i++){
		fwrite(&emptyData,sizeof(emptyData), 1, fptr);	
	}
	fflush(fptr);
	
	return fileno(fptr);
}

int closeDisk(int disk){
	//do we have to check if disk is a valid param?
	if (fclose(disk) == -1) {
		return fileCloseFailure;
	}	

	return 0;
}

/* readBlock() reads an entire block of BLOCKSIZE bytes from the open
disk (identified by ‘disk’) and copies the result into a local buffer
(must be at least of BLOCKSIZE bytes). The bNum is a logical block
number, which must be translated into a byte offset within the disk. The
translation from logical to physical block is straightforward: bNum=0
is the very first byte of the file. bNum=1 is BLOCKSIZE bytes into the
disk, bNum=n is n*BLOCKSIZE bytes into the disk. On success, it returns
0. -1 or smaller is returned if disk is not available (hasn’t been
opened) or any other failures. You must define your own error code
system. */

/* only reads 256 bytes per call? same size as buffer. file divided into 256 bytes sized blocks
 * so bNum tells you which block number you want to read from <= so have to calculate an offset(multiply by 
 * 256) */

int readBlock(int disk, int bNum, void *block){
	//error checking for valid bNum? 
	/*char buffer[BLOCKSIZE];*/
	block = malloc(sizeof(char)*BLOCKSIZE);
	if(lseek(disk, bNum * BLOCKSIZE, SEEK_SET) == -1){
		return readBlockFuncFailure; 
	}
	if(read(disk, block, BLOCKSIZE) == -1){
		return readBlockFuncFailure;
	}	
	return 0;
}

int writeBlock(int disk, int bNum, void *block){
}
