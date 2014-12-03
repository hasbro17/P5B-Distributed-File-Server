#include <stdio.h>
#include "udp.h"
#include "mfs.h"
#include <math.h>


int sd;//socket descriptor
int fdImage;//file descriptor for disk image
char imgName[1024];//name of file system image
struct sockaddr_in addr;



void initImage(char *imgName);
void handleRequest(char *buffer);

//server handler functions
int sLookup(int pinum, char *name);
int sStat(int inum, MFS_Stat_t *m);
int sWrite(int inum, char *buffer, int block);
int sRead(int inum, char *buffer, int block);
int sCreat(int pinum, int type, char *name);
int sUnlink(int pinum, char *name);
int shutDown();


int main(int argc, char *argv[])
{
	if(argc!=3)
	{
		fprintf(stderr,"Usage: server [portnum] [file-system-image]\n");
		exit(0);
	}

	int portNum=atoi(argv[1]);
	sprintf(imgName,argv[2]);

	//Open the port
	sd=UDP_Open(portNum);
	assert(sd != -1);

	initImage(imgName);
	
	char buffer[MFS_BLOCK_SIZE];
	//Server listening in loop
	while(1)
	{
		//read incoming message from port
		int rc=UDP_Read(sd, &addr, buffer, sizeof(msg_t));
		if(rc>0)
		{
			handleRequest(buffer);
			rc = UDP_Write(sd, &addr, buffer, sizeof(msg_t));//Send back the response to the client
		}
	}

	return 0;
}


void setbit(int n, char *byte)
{
	*byte |= (1 << n);
}

void clearbit(int n, char *byte)
{
	*byte &= ~(1 << n);
}

int checkbit(int n, char byte)
{
	return ( (byte >> n) & 1 );
}

int allocDataBlock()
{
	char byte;
	int i,j;
	int freeFound=0;//flag to check if free block found
	int freeBlock;
	//Find free block
	for(i=0; i<MAXDATABLOCKS; i=i+8)
	{
		lseek(fdImage,BYOFF_BIT(i),SEEK_SET);//get the byte for that block in the bitmap
		read(fdImage,&byte,sizeof(char));
		for(j=0; j<8; j++)
		{
			if(checkbit(j,byte)==0)//if unused bit found
			{
				freeBlock= i + j;
				freeFound=1;
				setbit(j,&byte);//set that bit to used
				//write back the byte
				lseek(fdImage,BYOFF_BIT(i),SEEK_SET);
				write(fdImage,&byte,sizeof(char));
				break;
			}
		}
		if(freeFound)
			break;
	}

	if(!freeFound)
		return -1;//No free block to write on, should not happen
	else
		return freeBlock;
}

//Find a free inode number
int getFreeInum()
{
	int i;
	int freeInode=-1;
	//Find a free inode number to write this new inode to
	lseek(fdImage, BYOFF_INODE(ROOTINO+1), SEEK_SET);//no need to read the root inode
	inode_t tempInode;
	for(i=1; i<NUMINODES; i++)
	{
		lseek(fdImage, BYOFF_INODE(i), SEEK_SET);
		read(fdImage, &tempInode, sizeof(inode_t));
		if(tempInode.type==MFS_UNUSED)
		{
			freeInode=i;
			break;
		}
	}
	return freeInode;
}

//Initialize the first directory data block with . and .. pathnames
//Needs the directory inum and its first data block dirDatNum
void dirDataInit(int inum, int pinum, int dirDatNum)
{
	int i;
	dir_t dirDat;
	for(i=0; i<NUM_DIR_ENTS; i++)
		dirDat.dirEnt[i].inum=-1;//all directory entries unused
	//Fill up the root data block with directory entries . and ..
	sprintf(dirDat.dirEnt[0].name,".");
	dirDat.dirEnt[0].inum=inum;
	sprintf(dirDat.dirEnt[1].name,"..");
	dirDat.dirEnt[1].inum=pinum;
	//Get to the data block dirDatNum
	lseek(fdImage, BYOFF_BLOCK(dirDatNum), SEEK_SET);
	//Write the block dirDat over there as the new directory block
	write(fdImage, &dirDat, sizeof(dir_t));

}



