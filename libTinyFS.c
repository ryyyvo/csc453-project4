#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "error.h"
#include "libDisk.h"
#include "libTinyFS.h"
#include <string.h>
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

	int numBlocks = (nBytes - (nBytes%BLOCKSIZE)) / BLOCKSIZE;
	if (numBlocks < 3) //need atleast one free block to store something, or it's a useless file
	{
		closeDisk(disk);
		return invalidNByteSize;
	}

	// initialize block #1
	char block[BLOCKSIZE];
	memset(block, 0x00, BLOCKSIZE);

	// superblock creation
	block[0] = BLOCK_TYPE_SUPERBLOCK;
	block[1] = VALID_BYTE;

	//byte 3 must be EMPTY so shifting things  a lil
	//block[2] = 2;			   // pointer to first free block
	//block[3] = ROOT_INODE_NUM; // root inode is at block 1
	

	block[2] = 2; //store pointer to free block
	block[3] = 0x00; //byte3 must be empty for all blocks
	block[4] = ROOT_INODE_NUM; //root inode
	if (writeBlock(disk, SUPERBLOCK_NUM, block) < 0)
	{
		closeDisk(disk);
		return writeBlockFuncFailure;
	}

	// write root inode
	memset(block, 0x00, BLOCKSIZE);

	block[0] = BLOCK_TYPE_INODE;
	block[1] = VALID_BYTE;

	if (writeBlock(disk, ROOT_INODE_NUM, block) < 0)
	{
		closeDisk(disk);
		return writeBlockFuncFailure;
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
		else
		{
			block[2] = 0x00; // if no next free block
		}

		block[3] = 0x00;
		if (writeBlock(disk, i, block) < 0)
		{
			closeDisk(disk);
			return writeBlockFuncFailure;
		}
	}

	closeDisk(disk);
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
		return alreadyMounted;
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
		return readBlockFuncFailure;
	}

	// check magic number ?? should be good enough
	if (buffer[1] != VALID_BYTE)
	{
		closeDisk(disk);
		return readBlockFuncFailure;
	}

	// check block type (byte 0 should be superblock type ?) ??
	if (buffer[0] != BLOCK_TYPE_SUPERBLOCK)
	{
		closeDisk(disk);
		return readBlockFuncFailure;
	}

	curr_mounted_disk = disk;
	return 0;
}

int tfs_unmount(void)
{
	if (curr_mounted_disk == -1)
	{
		return -1; // no disk currently mounted (add error code ?)
	}

	if (closeDisk(curr_mounted_disk) < 0)
	{
		return fileCloseFailure;
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
		return -1;
	}

	int nameLen = strlen(name);
	if (nameLen == 0 || nameLen > 8)
	{
		return -1;
	}

	// search for existing file in inode chain
	char block[BLOCKSIZE];
	int currentInodeBlock = ROOT_INODE_NUM;
	int foundInodeBlock = -1;

	if (readBlock(curr_mounted_disk, ROOT_INODE_NUM, block) < 0)
	{
		return readBlockFuncFailure;
	}

	// traverse inode linked list
	while (currentInodeBlock != 0)
	{
		if (readBlock(curr_mounted_disk, currentInodeBlock, block) < 0)
		{
			return readBlockFuncFailure;
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
			return readBlockFuncFailure;
		}

		int freeBlock = (unsigned char)block[2];
		if (freeBlock == 0)
		{
			return -1;
		}

		// read free block to get next free block
		char freeBlockData[BLOCKSIZE];
		if (readBlock(curr_mounted_disk, freeBlock, freeBlockData) < 0)
		{
			return readBlockFuncFailure;
		}
		int nextFreeBlock = (unsigned char)freeBlockData[2];

		// update superblock to point to next free block
		block[2] = nextFreeBlock;
		if (writeBlock(curr_mounted_disk, SUPERBLOCK_NUM, block) < 0)
		{
			return writeBlockFuncFailure;
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

		// write new inode to disk
		if (writeBlock(curr_mounted_disk, freeBlock, freeBlockData) < 0)
		{
			return writeBlockFuncFailure;
		}

		// link new inode to root inode's chain
		if (readBlock(curr_mounted_disk, ROOT_INODE_NUM, block) < 0)
		{
			return readBlockFuncFailure;
		}
		block[2] = freeBlock; // root inode now points to new inode
		if (writeBlock(curr_mounted_disk, ROOT_INODE_NUM, block) < 0)
		{
			return writeBlockFuncFailure;
		}

		foundInodeBlock = freeBlock;
	}

	FileTableEntry *newEntry = (FileTableEntry *)malloc(sizeof(FileTableEntry));
	if (!newEntry)
	{
		return -1;
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
{
	closeDisk(FD);
};

int tfs_writeFile(fileDescriptor FD, char *buffer, int size){
	FileTableEntry *current = openFileTable;
    
	int numBlocks = size/(BLOCKSIZE-4);
	while(current && current->fd != FD){
        current = current->next;
    }
    if(!current){
        return -1; //error
    }
	char block[BLOCKSIZE];	

	//char freeBlock = current->inodeBlockNum;
	int nextFreeBlock;
	char *buffer_ptr = buffer;
	memset(block, 0x00, BLOCKSIZE);
	for(int i = 2; i<numBlocks; i++){
		block[0] = BLOCK_TYPE_FILE_EXTENT;
		block[1] = VALID_BYTE;
		nextFreeBlock = i+1;
		block[2] = nextFreeBlock;
		block[3] = 0x00;
		memcpy(blk + 4, buffer_ptr, 252);
		buffer_ptr += 252;
		writeBlock(curr_mounted_disk, i, block);		
	}


	char inode[BLOCKSIZE];
    if (readBlock(curr_mounted_disk, current->inodeBlockNum, inode) < 0) {
        return readBlockFuncFailure;
    }
	inode[2] = nextFreeBlock;
	inode[12] = numBlocks * BLOCKSIZE;
	
	//have to update nextFreeBlock in inode byte 2 
	//have to chnage block[12-13] for size 

	if (writeBlock(curr_mounted_disk, current->inodeBlockNum, inode) < 0){     
		return -1; //error
	}
	current->currentOffset = 0;



}
int tfs_deleteFile(fileDescriptor FD);

int tfs_readByte(fileDescriptor FD, char *buffer){
	FileTableEntry *current = openFileTable;
	while(current && current->fd != FD){
		current = current->next;		
	}
	if(!current){
		return -1; //error
	}

	tfs_seek(FD, current->currentOffset);
	if(read(FD, buffer, 1) == -1){
		return -1; //failure
	}
	current->currentOffset += 1;
	return 0;
}
int tfs_seek(fileDescriptor FD, int offset)
{
	//FD or curr_mounted_disk??
	if (lseek(FD, offset, SEEK_SET) == -1)
	{
		return -1; // add in correct code
	}
	return 0;
}
