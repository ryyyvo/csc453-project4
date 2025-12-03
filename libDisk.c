#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "error.h"
#include "libDisk.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define BLOCKSIZE 256

int openDisk(char *filename, int nBytes)
{
	int fptr;

	if (nBytes == 0)
	{
		
		fptr = open(filename, O_RDWR);
		if (fptr<0)
		{
			return INVALID_ACCESS;
		}
		return fptr;
	}

	if (nBytes < BLOCKSIZE)
    {
        printf("nbytes less than blocksize not allowed");
        return INVALID_SIZE;
    }

	nBytes -= (nBytes % BLOCKSIZE);
	//fptr = fopen(filename, "w+"); // w+ if file new
	fptr = open(filename, O_RDWR | O_CREAT, 0644); //0644 file perm mode
	ftruncate(fptr, nBytes);
	if (fptr<0)
	{
		printf("can't create file");
		return FILE_FUNC_ERROR; // make error for fopen failure
	}

	
	return fptr;
}

int closeDisk(int disk)
{
	// do we have to check if disk is a valid param?
	if (close(disk) < 0)
	{
		return FILE_FUNC_ERROR;
	}

	return 0;
}



int readBlock(int disk, int bNum, void *block)
{
	off_t offset = bNum * BLOCKSIZE;
	if (lseek(disk, offset, SEEK_SET) == -1)
	{
		return READ_FUNC_FAILURE;
	}
	if (read(disk, block, BLOCKSIZE) != BLOCKSIZE)
	{
		return READ_FUNC_FAILURE;
	}
	return 0;
}

int writeBlock(int disk, int bNum, void *block)
{
	off_t offset = bNum * BLOCKSIZE;
	if (lseek(disk, offset, SEEK_SET) == -1)
	{
		return WRITE_FUNC_FAILURE;
	}
	if (write(disk, block, BLOCKSIZE) == -1)
	{
		return WRITE_FUNC_FAILURE;
	}
	return 0;
}
