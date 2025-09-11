#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <semaphore.h>
#include <stdbool.h>
#include <sys/select.h>
#include <time.h>
#include <string.h>
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
    sem_t playerTurn[MAX_PLAYERS]; // Le indican a cada jugador que puede enviar 1 movimiento
    
} Semaphores;

int getParameters(int argc, char *argv[], Parameters *params) {
	params->width = 10;
	params->height = 10;
	params->delay = 200;
	params->timeout = 10;
	params->seed = time(NULL);
	params->view = NULL;
	params->cantPlayers = 0;

    for (int i = 1; i < argc; i++) {
        char *arg = argv[i];
        if (arg[0] == '-' && i + 1 < argc) {
            int value = atoi(argv[++i]);   // take next as number
            switch (arg[1]) {
                case 'w': params->width = value; break;
                case 'h': params->height = value; break;
                case 'd': params->delay = value; break;
                case 't': params->timeout = value; break;
                case 's': params->seed = value; break;
                case 'v': params->view = argv[i-1]; break;
                case 'p': params->playerPaths[params->cantPlayers] = argv[i-1]; params->cantPlayers++; break;
                default:  printf("Unknown option: %s\n", arg); return 0; break;
            }
        }
    }
	if(params->width < 10){
		printf("width must be more or equal to 10\n");
		return 0;
	}
	if(params->height < 10){
		printf("height must be more or equal to 10\n");
		return 0;
	}
	if(!params->cantPlayers){
		printf("you must specify at least one player path using -p\n");
		return 0;
	}
	return 1;
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

int squareOccupied(GameState *gameState, int square){
	return(gameState->board[square] <= 0); // A value less or equal than 0 means its occupied
}

int validSquare(GameState *gameState, int x, int y){
	if(x < 0 || x >= gameState->width || y < 0 || y >= gameState->width){
		return 0;
	}
	if(squareOccupied(gameState, gameState->width * y + x)){
		return 0;
	}
	return 1;
}

int newXCalculator(currentX, move){
	int dx[8] = {0, 1, 1, 1, 0, -1, -1, -1};
	return currentX + dx[move];
}

int newYCalculator(currentY, move){
	int dy[8] = {-1, -1, 0, 1, 1, 1, 0, -1};
	return currentY + dy[move];
}

int newPositionCalculator(int currentPos, int move, int width){
	int nx = newXCalculator(currentPos % width, move);
	int ny = newYCalculator(currentPos / width, move);
	return (width * ny + nx);
}

int getPlayerPos(GameState *gameState, int numPlayer){
	return (gameState->width * gameState->players[numPlayer].y + gameState->players[numPlayer].x);
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

void calculateNextPosition(int landingSquares[MAX_PLAYERS], int moves[MAX_PLAYERS], GameState *gameState){
	int startingPlayer = rand() % gameState->cantPlayers;
	for(int i = 0; i < gameState->cantPlayers; i++){
		int player = (i + startingPlayer) % gameState->cantPlayers;
		int currentPos = getPlayerPos(gameState, player);
		int desiredPos = newPositionCalculator(currentPos, moves[player], gameState->width);

		if(moves[player] == -1 || squareOccupied(gameState, desiredPos)){
			landingSquares[player] = currentPos;
			continue;
		}
		landingSquares[player] = desiredPos;
	}
}

void checkToBlockPlayers(Player *players, int cantPlayers, bool block[MAX_PLAYERS]){
	return ; // TODO
}

int main (int argc, char *argv[]) {
    GameState gameState;
    Semaphores semaphores;
    Parameters params;
    int SHMfds[SHM_QUANT];
	srand(time(NULL));

    if(!getParameters(argc, argv, &params)){
		return -1;
	}

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
    int moves[MAX_PLAYERS];            // movimiento que desea hacer el jugador i (-1 si no se mueve, 0-8 para moverse en alguna direccion)
    //Loop de juego
    while (!gameState.gameFinished) {
        checkToBlockPlayers(gameState.players, gameState.cantPlayers, block);
        maxFD = prepareFDSet(&readablePipes, block, gameState.cantPlayers,pipesFD);
        checkBlockedPlayers(block, &gameState);
        //readMoves(moves); //usar select
        validateMoves(moves, &gameState); // verifies the validity of the moves
        calculateNextPosition(landingSquares, moves, &gameState); // calculates the next moves without an advantage for any player
        sem_wait(&semaphores.turnstile);
        sem_wait(&semaphores.readWriteMutex);
        if(maxFD != -1){                //revisar
            // Zona critica: Modificar el estado del juego
            for (int i=0; i < gameState.cantPlayers;i++){
                if (block[i]) {
                    gameState.players[i].isBlocked=true;
                    continue;
                }
                if (moves[i]!= -1) { 
                    if (!validMove[i] || spaceOccupied(landingSquares[i], &gameState)){
                        gameState.players[i].invalidMoves++;
                        continue;
                    }
                    movePlayer(i, landingSquares[i], &gameState);
                    gameState.players[i].validMoves++;
                }
            }
        }
        else{
            gameState.gameFinished= true;
        }


        sem_post(&semaphores.readWriteMutex);
        sem_post(&semaphores.turnstile);

        //Semaforos para notificar a la vista que hay cambios
        sem_post(&semaphores.readyToPrint);
        sem_wait(&semaphores.finishedPrinting);
    }
    
}