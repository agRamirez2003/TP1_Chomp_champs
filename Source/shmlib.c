#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

void *createSHM(char *name, size_t size, int *outFD)
{
    int fd;
    fd = shm_open(name, O_RDWR | O_CREAT, 0666);
    if (fd == -1)
    {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    if (-1 == ftruncate(fd, size))
    {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    *outFD = fd;

    void *toReturn = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (toReturn == MAP_FAILED)
    {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    return toReturn;
}

void closeSHM(void *toClose, size_t size, int fd, char *name)
{
    munmap(toClose, size);
    close(fd);
    shm_unlink(name);
}
