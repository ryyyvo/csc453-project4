#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "error.h"
#include "libDisk.h"
#include "libTinyFS.h"
int curr_mounted_disk = -1; //-1 means nothing mounted
FileTableEntry *openFileTable = NULL;
int nextFD = 0;

int tfs_mkfs(char *filename, int nBytes)
/* Makes a blank TinyFS file system of size nBytes on the unix file
specified by ‘filename’. This function should use the emulated disk
library to open the specified unix file, and upon success, format the
file to be a mountable disk. This includes initializing all data to 0x00,
setting magic numbers, initializing and writing the superblock and
inodes, etc. Must return a specified success/error code. */
{
	int disk = openDisk(filename, nBytes);
	if (disk < 0)
	{
		return disk; // return error code from openDisk
	}

	int numBlocks = (nBytes - (nBytes % BLOCKSIZE)) / BLOCKSIZE;
	if (numBlocks < 3) // need atleast one free block to store something, or it's a useless file
	{
		closeDisk(disk);
		return INVALID_SIZE;
	}

	char block[BLOCKSIZE];
	memset(block, 0x00, BLOCKSIZE);

	// superblock creation
	block[0] = BLOCK_TYPE_SUPERBLOCK;
	block[1] = VALID_BYTE;

	// byte 3 must be EMPTY so shifting things  a lil
	// block[2] = 2;			   // pointer to first free block
	// block[3] = ROOT_INODE_NUM; // root inode is at block 1

	block[2] = 2;			   // store pointer to free block
	block[3] = 0x00;		   // byte3 must be empty for all blocks
	block[4] = ROOT_INODE_NUM; // root inode
	if (writeBlock(disk, SUPERBLOCK_NUM, block) < 0)
	{
		closeDisk(disk);
		return WRITE_FUNC_FAILURE;
	}

	// write root inode
	memset(block, 0x00, BLOCKSIZE);

	block[0] = BLOCK_TYPE_INODE;
	block[1] = VALID_BYTE;

	if (writeBlock(disk, ROOT_INODE_NUM, block) < 0)
	{
		closeDisk(disk);
		return WRITE_FUNC_FAILURE;
	}

	// initialize remaining blocks as free blocks
	for (int i = 2; i < numBlocks; i++)
	{
		memset(block, 0x00, BLOCKSIZE);

		block[0] = BLOCK_TYPE_FREE;
		block[1] = VALID_BYTE;

		// link to next free block, or 0 if this is the last block
		if (i < numBlocks - 1)
		{
			block[2] = i + 1; // points to next free block
		}

		block[3] = 0x00;
		if (writeBlock(disk, i, block) < 0)
		{
			closeDisk(disk);
			return WRITE_FUNC_FAILURE;
		}
	}

	closeDisk(disk);
	//printf("end of print func");
	return 0;
}

int tfs_mount(char *diskname)
{
	/* tfs_mount(char *diskname) “mounts” a TinyFS file system located within
	‘diskname’. tfs_unmount(void) “unmounts” the currently mounted file
	system. As part of the mount operation, tfs_mount should verify the file
	system is the correct type. In tinyFS, only one file system may be
	mounted at a time. Use tfs_unmount to cleanly unmount the currently
	mounted file system. Must return a specified success/error code. */
	if (curr_mounted_disk != -1)
	{
		return ALRDY_MOUNTED;
	}

	int disk = openDisk(diskname, 0);
	if (disk < 0)
	{
		return disk; // return error code from openDisk
	}

	// make sure only one disk can be mounted

	// verify
	char buffer[BLOCKSIZE];
	if (readBlock(disk, SUPERBLOCK_NUM, buffer) < 0)
	{
		closeDisk(disk);
		return READ_FUNC_FAILURE;
	}

	// check magic number ?? should be good enough
	if (buffer[1] != VALID_BYTE)
	{
		closeDisk(disk);
		return READ_FUNC_FAILURE;
	}

	// check block type (byte 0 should be superblock type ?) ??
	if (buffer[0] != BLOCK_TYPE_SUPERBLOCK)
	{
		closeDisk(disk);
		return READ_FUNC_FAILURE;
	}

	curr_mounted_disk = disk;
	return 0;
}

