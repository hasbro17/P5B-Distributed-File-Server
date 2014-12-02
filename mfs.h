#ifndef __MFS_h__
#define __MFS_h__

// On-disk file system format.
// Disk layout is: superblock, inodes, block in-use bitmap, data blocks.
// Block 1 is unused.
// Block 2 is super block.
// Inodes start at block 3.
// Block 4 is for data block bitmap

#define MFS_UNUSED       (0)
#define MFS_DIRECTORY    (2)
#define MFS_REGULAR_FILE (1)

#define MFS_BLOCK_SIZE   (4096)

#define ROOTINO 0 //root inode number

//File system super block
typedef struct __superblock_t {
	//Note originally uint in xv6
	int size;         // Size of file system image (blocks)
    int nblocks;      // Number of data blocks
	int ninodes;      // Number of inodes.
} superblock_t;

//inode metadata
typedef struct __MFS_Stat_t {
    int type;   // MFS_DIRECTORY or MFS_REGULAR
    int size;   // bytes
    // note: no permissions, access times, etc.
} MFS_Stat_t;

//directory entry format
typedef struct __MFS_DirEnt_t {
    char name[60];  // up to 60 bytes of name in directory (including \0)
    int  inum;      // inode number of entry (-1 means entry not used)
} MFS_DirEnt_t;

//inode structure
typedef struct __inode_t {
//	MFS_Stat_t stat; //inode metadata
	int type;
	int size;
	int blockPtrs[14];//direct block pointers
} inode_t;

#define NUM_DIR_ENTS ( MFS_BLOCK_SIZE / (sizeof(MFS_DirEnt_t)) )//number of directory entries in a data block

//directory data block 64 directory entries of 64bytes each
typedef struct __dir_t {
	MFS_DirEnt_t dirEnt[NUM_DIR_ENTS];
} dir_t;

//To tell what type of function is being used
typedef enum __func_t{
	LOOKUP,
	STAT,
	WRITE,
	READ,
	CREAT,
	UNLINK,
	SHUTDOWN
} func_t;

//Message to send over UDP
typedef struct __msg_t {
	int pinum;//parent inode number
	int inum;
	int type;
	int block;// block number
	int retCode;//return code from server 0:sucess -1:failure
	char name[60];//path name
	char buffer[MFS_BLOCK_SIZE];//buffer to read/write a whole block
	func_t func;//function type being called
	MFS_Stat_t stat;
} msg_t;


// Byte offset for supernode block
#define BYOFF_SUPER MFS_BLOCK_SIZE//FIXME redundant

// Byte offset for inode i : inodes numbered from 0-63
#define BYOFF_INODE(i) ( sizeof(inode_t)*i + 2*MFS_BLOCK_SIZE )

//Byte offset for data block d : blocks numbered from 0-1023
#define BYOFF_BLOCK(b) ( b*MFS_BLOCK_SIZE + 4*MFS_BLOCK_SIZE )

//Byte offset for the bit corresponding to block b
#define BYOFF_BIT(b) ( b/8 + 3*MFS_BLOCK_SIZE)

//Bit offset within the byte in bitmap for block b
#define BIOFF_BIT(b) ( b%8 )


int MFS_Init(char *hostname, int port);
int MFS_Lookup(int pinum, char *name);
int MFS_Stat(int inum, MFS_Stat_t *m);
int MFS_Write(int inum, char *buffer, int block);
int MFS_Read(int inum, char *buffer, int block);
int MFS_Creat(int pinum, int type, char *name);
int MFS_Unlink(int pinum, char *name);
int MFS_Shutdown();

#endif // __MFS_h__
