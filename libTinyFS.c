#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "error.h"
#include "libDisk.h"
#include "libTinyFS.h"

int curr_mounted_disk = -1; //-1 means nothing mounted right now

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

	int numBlocks = nBytes / BLOCKSIZE;
	if (numBlocks < 2)
	{
		closeDisk(disk);
		return invalidNByteSize;
	}

	// initialize block
	char block[BLOCKSIZE];
	memset(block, 0x00, BLOCKSIZE);

	// superblock creation
	block[0] = BLOCK_TYPE_SUPERBLOCK;
	block[1] = VALID_BYTE;
	block[2] = 2;			   // pointer to first free block
	block[3] = ROOT_INODE_NUM; // root inode is at block 1

	if (writeBlock(disk, SUPERBLOCK_NUM, block) < 0)
	{
		closeDisk(disk);
		return writeBlockFuncFailure;
	}

	// write root inode
	memset(block, 0x00, BLOCKSIZE);

	block[0] = BLOCK_TYPE_INODE;
	block[1] = VALID_BYTE;
	block[2] = 0x00;
	block[3] = 0x00;
	// bytes 4-11: file name
	// bytes 12-13: file size
	// bytes 14+: pointers to file extents
	// all zeroes

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

	// verify
	char buffer[BLOCKSIZE];
	if (readBlock(disk, SUPERBLOCK_NUM, buffer) < 0)
	{
		closeDisk(disk);
		return readBlockFuncFailure;
	}

	// check magic number ??
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

fileDescriptor tfs_openFile(char *name);

int tfs_closeFile(fileDescriptor FD)
{
	closeDisk(FD);
}
int tfs_writeFile(fileDescriptor FD, char *buffer, int size);

int tfs_deleteFile(fileDescriptor FD);

int tfs_readByte(fileDescriptor FD, char *buffer);

int tfs_seek(fileDescriptor FD, int offset)
{
	if (lseek(disk, offset, SEEK_SET) == -1)
	{
		return -1; // add in correct code
	}
	return 0;
}
