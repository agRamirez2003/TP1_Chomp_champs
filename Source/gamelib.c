#include "../Libraries/gamelib.h"
#include "../Libraries/playerslib.h"
#include <stdbool.h>



void initializeBoard(int* board, int width, int height, int seed) {
    srand(seed);
    for (int i = 0; i < width * height; i++) {
        board[i] = rand() % 10; 
    }
}
// Verifica si hay algun lugar libre alrededor del jugador, devolviendo false si lo hay.
static playerIsBlocked( int playerX, int playerY, int* board, int boardWidth, int boardHeight){
    int currentPos= playerY * boardWidth + playerX;
    for (size_t i = 0; i < 3; i++){
        int currentLine= playerY -1 + i;
        if (currentLine < 0 || currentLine >= boardHeight)
            continue;
        for (size_t j = 0; j < 3; j++){ 
            int currentCol= playerX -1 + j;
            if (currentCol< 0 || currentCol>= boardWidth)
                continue;
            int currentSquare= currentLine*boardWidth + currentCol;
            
            if (board[currentSquare] >= 0)
                return false;
        }
    }
    return true;
}

void calculateNextPosition(int landingSquares[MAX_PLAYERS], int moves[MAX_PLAYERS], GameState *gameState){
    for(int i = 0; i < gameState->cantPlayers; i++){
        int nx = newXCalculator(gameState->players[i].x, moves[i]);
        int ny = newYCalculator(gameState->players[i].y, moves[i]);
        landingSquares[i] = gameState->width * ny + nx;
    }
}

int newXCalculator(currentX, move){
	int dx[8] = {0, 1, 1, 1, 0, -1, -1, -1};
	return currentX + dx[move];
}

int newYCalculator(currentY, move){
	int dy[8] = {-1, -1, 0, 1, 1, 1, 0, -1};
	return currentY + dy[move];
}

void checkBlockedPLayers(bool block[], GameState* gameState){
    for (size_t i = 0; i < gameState->cantPlayers; i++){
        if (gameState->players[i].isBlocked){
            block[i]= true;
            continue;
        }
        block[i]= playerIsBlocked(gameState->players[i].x, gameState->players[i].y, gameState->board, gameState->width, gameState->height);
    }
}

bool spaceOccupied(int landingSquare, GameState* gameState){
    return gameState->board[landingSquare] < 0;
}

void validateMoves(int moves[MAX_PLAYERS], GameState *gameState){
	for(int i = 0; i < gameState->cantPlayers; i++){
		int nx = newXCalculator(gameState->players[i].x, moves[i]);
		int ny = newYCalculator(gameState->players[i].y, moves[i]);
		int newPos = gameState->width * ny + nx;

		if(!validSquare(gameState, nx, ny)){
			moves[i] = INVALID_MOVE; // no move as it was an invalid move
		}
	}
}