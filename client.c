#include <stdio.h>
#include "udp.h"
#include "mfs.h"


#define BUFFER_SIZE (4096)
char buffer[BUFFER_SIZE];

int main(int argc, char *argv[])
{
	if(argc != 2)
	{
		fprintf(stderr,"Usage: client [Server-portnum]\n");
		exit(0);
	}

	int portNum=atoi(argv[1]);

	//Always running on local so for now get local hostname
	char hostname[1024];
	gethostname((char*)hostname,1024);

	//connect to server
	int rc=MFS_Init(hostname,portNum);	
	assert(rc==0);
	//What to do here to test out the server?

    rc = MFS_Creat(0, MFS_DIRECTORY, "dir2\0");
	printf("Create dir returned: %d\n",rc);
   
   	int inum = MFS_Lookup(0, "dir2\0");
	assert(rc!=-1);

	MFS_Creat(inum, MFS_DIRECTORY, "dir3\0");
	printf("Create dir3 in inum:%d returned: %d\n",inum, rc);

	inum=MFS_Lookup(1,"dir3\0");
	printf("Lookup dir3 in pinum 1 returned:%d\n",inum);
    
	rc = MFS_Creat(2, MFS_REGULAR_FILE, "file1\0");
	printf("Create file returned: %d\n",rc);

/*
	inum=MFS_Lookup(2,"file1\0");
	printf("Lookup file1 in pinum 2 returned:%d\n",inum);

	rc=MFS_Unlink(2,"file1\0");
	printf("Unlink file1 returned: %d\n",rc);
//	assert(rc==0);
	
	inum=MFS_Lookup(2,"file1\0");
	printf("Lookup after unlink of file1 in pinum 2 returned:%d\n",inum);
*/
    char *buff = malloc(4096);
    sprintf(buff, "File created successfully");
    rc = MFS_Write(3, buff, 0);
    assert(rc == 0);
    
    char *retBuff = malloc(4096);
    rc = MFS_Read(3, retBuff, 0);
    if(strcmp(retBuff, buff) != 0) {
		printf("Problem\n");
        exit(-1);
    }

	MFS_Shutdown();
	return 0;
}
