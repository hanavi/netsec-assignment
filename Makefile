all: techrypt techdec

CFLAGS=-lgcrypt -ggdb

techrypt: techrypt.o crypto.o network.o
	gcc techrypt.o crypto.o network.o -o techrypt $(CFLAGS)

techdec: techdec.o crypto.o network.o
	gcc techdec.o crypto.o network.o -o techdec $(CFLAGS)

techrypt.o: techrypt.c techrypt.h 
	gcc -c techrypt.c $(CFLAGS)

techdec.o: techdec.c techdec.h network.h
	gcc -c techdec.c $(CFLAGS)

crypto.o: crypto.c crypto.h
	gcc -c crypto.c $(CFLAGS)

network.o: network.c network.h
	gcc -c network.c $(CFLAGS)

clean:
	rm -fr *.o techrypt techdec *.gt *.check
