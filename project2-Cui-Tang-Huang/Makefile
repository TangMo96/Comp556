CC          = g++
CFLAGS      = -Wall -std=c++11

DIR_RECVFILE  = ./src/recvfile
DIR_SENDFILE  = ./src/sendfile


all: recvfile sendfile

recvfile:
	$(CC) $(CFLAGS) -o recvfile $(DIR_RECVFILE)/*.h $(DIR_RECVFILE)/*.cpp

sendfile:
	$(CC) $(CFLAGS) -o sendfile $(DIR_SENDFILE)/*.h $(DIR_SENDFILE)/*.cpp

clean:
	rm -f *.o
	rm -f *~
	rm -f *.gch
	rm -f recvfile
	rm -f sendfile
