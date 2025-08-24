#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
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
    char *playerPaths[MAX_PLAYERS];
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

void forkPlayers(GameState *gameState, char* playerPaths[MAX_PLAYERS] , int pipesFD[MAX_PLAYERS][2]) {
    for (int i = 0; i < gameState->cantPlayers; i++) {
        if (pipe(pipesFD[i]) == -1)
        {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        gameState->players[i].pid = fork();
        if (gameState->players[i].pid == -1)
        {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        if (gameState->players[i].pid == 0)
        {   
            //Cierro pipes de hijos anteriores
            for (int j = 0; j < i; j++) {
                close(pipesFD[j][0]);
            }
            close(pipesFD[i][0]);
            dup2(pipesFD[i][1], STDOUT_FILENO);
            close(pipesFD[i][1]);
            char *args[4];
            char aux[50];
            strcpy(aux, playerPaths[i]);
            args[0] = basename(aux); // nombre del ejecutable
            char widthAux[5], heightAux[10];
            snprintf(widthAux, sizeof(widthAux), "%d", gameState->width);
            snprintf(heightAux, sizeof(heightAux), "%d", gameState->height);
            args[1] = widthAux;
            args[2] = heightAux;
            args[3] = NULL;
            int error = execve(playerPaths[i], args, NULL);
            if (error == -1)
            {
                perror("view execve failed");
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            close(pipesFD[i][1]);
        }
    }
}

void initializeSHM(GameState *gameState, Parameters *params, Semaphores *semaphores) {
    initializeGameState(gameState, params);
    initializeSemaphores(semaphores);
}

int main (int argc, char *argv[]){
    GameState gameState;
    Semaphores semaphores;
    Parameters params;

    getParameters(argc, argv, &params);

    initializeSHM(&gameState, &params, &semaphores);

    int pipesFD[MAX_PLAYERS][2];
    forkPlayers(&gameState, params.playerPaths, pipesFD);

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