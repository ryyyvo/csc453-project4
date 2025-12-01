#define BLOCKSIZE 256
#define DEFAULT_DISK_SIZE 10240
#define DEFAULT_DISK_NAME "tinyFSDisk"
#define VALID_BYTE 0x44

// block types
#define BLOCK_TYPE_SUPERBLOCK 1
#define BLOCK_TYPE_INODE 2
#define BLOCK_TYPE_FILE_EXTENT 3
#define BLOCK_TYPE_FREE 4

// block numbers
#define SUPERBLOCK_NUM 0
#define ROOT_INODE_NUM 1

typedef int fileDescriptor;

// function prototypes
int tfs_mkfs(char *filename, int nBytes);

int tfs_mount(char *diskname);

int tfs_unmount(void);

fileDescriptor tfs_openFile(char *name);

int tfs_closeFile(fileDescriptor FD);

int tfs_writeFile(fileDescriptor FD, char *buffer, int size);

int tfs_deleteFile(fileDescriptor FD);

int tfs_readByte(fileDescriptor FD, char *buffer);

int tfs_seek(fileDescriptor FD, int offset);