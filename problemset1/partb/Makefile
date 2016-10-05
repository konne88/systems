CFLAGS = -Ilibevent -Ilibevent/include -Wall
LIBS   = libevent/.libs/libevent.a -lrt

all: libevent 550server

550server:
	$(CC) $(CFLAGS) -g -O2 -o 550server main.c $(LIBS)

libevent:
	tar -xvf libevent*.tar.gz
	mv libevent*/ libevent
	cd libevent/ && ./configure && make 

clean:
	rm -rf libevent/ 550server
