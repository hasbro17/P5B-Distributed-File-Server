#include "mfs.h"
#include "udp.h"


struct sockaddr_in addr;
int sd;
int portNum;

struct timeval tv;
fd_set readfds;


//sends the message msg and returns the server response by overwriting the original msg
void send_and_recieve(char *msg)
{
	int rv=0;
	int rc;
	//Keep retrying until success, wait 5 seconds before retry
	while(rv==0)
	{
		rc=UDP_Write(sd, &addr, msg, sizeof(msg_t));

		FD_ZERO(&readfds);
		FD_SET(sd, &readfds);
		tv.tv_sec=5;// 5 second timeout
		tv.tv_usec=0;

		rv=select(sd+1, &readfds, NULL, NULL, &tv);

		if(rv>0)
		{
			if(rc>0)//if the original write was successful
			{
				struct sockaddr_in tempaddr;//don't want to change original server address if it does change
				rc=UDP_Read(sd, &tempaddr, msg, sizeof(msg_t));
			}
		}
	}

}


int MFS_Init(char *hostname, int port)
{
	portNum=port;
	sd=UDP_Open(0);
	assert(sd > -1);

	int rc= UDP_FillSockAddr(&addr, hostname, portNum);
	assert(rc == 0);
	return rc;	
}

int MFS_Lookup(int pinum, char *name)
{
	//initialize msg
	msg_t msg;
	msg.func=LOOKUP;
	msg.pinum=pinum;
	//memcpy(msg.name, name, sizeof(msg.name));
	sprintf(msg.name,name);
	msg.inum=-1;
	msg.block=-1;
	msg.retCode=-1;

	send_and_recieve((char*)&msg);
	return msg.retCode;
}

int MFS_Stat(int inum, MFS_Stat_t *m)
{
	//initialize msg
	msg_t msg;
	msg.func=STAT;
	msg.pinum=-1;
	msg.inum=inum;
	msg.block=-1;
	msg.retCode=-1;

	send_and_recieve((char*)&msg);

	m->type=msg.stat.type;
	m->size=msg.stat.size;
	return msg.retCode;
}

int MFS_Write(int inum, char *buffer, int block)
{
	//initialize msg
	msg_t msg;
	msg.func=WRITE;
	msg.pinum=-1;
	msg.inum=inum;
	msg.block=block;
	msg.retCode=-1;
	memcpy(msg.buffer,buffer,sizeof(msg.buffer));

	send_and_recieve((char*)&msg);

	return msg.retCode;
}

int MFS_Read(int inum, char *buffer, int block)
{
	//initialize msg
	msg_t msg;
	msg.func=READ;
	msg.pinum=-1;
	msg.inum=inum;
	msg.block=block;
	msg.retCode=-1;

	send_and_recieve((char*)&msg);

	memcpy(buffer, msg.buffer,sizeof(msg.buffer));
	
	return msg.retCode;
}

int MFS_Creat(int pinum, int type, char *name)
{
	//initialize msg
	msg_t msg;
	msg.func=CREAT;
	msg.pinum=pinum;
	msg.inum=-1;
	msg.block=-1;
	msg.retCode=-1;
	msg.stat.type=type;
	sprintf(msg.name,name);
		
	send_and_recieve((char*)&msg);

	return msg.retCode;
}

int MFS_Unlink(int pinum, char *name)
{

	//initialize msg
	msg_t msg;
	msg.func=UNLINK;
	msg.pinum=pinum;
	msg.inum=-1;
	msg.block=-1;
	msg.retCode=-1;
	sprintf(msg.name,name);
	
	send_and_recieve((char*)&msg);

	return msg.retCode;
}

int MFS_Shutdown()
{
	//initialize msg
	msg_t msg;
	msg.func=SHUTDOWN;
	msg.retCode=-1;
	
	send_and_recieve((char*)&msg);

	return msg.retCode;
}
