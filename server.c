#include <stdio.h>
#include "udp.h"
#include "mfs.h"

#define BUFFSIZE 1024

int sd;//socket descriptor
int fdImage;//file descriptor for disk image
char name[1024];//name of file system image
struct sockaddr_in addr;



int main(int argc, char *argv[])
{
	if(argc!=3)
	{
		fprintf(stderr,"Usage: server [portnum] [file-system-image]\n");
		exit(0);
	}

	int portNum=atoi(argv[1]);
	//Open the port
	sd=UDP_Open(portNum);
	assert(sd != -1);
	
	//Server listening in loop
	while(1)
	{
		struct sockaddr_in addr;//structure to store address of incoming socket
		char buffer[BUFFSIZE];//1kb buffer size for now
		//read incoming message from port
		int rc=UDP_Read(sd, &addr, buffer, BUFFSIZE);
		if(rc>0)
		{
			char * msg = strdup(buffer);
			printf("Server recieved:%s\n",msg);
			sprintf(buffer, "Server got your message: %s\n",msg);
			rc = UDP_Write(sd, &addr, buffer, BUFFSIZE);//Send back the response to the client
			if(rc < 0)
				printf("Server UDP_write failed\n");
		}

	}
	return 0;
}



