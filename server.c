#include <stdio.h>
#include "udp.h"
#include "mfs.h"

#define BUFFSIZE 1024

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
		int rc=UDP_Read(sd, &addr, buffer, BUFFSIZE);
		if(rc>0)
		{
			handleRequest(buffer);
			rc = UDP_Write(sd, &addr, buffer, BUFFSIZE);//Send back the response to the client
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

int checkbit(int n, char *byte)
{
	return ( (*byte >> n) & 1 );
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


		//Create the root inode
		inode_t inode;
		inode.type=MFS_DIRECTORY;//type directory
		for(i=0; i<14; i++)
			inode.blockPtrs[i]=-1;//data block pointers unassigned
		inode.blockPtrs[0]=BYOFF_BLOCK(0);//first pointer to data block 0	
		inode.size=MFS_BLOCK_SIZE;//FIXME Inode size is one data block for root directory entries?
		//Get to start of inode 0
		lseek(fdImage,BYOFF_INODE(ROOTINO),SEEK_SET);
		//Write the root inode
		write(fdImage, &inode, sizeof(inode_t));


		//Set the bitmap for the bit of data block 0 just assigned
		char byte;
		lseek(fdImage,BYOFF_BIT(0),SEEK_SET);
		read(fdImage, &byte, sizeof(char));//read the byte
		setbit(BIOFF_BIT(0),&byte);//set the necessary bit
		lseek(fdImage,BYOFF_BIT(0),SEEK_SET);
		write(fdImage, &byte, sizeof(char));//write it back
		
		

		//First root directory data block
		dir_t rootDir;
		for(i=0; i<NUM_DIR_ENTS; i++)
			rootDir.dirEnt[i].inum=-1;//all directory entries unused
		//Fill up the root data block with directory entries . and ..
		sprintf(rootDir.dirEnt[0].name,".");
		rootDir.dirEnt[0].inum=ROOTINO;
		sprintf(rootDir.dirEnt[1].name,"..");
		rootDir.dirEnt[1].inum=ROOTINO;
		//Get to start of data block 0
		lseek(fdImage, BYOFF_BLOCK(0), SEEK_SET);
		//Write the root directory block
		write(fdImage, &rootDir, sizeof(dir_t));
	}
	else
	{
		//Do nothing for now
	}
}

int sLookup(int pinum, char *name)
{
	return 0;
}

int sStat(int inum, MFS_Stat_t *m)
{
	return 0;
}

int sWrite(int inum, char *buffer, int block)
{
	return 0;
}
int sRead(int inum, char *buffer, int block)
{
	return 0;
}

int sCreat(int pinum, int type, char *name)
{
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

