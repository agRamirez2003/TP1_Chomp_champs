#include "../Libraries/queuelib.h"

void qinit(Queue *q){
	q->front = 0;
	q->rear = -1;
	q->size = 0;
}

void qpush(Queue *q, int value){
	if(q->size == MAX){
		return ;
	}
	q->rear = (q->rear+1) % MAX;
	q->data[q->rear] = value;
	q->size++;
}

int qpop(Queue *q){
	if(q->size == 0){
		return -1;
	}
	int value = q->data[q->front];
	q->front = (q->front + 1) % MAX;
	q->size--;
	return value;
}

int qlen(Queue *q){
	return q->size;
}