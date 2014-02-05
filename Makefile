CC = gcc
AR = ar
LIBS = -lz -lm
CFLAGS = -fpic -O3 -g --std=c99 -Wall -Wextra

PROGS = matio_read matio_write
OBJS = matread.o matwrite.o matsearch.o matclean.o
all : matio_read matio_write ;

matio_read : $(OBJS) matio_test.c ;
	$(CC) -o$@ $^ $(LIBS) $(CFLAGS)

matio_write : $(OBJS) matio_test.c ;
	$(CC) -o$@ $^ $(LIBS) $(CFLAGS) -D__WRITE

debug : $(OBJS) debug.c ;
	$(CC) -o$@ $^ $(LIBS) $(CFLAGS)

libmatio : $(OBJS) ;
	$(CC) -shared -o$@.so $^ $(CFLAGS)
	cp $@.so /home/mchale/lib
	cp matio.h /home/mchale/include/		

install : libmatio ;
	cp $^.so /usr/lib/

uninstall : ;
	rm -fr /usr/lib/libmatio.so

%.o : %.c ;
	$(CC) -c $^ $(CFLAGS)
	
clean : ;
	rm -fr $(PROGS)
	rm -fr *.o
	rm -fr *.so
