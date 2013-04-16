PROGS = zhuftpclient
all:$(PROGS)
CC = gcc
OBJS=main.o ftpclient_UI.o fileutils.o transfer.o

CFLAGS=-g  -I.\
       `pkg-config --cflags gtk+-2.0 glib-2.0 gthread-2.0`

LIBS=-L. `pkg-config --libs gtk+-2.0 glib-2.0 gthread-2.0 `\
     -lpthread\
     -L/usr/local/lib/libgthread-2.0.so

.c.o:
	$(CC) $(CFLAGS) -c $<

zhuftpclient:$(OBJS)
	$(CC) -o $@ $(OBJS) $(LIBS)

main.o:ftpclient_UI.h
ftpclient_UI.o:ftpclient_UI.h 
fileutils.o:fileutils.h 
transfer.o:transfer.h

clean:
	rm -rf *.o $(PROGS)

