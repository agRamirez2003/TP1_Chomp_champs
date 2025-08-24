#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include "../Libraries/playerslib.h"

#define MAX_PLAYERS 9
#define SHM_STATE_NAME "/game_state"
#define SHM_SEMAPHORES_NAME "/game_semaphores"

typedef struct {
    unsigned short width;        // Ancho del tablero
    unsigned short height;       // Alto del tablero
    unsigned int cantPlayers;    // Cantidad de jugadores
    Player players[MAX_PLAYERS]; // Lista de jugadores
    bool gameFinished;           // Indica si el juego se ha terminado
    int board[];                 // Puntero al comienzo del tablero. fila-0, fila-1, ..., fila-n-1
} GameState;

typedef struct {
    int width;
    int height;
    int timeout;
    int seed;
    int cantPlayers;
    int delay;
    char *view;
} Parameters;

typedef struct{
    sem_t readyToPrint;       // Notify view of changes
    sem_t finishedPrinting;   // Notify master that printing is done
    sem_t turnstile;          // Mutex to prevent starvation
    sem_t readWriteMutex;     // Read/Write Game state mutex 
    sem_t cantReadersMutex;   // Mutex for turnstile counter
    unsigned int cantReading; // Number of players reading state 
} Semaphores;

void getParameters(int argc, char *argv[], Parameters *params) {
    // TODO: funcion para inicializar los parametros
}

void initializeSemaphores(Semaphores *semaphores) {
    int shm_semaphores_fd;
    semaphores = (Semaphores *) createSHM(SHM_SEMAPHORES_NAME, sizeof(Semaphores), &shm_semaphores_fd);
    sem_init(&semaphores->readyToPrint, 1, 0);
    sem_init(&semaphores->finishedPrinting, 1, 0);
    sem_init(&semaphores->turnstile, 1, 1);
    sem_init(&semaphores->readWriteMutex, 1, 1);
    sem_init(&semaphores->cantReadersMutex, 1, 1);
    semaphores->cantReading = 0;
}

void initializeGameState(GameState *gameState, Parameters *params) {
    int shm_state_fd;
    gameState = (GameState *) createSHM(SHM_STATE_NAME, sizeof(GameState) + params->width * params->height * sizeof(int), &shm_state_fd);
    
    gameState->width = params->width;
    gameState->height = params->height;
    gameState->cantPlayers = params->cantPlayers;
    gameState->gameFinished = false;

    for (int i = 0; i < MAX_PLAYERS; i++) {
        //TODO:Inicializar jugadores
    }
    //TODO: libreria para lo relacionado al juego (como esta funcion)
    initializeBoard(gameState->board, params->width, params->height, params->seed);
}

void initializeSHM(GameState *gameState, Parameters *params, Semaphores *semaphores) {
    // funcion para inicializar el estado del juego y los semaforos
    initializeGameState(gameState, params);
    initializeSemaphores(semaphores);
}

int main (int argc, char *argv[]){
    GameState gameState;
    Semaphores semaphores;
    Parameters params;

    getParameters(argc, argv, &params);

    initializeSHM(&gameState, &params, &semaphores);

    
    //Loop de juego
    while (!gameState.gameFinished)
    {

        sem_wait(&semaphores.turnstile);
        sem_wait(&semaphores.readWriteMutex);

        // Zona critica: Modificar el estado del juego

        sem_post(&semaphores.readWriteMutex);
        sem_post(&semaphores.turnstile);

        //Semaforos para notificar a la vista que hay cambios
        sem_post(&semaphores.readyToPrint);
        sem_wait(&semaphores.finishedPrinting);
    }
    
}