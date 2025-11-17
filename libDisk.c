#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "error.h"

#define BLOCKSIZE 256

int openDisk(char *filename, int nBytes){
	if (nBytes < BLOCKSIZE){
		printf("nbytes less than blocksize not allowed");
		return invalidNByteSize; //failure
	}
	else if(nBytes == 0){
		//existing disk open, but have to double check
		//	content must NOT be overwritten
		//if file doesn't exist, return diskNotExist 
		FILE *fptr = fopen(filename, "r+"); //r+ only works if it exists
		if(!fptr){
			return diskNotExist;
		}
	}
	else if (nBytes % BLOCKSIZE != 0){
		nBytes -= (nBytes%BLOCKSIZE);	
		FILE *fptr = fopen(filename, "w+");//w+ if file new
	}
	else if(nBytes > BLOCKSIZE){
	/*files content may be overwritten*/
	}

	return 0;
}


int closeDisk(int disk){
}

int readBlock(int disk, int bNum, void *block){
}

int writeBlock(int disk, int bNum, void *block){
}