void initImage(char *imgName)
{
	int i=0;
	fdImage=open(imgName, O_RDWR);
	
	//Image does not exist
	if(fdImage<0)
	{
		fdImage=open(imgName, O_RDWR | O_CREAT, S_IRWXU);

		//create new super block
		superblock_t superblock;
		superblock.size=5;// 5: 1.unused, 2.superblock, 3.inode-block, 4.bitmap-block, 5.1st data block
		superblock.nblocks=1;//1 data block to hold root directory entries
		superblock.ninodes=1;//only root inode
		//Get to start of superblock
		lseek(fdImage, BYOFF_SUPER, SEEK_SET);
		//write the super block
		write(fdImage, &superblock, sizeof(superblock_t));


		//Create all the inodes and mark them as unused
		lseek(fdImage, BYOFF_INODE(0), SEEK_SET);
		inode_t inode;
		for(i=0; i<14; i++)
			inode.blockPtrs[i]=-1;//data block pointers unassigned
		inode.type=MFS_UNUSED;//unused inode
		inode.size=0;//No data blocks for the inode
		for (i=0; i<NUMINODES; i++)
		{
			lseek(fdImage, BYOFF_INODE(i), SEEK_SET);
			write(fdImage, &inode, sizeof(inode_t));
		}


		//Create the root inode
		inode.type=MFS_DIRECTORY;//type directory
		inode.blockPtrs[0]=BYOFF_BLOCK(0);//first pointer to data block 0	
		inode.size=MFS_BLOCK_SIZE;//FIXME Inode size is one data block for root directory entries?
		//Get to start of inode 0
		lseek(fdImage,BYOFF_INODE(ROOTINO),SEEK_SET);
		//Write the root inode
		write(fdImage, &inode, sizeof(inode_t));

		
		//Set the datablock bitmap to all zeros
		char zeroBuffer[MFS_BLOCK_SIZE];
		memset(zeroBuffer,0, sizeof(zeroBuffer));
		lseek(fdImage,BYOFF_BIT(0),SEEK_SET);
		write(fdImage, &zeroBuffer, sizeof(zeroBuffer));

/*
		//Set the bitmap for the bit of data block 0 just assigned
		char byte;
		lseek(fdImage,BYOFF_BIT(0),SEEK_SET);
		read(fdImage, &byte, sizeof(char));//read the byte
		setbit(BIOFF_BIT(0),&byte);//set the necessary bit
		lseek(fdImage,BYOFF_BIT(0),SEEK_SET);
		write(fdImage, &byte, sizeof(char));//write it back
		
*/
		int freeBlock=allocDataBlock();
		//First root directory data block
		dirDataInit(ROOTINO, ROOTINO, freeBlock);//initialize with . and ..	
		
		fsync(fdImage);
	}
	else
	{
		//Do nothing for now
	}
}

int sLookup(int pinum, char *name)
{
	int i,j;
	//Check if valid pinum
	if(pinum<0 || pinum>=NUMINODES)
		return -1;
		
	//Get the parent inode
	lseek(fdImage, BYOFF_INODE(pinum), SEEK_SET);
	inode_t pinode;
	read(fdImage, &pinode, sizeof(pinode));
	//should be a directory
	if(pinode.type!=MFS_DIRECTORY)
		return -1;

	dir_t dirBlock;
	for(i=0; i<14; i++)
	{
		if(pinode.blockPtrs[i]!=-1)//valid blockPtr
		{
			//Go to that block
			lseek(fdImage, pinode.blockPtrs[i], SEEK_SET);
			read(fdImage, &dirBlock, sizeof(dir_t));
			//Check all entries in the directory block
			for(j=0; j<NUM_DIR_ENTS; j++)
			{
				if(dirBlock.dirEnt[j].inum!=-1)//valid inode number
				{
					if( strcmp(dirBlock.dirEnt[j].name, name)==0 )//name match
						return dirBlock.dirEnt[j].inum;
				}
			}
		}
	}

	//Name does not exist
	return -1;
}

