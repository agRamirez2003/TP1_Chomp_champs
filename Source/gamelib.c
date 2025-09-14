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
    for (size_t i = 0; i < POSSIBLE_MOVES; i++){ //TODO: rehacerlo para que recorra el board alrededor del jugador con 2 for anidados 
        switch (i)
        {
        case 0:
            if (playerY == 0)
                break;
            if (board[currentPos - boardWidth] >= 0)
                return false;
            break;
        case 1:
            if (playerY == 0 || playerX == boardWidth - 1)
                break;
            if (board[currentPos - boardWidth + 1] >= 0)
                return false;
            break;
        case 2:
            if (playerX == boardWidth - 1)
                break;
            if (board[currentPos + 1] >= 0)
                return false;
            break;
        case 3:
            if (playerY == boardHeight - 1 || playerX == boardWidth - 1)
                break;
            if (board[currentPos + boardWidth + 1] >= 0)
                return false;
            break;
        case 4:
            if (playerY == boardHeight - 1)
                break;
            if (board[currentPos + boardWidth] >= 0)
                return false;
            break;
        case 5:
            if (playerY == boardHeight - 1 || playerX == 0)
                break;
            if (board[currentPos + boardWidth - 1] >= 0)
                return false;
            break;
        case 6:
            if (playerX == 0)
                break;
            if (board[currentPos - 1] >= 0)
                return false;
            break;
        case 7:
            if (playerY == 0 || playerX == 0)
                break;
            if (board[currentPos - boardWidth - 1] >= 0)
                return false;
            break;
        }
    }
    return true;
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
			moves[i] = -1; // no move as it was an invalid move
		}
	}
}