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