int tfs_unmount(void)
{
	if (curr_mounted_disk == -1)
	{
		return INVALID_ACCESS; // no disk currently mounted (add error code ?)
	}

	if (closeDisk(curr_mounted_disk) < 0)
	{
		return FILE_FUNC_ERROR;
	}

	curr_mounted_disk = -1;
	return 0;
}

fileDescriptor tfs_openFile(char *name)
/* Creates or Opens a file for reading and writing on the currently
mounted file system. Creates a dynamic resource table entry for the file,
and returns a file descriptor (integer) that can be used to reference
this entry while the filesystem is mounted. */
{
	if (curr_mounted_disk == -1)
	{
		return NTHG_MOUNTED;
	}

	int nameLen = strlen(name);
	if (nameLen == 0 || nameLen > 8)
	{
		return INVALID_ACCESS;
	}

	// search for existing file in inode chain
	char block[BLOCKSIZE];
	int currentInodeBlock = ROOT_INODE_NUM;
	int foundInodeBlock = -1;

	if (readBlock(curr_mounted_disk, ROOT_INODE_NUM, block) < 0)
	{
		return READ_FUNC_FAILURE;
	}

	// traverse inode linked list
	while (currentInodeBlock != 0)
	{
		if (readBlock(curr_mounted_disk, currentInodeBlock, block) < 0)
		{
			return READ_FUNC_FAILURE;
		}

		// check if inode has a filename (bytes 4-11)
		int hasFilename = 0;
		for (int i = 4; i < 12; i++)
		{
			if (block[i] != 0x00)
			{
				hasFilename = 1;
				break;
			}
		}

		// compare filenames
		if (hasFilename)
		{
			int match = 1;
			for (int i = 0; i < nameLen; i++)
			{
				if (block[4 + i] != name[i])
				{
					match = 0;
					break;
				}
			}
			// check bytes are 0x00
			for (int i = nameLen; i < 8 && match; i++)
			{
				if (block[4 + i] != 0x00)
				{
					match = 0;
					break;
				}
			}

			if (match)
			{
				foundInodeBlock = currentInodeBlock;
				break;
			}
		}

		// move to next inode in chain (byte 2)
		currentInodeBlock = (unsigned char)block[2];
	}

	// create new inode
	if (foundInodeBlock == -1)
	{
		// get first free block
		if (readBlock(curr_mounted_disk, SUPERBLOCK_NUM, block) < 0)
		{
			return READ_FUNC_FAILURE;
		}

		int freeBlock = (unsigned char)block[2];
		if (freeBlock == 0)
		{
			return INVALID_ACCESS;
		}

		// read free block to get next free block
		char freeBlockData[BLOCKSIZE];
		if (readBlock(curr_mounted_disk, freeBlock, freeBlockData) < 0)
		{
			return READ_FUNC_FAILURE;
		}
		int nextFreeBlock = (unsigned char)freeBlockData[2];

		// update superblock to point to next free block
		block[2] = nextFreeBlock;
		if (writeBlock(curr_mounted_disk, SUPERBLOCK_NUM, block) < 0)
		{
			return WRITE_FUNC_FAILURE;
		}

		// initialize new inode block
		memset(freeBlockData, 0x00, BLOCKSIZE);
		freeBlockData[0] = BLOCK_TYPE_INODE;
		freeBlockData[1] = VALID_BYTE;

		// copy filename (bytes 4-11)
		for (int i = 0; i < nameLen; i++)
		{
			freeBlockData[4 + i] = name[i];
		}

	
		/*
		// write new inode to disk
		if (writeBlock(curr_mounted_disk, freeBlock, freeBlockData) < 0)
		{
			return writeBlockFuncFailure;
		}
		*/

		char rootBlock[BLOCKSIZE];
		// link new inode to root inode's chain
		if (readBlock(curr_mounted_disk, ROOT_INODE_NUM, rootBlock) < 0)
		{
			return READ_FUNC_FAILURE;
		}
		freeBlockData[2] = rootBlock[2];

		 // write new inode to disk
		 //         if (writeBlock(curr_mounted_disk, freeBlock, freeBlockData) < 0)
		 //                 {
		 //                             return writeBlockFuncFailure;
		 //              
		if(writeBlock(curr_mounted_disk, freeBlock, freeBlockData) < 0){
			return WRITE_FUNC_FAILURE;
		}



		//block[2] = freeBlock; // root inode now points to new inode
		rootBlock[2] = freeBlock;
		if (writeBlock(curr_mounted_disk, ROOT_INODE_NUM,rootBlock) < 0)
		{
			return WRITE_FUNC_FAILURE;
		}

		foundInodeBlock = freeBlock;
	}

	FileTableEntry *newEntry = (FileTableEntry *)malloc(sizeof(FileTableEntry));
	if (!newEntry)
	{
		return INVALID_ACCESS;
	}

	newEntry->fd = nextFD++;
	strncpy(newEntry->filename, name, 8);
	newEntry->filename[8] = '\0';
	newEntry->inodeBlockNum = foundInodeBlock;
	newEntry->currentOffset = 0;
	newEntry->next = openFileTable;
	openFileTable = newEntry;

	return newEntry->fd;
}

