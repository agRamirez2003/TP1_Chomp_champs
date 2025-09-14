CC = gcc
CFLAGS = -Wall -Wextra -O2

# Libraries for master
MASTER_DEPS = gamelib.c shmlib.c playerslib.c

# Executables
all: master player view

# Master depends on libs
master: master.c $(MASTER_DEPS)
	$(CC) $(CFLAGS) $^ -o $@

# Player is standalone
player: player.c
	$(CC) $(CFLAGS) $< -o $@

# View is standalone
view: view.c
	$(CC) $(CFLAGS) $< -o $@

# Clean
clean:
	rm -f master player view