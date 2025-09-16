CC = gcc
CFLAGS = -Wall -pedantic -std=c99
LIBS = gamelib.c playerslib.c shmlib.c

# Archivos principales que producen ejecutables
TARGETS = master playerRand view

all: $(TARGETS)

master: master.c $(LIBS)
	$(CC) $(CFLAGS) -o $@ $^
	chmod 777 $@

playerRand: playerRand.c $(LIBS)
	$(CC) $(CFLAGS) -o $@ $^
	chmod 777 $@

view: view.c $(LIBS)
	$(CC) $(CFLAGS) -o $@ $^
	chmod 777 $@

clean:
	rm -f $(TARGETS) *.o