int tfs_closeFile(fileDescriptor FD)
/* Closes the file, de-allocates all system resources, and removes table
entry */
{
	if (FD < 0)
	{
		return INVALID_FD;
	}

	// search for the file descriptor in open file table
	FileTableEntry *current = openFileTable;
	FileTableEntry *prev = NULL;

	while (current != NULL)
	{
		if (current->fd == FD)
		{
			if (prev == NULL)
			{
				// removing head of list
				openFileTable = current->next;
			}
			else
			{
				// remove middle/end
				prev->next = current->next;
			}

			free(current);
			return 0;
		}

		prev = current;
		current = current->next;
	}
	return INVALID_FD;
}

int tfs_writeFile(fileDescriptor FD, char *buffer, int size)
{
	FileTableEntry *current = openFileTable;
	while (current && current->fd != FD)
	{
		current = current->next;
	}
	if (!current)
	{
		return INVALID_ACCESS;
	}

	char inodeBlock[BLOCKSIZE];
	if (readBlock(curr_mounted_disk, current->inodeBlockNum, inodeBlock) < 0)
	{
		return READ_FUNC_FAILURE;
	}

	// get number of extent blocks needed
	int numBlocks = (size + 251) / 252;
	if (numBlocks == 0 && size > 0)
	{
		numBlocks = 1;
	}

	int firstExtentBlock = 0;
	int prevExtentBlock = 0;
	int remainingSize = size;
	int bufferOffset = 0;

	// write extent blocks
	for (int i = 0; i < numBlocks; i++)
	{
		// allocate block from free list
		char superblock[BLOCKSIZE];
		if (readBlock(curr_mounted_disk, SUPERBLOCK_NUM, superblock) < 0)
		{
			return READ_FUNC_FAILURE;
		}

		int freeBlock = (unsigned char)superblock[2];
		if (freeBlock == 0)
		{
			return INVALID_ACCESS;
		}

		// get next free block in chain
		char freeBlockData[BLOCKSIZE];
		if (readBlock(curr_mounted_disk, freeBlock, freeBlockData) < 0)
		{
			return READ_FUNC_FAILURE;
		}
		int nextFreeBlock = (unsigned char)freeBlockData[2];

		// update superblock to point to next free block
		superblock[2] = nextFreeBlock;
		if (writeBlock(curr_mounted_disk, SUPERBLOCK_NUM, superblock) < 0)
		{
			return WRITE_FUNC_FAILURE;
		}

		// initialize extent block
		memset(freeBlockData, 0x00, BLOCKSIZE);
		freeBlockData[0] = BLOCK_TYPE_FILE_EXTENT;
		freeBlockData[1] = VALID_BYTE;

		// calculate bytes to write in this extent
		int bytesToWrite = (remainingSize > 252) ? 252 : remainingSize;

		// copy data from buffer to extent block (bytes 4-255)
		for (int j = 0; j < bytesToWrite; j++)
		{
			freeBlockData[4 + j] = buffer[bufferOffset + j];
		}

		// write extent block
		if (writeBlock(curr_mounted_disk, freeBlock, freeBlockData) < 0)
		{
			return WRITE_FUNC_FAILURE;
		}

		// update previous extent block to point to this one
		if (i > 0)
		{
			char prevExtentData[BLOCKSIZE];
			if (readBlock(curr_mounted_disk, prevExtentBlock, prevExtentData) < 0)
			{
				return READ_FUNC_FAILURE;
			}
			prevExtentData[2] = freeBlock; // link prev to curr
			if (writeBlock(curr_mounted_disk, prevExtentBlock, prevExtentData) < 0)
			{
				return WRITE_FUNC_FAILURE;
			}
		}

		// track first extent block
		if (i == 0)
		{
			firstExtentBlock = freeBlock;
		}

		prevExtentBlock = freeBlock;
		bufferOffset += bytesToWrite;
		remainingSize -= bytesToWrite;
	}

	// update inode with first extent pointer and file size
	inodeBlock[14] = firstExtentBlock;

	inodeBlock[12] = (size >> 8) & 0xFF;
	inodeBlock[13] = size & 0xFF;

	if (writeBlock(curr_mounted_disk, current->inodeBlockNum, inodeBlock) < 0)
	{
		return WRITE_FUNC_FAILURE;
	}

	current->currentOffset = 0;

	return 0;
}




