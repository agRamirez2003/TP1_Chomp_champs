#ifndef GAMELIB_H
#define GAMELIB_H

#define MAX_PLAYERS 9
#include "playerslib.h"

typedef struct {
    unsigned short width;        // Ancho del tablero
    unsigned short height;       // Alto del tablero
    unsigned int cantPlayers;    // Cantidad de jugadores
    Player players[MAX_PLAYERS]; // Lista de jugadores
    bool gameFinished;           // Indica si el juego se ha terminado
    int board[];                 // Puntero al comienzo del tablero. fila-0, fila-1, ..., fila-n-1
} GameState;

void initializeBoard(int* board, int width, int height, int seed);

#endif // GAMELIB_H