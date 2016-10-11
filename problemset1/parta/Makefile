CC = g++
CFLAGS = -Wall -std=c++11

OBJS = 550shell.o

all: 550shell

550shell: $(OBJS)
	$(CC) $(CFLAGS) -g -o $@ $^

%.o: %.cpp
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o *~ 550shell