int tfs_deleteFile(fileDescriptor FD)
/* deletes a file and marks its blocks as free on disk. */
{
	// find file in open file table
	FileTableEntry *current = openFileTable;
	FileTableEntry *prev = NULL;

	while (current && current->fd != FD)
	{
		prev = current;
		current = current->next;
	}

	if (!current)
	{
		return INVALID_FD;
	}

	char inodeBlock[BLOCKSIZE];
	if (readBlock(curr_mounted_disk, current->inodeBlockNum, inodeBlock) < 0)
	{
		return READ_FUNC_FAILURE;
	}

	if (current->inodeBlockNum == ROOT_INODE_NUM)
	{
		return INVALID_ACCESS; // prevent delete root inode
	}

	// get first extent block pointer and next inode pointer
	int firstExtent = (unsigned char)inodeBlock[14];
	int nextInode = (unsigned char)inodeBlock[2];

	// get previous inode
	char block[BLOCKSIZE];
	int prevInodeBlock = ROOT_INODE_NUM;

	if (readBlock(curr_mounted_disk, ROOT_INODE_NUM, block) < 0)
	{
		return READ_FUNC_FAILURE;
	}

	// traverse to inode that points to the one being deleted
	int currentInodeBlock = ROOT_INODE_NUM;
	while (currentInodeBlock != 0)
	{
		if (readBlock(curr_mounted_disk, currentInodeBlock, block) < 0)
		{
			return READ_FUNC_FAILURE;
		}

		int nextInodeInChain = (unsigned char)block[2];
		if (nextInodeInChain == current->inodeBlockNum)
		{
			prevInodeBlock = currentInodeBlock;
			break;
		}

		currentInodeBlock = nextInodeInChain;
	}

	// update previous inode to skip over deleted inode
	if (readBlock(curr_mounted_disk, prevInodeBlock, block) < 0)
	{
		return READ_FUNC_FAILURE;
	}
	block[2] = nextInode;
	if (writeBlock(curr_mounted_disk, prevInodeBlock, block) < 0)
	{
		return WRITE_FUNC_FAILURE;
	}

	// return extent blocks to free list
	int currentExtent = firstExtent;
	while (currentExtent != 0)
	{
		char extentBlock[BLOCKSIZE];
		if (readBlock(curr_mounted_disk, currentExtent, extentBlock) < 0)
		{
			return READ_FUNC_FAILURE;
		}

		int nextExtent = (unsigned char)extentBlock[2];

		// get current free list head
		char superblock[BLOCKSIZE];
		if (readBlock(curr_mounted_disk, SUPERBLOCK_NUM, superblock) < 0)
		{
			return READ_FUNC_FAILURE;
		}

		int currentFreeHead = (unsigned char)superblock[2];

		// mark extent as free and prepend to free list
		memset(extentBlock, 0x00, BLOCKSIZE);
		extentBlock[0] = BLOCK_TYPE_FREE;
		extentBlock[1] = VALID_BYTE;
		extentBlock[2] = currentFreeHead;

		if (writeBlock(curr_mounted_disk, currentExtent, extentBlock) < 0)
		{
			return WRITE_FUNC_FAILURE;		}

		// update superblock to point to this block
		superblock[2] = currentExtent;
		if (writeBlock(curr_mounted_disk, SUPERBLOCK_NUM, superblock) < 0)
		{
			return WRITE_FUNC_FAILURE;
		}

		currentExtent = nextExtent;
	}

	// return inode block to free list
	char superblock[BLOCKSIZE];
	if (readBlock(curr_mounted_disk, SUPERBLOCK_NUM, superblock) < 0)
	{
		return READ_FUNC_FAILURE;;
	}

	int currentFreeHead = (unsigned char)superblock[2];

	// mark inode as free and prepend to free list
	memset(inodeBlock, 0x00, BLOCKSIZE);
	inodeBlock[0] = BLOCK_TYPE_FREE;
	inodeBlock[1] = VALID_BYTE;
	inodeBlock[2] = currentFreeHead;

	if (writeBlock(curr_mounted_disk, current->inodeBlockNum, inodeBlock) < 0)
	{
		return WRITE_FUNC_FAILURE;
	}

	superblock[2] = current->inodeBlockNum;
	if (writeBlock(curr_mounted_disk, SUPERBLOCK_NUM, superblock) < 0)
	{
		return WRITE_FUNC_FAILURE;	
	}

	// remove from open file table
	if (prev == NULL)
	{
		openFileTable = current->next;
	}
	else
	{
		prev->next = current->next;
	}

	free(current);

	return 0;
}

