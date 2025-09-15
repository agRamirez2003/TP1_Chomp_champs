#ifndef GAMELIB_H
#define GAMELIB_H

#define MAX_PLAYERS 9
#define POSSIBLE_MOVES 8
#define INVALID_MOVE -1
#define NOT_MOVED -2

#include "playerslib.h"
#include <stdbool.h>

typedef struct {
    unsigned short width;        // Ancho del tablero
    unsigned short height;       // Alto del tablero
    unsigned int cantPlayers;    // Cantidad de jugadores
    Player players[MAX_PLAYERS]; // Lista de jugadores
    bool gameFinished;           // Indica si el juego se ha terminado
    int board[];                 // Puntero al comienzo del tablero. fila-0, fila-1, ..., fila-n-1
} GameState;

void initializeBoard(int* board, int width, int height, int seed);

void checkBlockedPlayers(bool block[], GameState* gameState);

bool spaceOccupied(int landingSquare, GameState* gameState);

void validateMoves(int moves[MAX_PLAYERS], GameState *gameState);

void calculateNextPosition(int landingSquares[MAX_PLAYERS], int moves[MAX_PLAYERS], GameState *gameState);

#endif // GAMELIB_H