int sStat(int inum, MFS_Stat_t *m)
{
	//Invalid inode number
	if(inum < 0 || inum >=NUMINODES)
		return -1;
	
	inode_t inode;
	//Go to inode
	lseek(fdImage, BYOFF_INODE(inum), SEEK_SET);
	read(fdImage, &inode, sizeof(inode_t));

	//Not in use
	if(inode.type==MFS_UNUSED)
		return -1;
	
	m->size=inode.size;
	m->type=inode.type;
	return 0;
}



int sWrite(int inum, char *buffer, int block)
{
	//Invalid inode number
	if(inum < 0 || inum >=NUMINODES)
		return -1;
	if(block<0 || block>=14)
		return -1;

	inode_t inode;
	//Go to inode
	lseek(fdImage, BYOFF_INODE(inum), SEEK_SET);
	read(fdImage, &inode, sizeof(inode_t));
	//Should be a file
	if(inode.type!=MFS_REGULAR_FILE)
		return -1;
	
	if(inode.blockPtrs[block]==-1)//write on new block
	{
		//allocate a free data block
		int freeBlock = allocDataBlock();
		if(freeBlock<0)//No free blocks left
			return -1;
		//Update the inode
		inode.blockPtrs[block]=BYOFF_BLOCK(freeBlock);//point to free block
		inode.size+=MFS_BLOCK_SIZE;//increment the inode size

		//write to the inode
		lseek(fdImage, BYOFF_INODE(inum), SEEK_SET);
		write(fdImage, &inode, sizeof(inode_t));
		//move to the new block
		lseek(fdImage, BYOFF_BLOCK(freeBlock), SEEK_SET);
	}
	else //already present
	{
		//move to the already present block
		lseek(fdImage, inode.blockPtrs[block], SEEK_SET);
	}

	//write to the data block
	write(fdImage, buffer, MFS_BLOCK_SIZE);
	fsync(fdImage);
	return 0;
}
int sRead(int inum, char *buffer, int block)
{
	//Invalid inode number
	if(inum < 0 || inum >=NUMINODES)
		return -1;
	if(block<0 || block>=14)
		return -1;

	inode_t inode;
	//Go to inode
	lseek(fdImage, BYOFF_INODE(inum), SEEK_SET);
	read(fdImage, &inode, sizeof(inode_t));	
	//Should be either file or directory
	if(inode.type!=MFS_UNUSED)
		return -1;
	//Should point to a vaild block
	if(inode.blockPtrs[block]==-1)
		return -1;

	//Move to and read that block
	lseek(fdImage, inode.blockPtrs[block], SEEK_SET);
	read(fdImage, buffer, MFS_BLOCK_SIZE);
	return 0;
}

