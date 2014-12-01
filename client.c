#include <stdio.h>
#include "udp.h"
#include "mfs.h"


#define BUFFSIZE 1024

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

	return 0;


}
