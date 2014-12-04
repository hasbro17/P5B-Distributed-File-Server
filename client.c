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

    
	rc = MFS_Creat(2, MFS_REGULAR_FILE, "file1\0");
	printf("Create file returned: %d\n",rc);


    char *buff = malloc(4096);
    sprintf(buff, "srart block");
    rc = MFS_Write(3, buff, 0);
    assert(rc == 0);
    
    char *retBuff = malloc(4096);
    rc = MFS_Read(3, retBuff, 0);
    if(strcmp(retBuff, buff) != 0) {
		printf("Problem\n");
        exit(-1);
    }

	return 0;
}