int sCreat(int pinum, int type, char *name)
{

	int i,j;
	int newBlockPtr=-1;
	int newDirEnt=-1;
	int newF=1; 
	//Check if valid pinum
	if(pinum<0 || pinum>=NUMINODES)
		return -1;
	//check name size
	if(strlen(name) < 1 || strlen(name) >= 60)
		return -1;

	//Get the parent inode
	lseek(fdImage, BYOFF_INODE(pinum), SEEK_SET);
	inode_t pinode;
	read(fdImage, &pinode, sizeof(pinode));
	//should be a directory
	if(pinode.type!=MFS_DIRECTORY)
		return -1;

	//scan all directory entries to check if name already exists, also save the first unassigned directory entry for later use
	dir_t dirBlock;
	for(i=0; i<14; i++)
	{
		if(pinode.blockPtrs[i]!=-1)//valid blockPtr
		{
			printf("Parent inode:%d points to blockPtr[%d]:%d\n",pinum,i,pinode.blockPtrs[i]);
			//Go to that block
			lseek(fdImage, pinode.blockPtrs[i], SEEK_SET);
			read(fdImage, &dirBlock, sizeof(dir_t));
			//Check all entries in the directory block
			for(j=0; j<NUM_DIR_ENTS; j++)
			{
				if(dirBlock.dirEnt[j].inum!=-1)//valid inode number
				{
					printf("Valid directory entry number:%d name:%s and inode num:%d\n",j,dirBlock.dirEnt[j].name,dirBlock.dirEnt[j].inum);
					if( strcmp(dirBlock.dirEnt[j].name, name)==0 )//name match
						return 0;//file or directory name already exists so success
				}
				else//unused dirent
				{
					if(newF)//save the first unused dir entry to be overwritten later
					{
						printf("Parent inode:%d points to blockPtr[%d]:%d\n",pinum,i,pinode.blockPtrs[i]);
						printf("Within parent inode the first unused dir ent is %d\n",j);
						newBlockPtr=i;
						newDirEnt=j;
						newF=0;
					}
				}
			}
		}
	}

	//TODO
	//Corner case where no unused dirEntry was found. Meaning we have to assign a whole new data block of directory entries and update the pinode size
	//This could fail if all the data blocks of this directory are filled with valid data entries. No room for more


	//Find a free inode number
	int freeInode= getFreeInum();	
	if(freeInode<0)//no free inode was found then no more space for new files
		return -1;

	//Create the new inode
	inode_t inode;
	inode.type=type;
	for(i=0; i<14; i++)
		inode.blockPtrs[i]=-1;
	inode.size=0;
	
	//if directory type then initialize it with . and ..
	if(type==MFS_DIRECTORY)
	{
		inode.size=MFS_BLOCK_SIZE;
		int d = allocDataBlock();
		if(d<0)//out of data blocks
			return -1;
		inode.blockPtrs[0]=BYOFF_BLOCK(d);
		dirDataInit(freeInode, pinum, d);		
		printf("Dir Inode:%d with data block:%d\n",freeInode,d);
	}
		
	//Get to the free inode
	lseek(fdImage, BYOFF_INODE(freeInode), SEEK_SET);
	//Write down the new inode there
	write(fdImage, &inode, sizeof(inode_t));

	//Write the name:inum directory entry
	//Go to the parent directory's data block
	lseek(fdImage, pinode.blockPtrs[newBlockPtr], SEEK_SET);
	read(fdImage, &dirBlock, sizeof(dir_t));
	//Edit the unused directory entry inside it
	dirBlock.dirEnt[newDirEnt].inum=freeInode;
	sprintf(dirBlock.dirEnt[newDirEnt].name, name);
	//Write down the updated directory entry data block	
	lseek(fdImage, pinode.blockPtrs[newBlockPtr], SEEK_SET);
	write(fdImage, &dirBlock, sizeof(dir_t));

	printf("New inode number:%d and name:%s and directory blockPtr:%d entry number:%d\n", freeInode, name, newBlockPtr, newDirEnt);
	return 0;
}



int sUnlink(int pinum, char *name)
{
	return 0;
}

int shutDown()
{
	return 0;
}

void handleRequest(char *buffer)
{
	msg_t *msg = (msg_t *)buffer;
	
	switch(msg->func){
		case LOOKUP:
			msg->retCode=sLookup(msg->pinum, msg->name);
			break;
		case STAT:
			msg->retCode=sStat(msg->inum, &(msg->stat));
			break;
		case WRITE:
			msg->retCode=sWrite(msg->inum, msg->buffer, msg->block);
			break;
		case READ:
			msg->retCode=sRead(msg->inum, msg->buffer, msg->block);
			break;
		case CREAT:
		{//need this bracket to define a scope for variable declaration
			//MFS_Stat_t *stat = &(msg->stat); 
			msg->retCode=sCreat(msg->pinum, msg->type, msg->name);	
			break;
		}
		case UNLINK:
			msg->retCode=sUnlink(msg->pinum, msg->name);
			break;
		case SHUTDOWN:
			msg->retCode=0;
			fsync(fdImage);
			close(fdImage);
			UDP_Write(sd,&addr,(char*)msg, sizeof(msg_t));
			exit(0);
			break;
		default:
			msg->retCode=-1;
			break;
	}

}

