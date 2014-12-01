CC=gcc
CFLAGS= -Wall -Werror -g

all: libmfs.so server client

#Executables
server: server.o udp.o
	$(CC) $(CFLAGS) -o server server.o udp.o

client: client.o libmfs.so
	$(CC) -lmfs -L. $(CFLAGS) -o client client.o


libmfs.so: udp.o mfs.o
	$(CC) $(CFLAGS) -shared -o libmfs.so udp.o mfs.o

mfs.o: mfs.c
	$(CC) $(CFLAGS) -c -fpic mfs.c -o mfs.o
udp.o: udp.c
	$(CC) $(CFLAGS) -c -fpic udp.c -o udp.o

server.o: server.c
	$(CC) $(CFLAGS) -c server.c -o server.o

client.o: client.c
	$(CC) $(CFLAGS) -c client.c -o client.o

#rule for .o files found in dependencies of other targets(server, client)
#%.o: %.c
#	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f server.o udp.o mfs.o server client libmfs.so