int tfs_readByte(fileDescriptor FD, char *buffer){
	FileTableEntry *current = openFileTable;
    while (current && current->fd != FD)
    {
        current = current->next;
    }
    if (!current)
    {
        return READ_FUNC_FAILURE;
   }

	char inode[BLOCKSIZE];
	if (readBlock(curr_mounted_disk, current->inodeBlockNum, inode) < 0){
	    return READ_FUNC_FAILURE;
	}


	//int size = inode[12]; //IS THIS CORRECT?
	int size = ((unsigned char)inode[12] << 8) | (unsigned char)inode[13];
	int offset = current->currentOffset;
	if(offset >= size){
		return INVALID_ACCESS; //reached end of file
	}

	/*int offset = current->currentOffset;*/
	int blockToGoTo = offset/252;
	int inBlock = offset%252;
	char block[BLOCKSIZE];
	int extent = inode[14];
	for(int i = 0; i<blockToGoTo; i++){
		if(readBlock(curr_mounted_disk,extent, block)<0){
			return READ_FUNC_FAILURE;
		}
		extent = (unsigned char)block[2]; 
		//check if ended before 
	}
	
	if(readBlock(curr_mounted_disk, extent, block) < 0){
        return READ_FUNC_FAILURE;
	}

    *buffer = block[4 + inBlock];
	current->currentOffset++;
	return 0;	
}
int tfs_seek(fileDescriptor FD, int offset)
{
	FileTableEntry *current = openFileTable;
    while (current && current->fd != FD)
    {
        current = current->next;
    }
    if (!current)
    {
        return INVALID_ACCESS; // error
    }
	char inode[BLOCKSIZE];
    if (readBlock(curr_mounted_disk, current->inodeBlockNum, inode) < 0){
        return READ_FUNC_FAILURE;
	}


	int size = ((unsigned char)inode[12] << 8) | (unsigned char)inode[13];
	current->currentOffset = offset;
	return 0;
}
