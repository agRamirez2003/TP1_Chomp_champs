#define _POSIX_C_SOURCE 200112L // Si no pongo esta linea no me funciona ftrunctate ni getopt (agus)
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

#include "../Libraries/playerslib.h"
#include "../Libraries/goodPlayerLib.h"
#include "../Libraries/queuelib.h"
#include <limits.h>

#define MAX_PLAYERS 9
#define DIRECTIONS 8
#define INFINITY INT_MAX
#define MINUS_INFINITY INT_MIN

int countReachableTiles(GameState *gameState, int position, int boardWidth, int boardHeight){
	int visited[boardWidth * boardHeight];
	memset(visited, 0, boardWidth * boardHeight);
	Queue *queue;
	qinit(queue);

	qpush(queue, position);
	visited[position] = 1;

	int dx[8] = {0, 1, 1, 1, 0, -1, -1, -1};
	int dy[8] = {-1, -1, 0, 1, 1, 1, 0, -1};

	int count = 0;

	while(qlen(queue) > 0){
		int currentPos = qpop(queue);
		count++;

		for(int i = 0; i < DIRECTIONS; i++){
			int nx = currentPos % boardWidth + dx[i];
			int ny = currentPos / boardWidth + dy[i];

			if(nx < 0 || ny < 0 || nx >= boardWidth || ny >= boardHeight){
				continue;
			}
			if(visited[boardWidth * nx + ny] || gameState->board[boardWidth * nx + ny] <= 0){
				continue;
			}
			visited[boardWidth * nx + ny] = 1;
			qpush(queue, boardWidth * nx + ny);
		}
	}
}

int makeMove(GameState* gameState, int playerNum, int move, int boardWidth, int boardHeight){
	int dx[8] = {0, 1, 1, 1, 0, -1, -1, -1};
	int dy[8] = {-1, -1, 0, 1, 1, 1, 0, -1};
	
	if(move < 0 || move >= DIRECTIONS){
		return 0;
	}

	int nx = gameState->players[playerNum].x + dx[move];
	int ny = gameState->players[playerNum].y + dy[move];

	if(nx < 0 || ny < 0 || nx >= boardWidth || ny >= boardHeight){
		return 0;
	}
	if(gameState->board[boardWidth * nx + ny] <= 0){
		return 0;
	}
	
	// set the new (temporary) values

	gameState->players[playerNum].x = nx;
	gameState->players[playerNum].y = ny;
	gameState->board[boardWidth * nx + ny] = playerNum * -1;
	return 1;
}

void unMakeMove(GameState* gameState, int playerNum, int move, int boardWidth, int boardHeight){
	int dx[8] = {0, 1, 1, 1, 0, -1, -1, -1};
	int dy[8] = {-1, -1, 0, 1, 1, 1, 0, -1};

	int nx = gameState->players[playerNum].x - dx[move];
	int ny = gameState->players[playerNum].y - dy[move];

	gameState->players[playerNum].x = nx;
	gameState->players[playerNum].y = ny;
	gameState->board[boardWidth * nx + ny] = playerNum * -1;
}

int *decideNextMoveRec(GameState *gameState, int boardWidth, int boardHeight){
	// TODO
	int *bestMove = malloc(sizeof(int) * 2);
	bestMove[0] = 0;
	bestMove[1] = MINUS_INFINITY;
	return bestMove;
}

unsigned char chooseGoodMove(GameState *gameState, int boardWidth, int boardHeight, int playerNum){
	int * bestMove = decideNextMoveRec(gameState, boardWidth, boardHeight); // 0: move, 1: score
	if(bestMove[0] != MINUS_INFINITY && bestMove[0] != INFINITY){
		return bestMove[0];
	}
	bestMove[0] = MINUS_INFINITY;
	for(int i = 0; i < DIRECTIONS; i++){
		int dx[8] = {0, 1, 1, 1, 0, -1, -1, -1};
		int dy[8] = {-1, -1, 0, 1, 1, 1, 0, -1};
		if(gameState->players[playerNum].x + dx[i] >= boardWidth ||
			gameState->players[playerNum].x + dx[i] < 0 ||
			gameState->players[playerNum].y + dy[i] >= boardWidth ||
			gameState->players[playerNum].y + dy[i] < 0
			){
			continue;
		}
		if(gameState->board[boardWidth * (gameState->players[playerNum].x + dx[i]) + gameState->players[playerNum].y + dy[i]] > 0){ // occupied cell
			continue;
		}
		if(!makeMove(gameState, playerNum, i, boardWidth, boardHeight)){
			continue;
		}
		int bestJScore = 0;
		for(int j = 0; j < DIRECTIONS; j++){
			if(gameState->players[playerNum].x + dx[i] >= boardWidth ||
				gameState->players[playerNum].x + dx[i] < 0 ||
				gameState->players[playerNum].y + dy[i] >= boardWidth ||
				gameState->players[playerNum].y + dy[i] < 0
				){
				continue;
			}
			if(gameState->board[boardWidth * (gameState->players[playerNum].x + dx[j]) + gameState->players[playerNum].y + dy[j]] > 0){ // occupied cell
				continue;
			}
			int currentScore = countReachableTiles(gameState, boardWidth * (gameState->players[playerNum].x + dx[j]) + gameState->players[playerNum].y + dy[j], boardWidth, boardHeight);

			if(currentScore > bestJScore){
				bestJScore = currentScore;
			}
		}
		
		if(bestJScore > bestMove[0]){
			bestMove[0] = bestJScore;
			bestMove[1] = i;
		}

		unMakeMove(gameState, playerNum, i, boardWidth, boardHeight);
	}
	return '1';
}

/*
ps: procesos
lsof: list open file | pipe
valgrind: chequeos de memory leaks (dinamico)
pvs studio: chequeos antes de entregarlo (estatico)
gdb para el posix el problema ese pedorro
getops
*/