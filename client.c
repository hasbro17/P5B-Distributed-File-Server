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

    //rc = MFS_Creat(0, MFS_DIRECTORY, "dir2\0");
    rc = MFS_Creat(0, MFS_REGULAR_FILE, "file1\0");
	printf("Create returned: %d\n",rc);

	return 0;
}
