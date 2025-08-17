#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdbool.h>

#define MAX_PLAYERS 9


typedef struct
{
    char name[16];             // Nombre del jugador
    unsigned int score;        // Puntaje
    unsigned int invalidMoves; // Cantidad de solicitudes de movimientos inválidas realizadas
    unsigned int validMoves;   // Cantidad de solicitudes de movimientos válidas realizadas
    unsigned short x, y;       // Coordenadas x e y en el tablero
    pid_t pid;                 // Identificador de proceso
    bool isBlocked;            // Indica si el jugador está bloqueado
} Player;