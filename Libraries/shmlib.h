#ifndef SHM_LIB_H
#define SHM_LIB_H

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

// Crea y mapea una memoria compartida con permisos RD y WR, con modo 0666, y retorna un puntero a la misma. OutFd es el puntero al FD
void *createSHM(char *name, size_t size, int *outFD);

// unlink, unmap y close de la memoria compartida y su fd.
void closeSHM(void *toClose, size_t size, int fd, char *name);

#endif