#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <semaphore.h>
#include <stdbool.h>
#include <sys/select.h>
#include <sys/time.h>
#include "../Libraries/playerslib.h"
#include "../Libraries/gamelib.h"

#define SHM_STATE_NAME "/game_state"
#define SHM_SEMAPHORES_NAME "/game_sync"

#define SHM_QUANT 2 


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

int initializeSemaphores(Semaphores *semaphores) {
    int shm_semaphores_fd;
    semaphores = (Semaphores *) createSHM(SHM_SEMAPHORES_NAME, sizeof(Semaphores), &shm_semaphores_fd);
    sem_init(&semaphores->readyToPrint, 1, 0);
    sem_init(&semaphores->finishedPrinting, 1, 0);
    sem_init(&semaphores->turnstile, 1, 1);
    sem_init(&semaphores->readWriteMutex, 1, 1);
    sem_init(&semaphores->cantReadersMutex, 1, 1);
    semaphores->cantReading = 0;
    return shm_semaphores_fd;
}

int initializeGameState(GameState *gameState, Parameters *params) {
    int shm_state_fd;
    gameState = (GameState *) createSHM(SHM_STATE_NAME, sizeof(GameState) + params->width * params->height * sizeof(int), &shm_state_fd);
    
    gameState->width = params->width;
    gameState->height = params->height;
    gameState->cantPlayers = params->cantPlayers;
    gameState->gameFinished = false;

    for (int i = 0; i < gameState->cantPlayers; i++) {
        gameState->players[i] = createNewPlayer(params->playerPaths[i]);
    }

    initializeBoard(gameState->board, params->width, params->height, params->seed);
    return shm_state_fd;
}

void forkPlayers(GameState *gameState, char* playerPaths[MAX_PLAYERS] , int pipesFD[MAX_PLAYERS][2]) {
    for (int i = 0; i < gameState->cantPlayers; i++) {
        if (pipe(pipesFD[i]) == -1){
            perror("fork");
            exit(EXIT_FAILURE);
        }
        gameState->players[i].pid = fork();
        if (gameState->players[i].pid == -1){
            perror("fork");
            exit(EXIT_FAILURE);
        }

        if (gameState->players[i].pid == 0){   
            //Cierro pipes de hijos anteriores
            for (int j = 0; j < i; j++) {
                close(pipesFD[j][0]);
                close(pipesFD[j][1]);
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
            if (error == -1){
                perror("view execve failed");
                exit(EXIT_FAILURE);
            }
        }
        else {
            close(pipesFD[i][1]);
        }
    }
}

int prepareFDSet(fd_set* set, bool block[MAX_PLAYERS], int cantPLayers,int fds[MAX_PLAYERS][2]) {
    FD_ZERO(set);           
    int maxFD= -1;

    for (size_t i = 0; i < cantPLayers; i++){
        if (!block[i]) {
            FD_SET(fds[i][0], set);
            if (fds[i][0] > maxFD) {
                maxFD = fds[i][0];
            }
        }
    }
    
    return maxFD;
}
//Si se agregan mas SHM, tambien cambiar el define de SHM_QUANT
void initializeSHM(GameState *gameState, Parameters *params, Semaphores *semaphores, int SHMfds[SHM_QUANT]) {
    SHMfds[0]=initializeGameState(gameState, params);
    SHMfds[1]=initializeSemaphores(semaphores);
}

int main (int argc, char *argv[]) {
    GameState gameState;
    Semaphores semaphores;
    Parameters params;
    int SHMfds[SHM_QUANT];

    getParameters(argc, argv, &params);

    initializeSHM(&gameState, &params, &semaphores, SHMfds);

    int pipesFD[MAX_PLAYERS][2];
    forkPlayers(&gameState, params.playerPaths, pipesFD);

    //int moves[MAX_PLAYERS];
    fd_set readablePipes;
    int maxFD;
    bool validMove[MAX_PLAYERS];     // indica si el movimiento del jugador i fue hecho dentro del tablero
    bool block[MAX_PLAYERS];         // indica si hay que bloquear al jugador i
    //bool everyoneBlocked;            // flag para terminar el juego si todos los jugadores estan bloqueados
    int landingSquares[MAX_PLAYERS]; // index de board[] donde el jugador desea moverse (no se verifica si hay un jugador dentro porque podria haber una colision entre jugadores)
    //Loop de juego
    while (!gameState.gameFinished) {
        checkToBlockPlayers(gameState.players, gameState.cantPlayers, block);
        maxFD = prepareFDSet(&readablePipes, block, gameState.cantPlayers,pipesFD);
        //readMoves(moves);
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