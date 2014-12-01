#include <stdio.h>
#include "udp.h"
#include "mfs.h"


#define BUFFSIZE 1024

int main(int argc, char *argv[])
{
	if(argc != 3)
	{
		fprintf(stderr,"Usage: client [Server-portnum] [Message-to-send]\n");
		exit(0);
	}

	int portNum=atoi(argv[1]);

	struct sockaddr_in addr;//structure to store address of incoming socket(server in this case)
	char buffer[BUFFSIZE];//1kb buffer size for now
	sprintf(buffer, argv[2]);//copy message to send

	//Always running on local so for now get local hostname
	char *hostname[1024];
	gethostname((char*)hostname,1024);
	
	//Find the server with the hostname and port number... to be later moved to MFS_Init(char* hostname, int port)
	int sd=UDP_Open(0);//open a socket for the client first
	assert(sd > -1);

	int rc=UDP_FillSockAddr(&addr, (char*)hostname, portNum);
	assert(rc==0);

	//Send message to server	
	rc = UDP_Write(sd, &addr, buffer, BUFFSIZE);//Send back the response to the client
	if(rc < 0)
		printf("Client UDP_write failed\n");
	
	rc=UDP_Read(sd, &addr, buffer, BUFFSIZE);
	if(rc>0)
	{
		char * msg = strdup(buffer);
		printf("Client recieved: %s\n",msg);
	}

	return 0;


}
