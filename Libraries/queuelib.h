#ifndef QUEUELIB_H
#define QUEUELIB_H

#include <stdio.h>
#include <stdlib.h>

#define MAX 1000

typedef struct{
	int data[MAX];
	int front;
	int rear;
	int size;
} Queue;

void qinit(Queue *q);
void qpush(Queue *q, int value);
int qpop(Queue *q);
int qlen(Queue *q);

#endif