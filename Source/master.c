#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <semaphore.h>
#include <stdbool.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <time.h>
#include <string.h>
#include <libgen.h>
#include "../Libraries/playerslib.h"
#include "../Libraries/gamelib.h"
#include "../Libraries/shmlib.h"

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
    int viewFlag;
    char playerPaths[MAX_PLAYERS][50];
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

int getParameters(int argc, char *argv[], Parameters *params){
	params->width = 10;
	params->height = 10;
	params->delay = 200;
	params->timeout = 10;
	params->seed = time(NULL);
	params->view = NULL;
	params->cantPlayers = 0;
    params->viewFlag = 0;
    int opt;

    while ((opt = getopt(argc, argv, "w:h:d:t:s:v:p:")) != -1){
        switch (opt){
        case 'w': params->width = atoi(optarg);
            break;
        case 'h':
            params->height = atoi(optarg);
            break;
        case 'd':
            params->delay = atoi(optarg);
            break;
        case 't':
            params->timeout = atoi(optarg);
            break;
        case 's':
            params->seed = atoi(optarg);
            break;
        case 'v':
            params->view = optarg;
            params->viewFlag = 1;
            break;
        case 'p':
            // First player comes from optarg
            if (params->cantPlayers >= MAX_PLAYERS){
                printf("There have to be at most nine players\n");
                exit(EXIT_FAILURE);
            }
            strcpy(params->playerPaths[params->cantPlayers], optarg);
            params->cantPlayers++;

            // Now process remaining words in argv
            while (optind < argc && argv[optind][0] != '-')
            {
                if (params->cantPlayers == MAX_PLAYERS)
                {
                    printf("There have to be at most nine players\n");
                    exit(EXIT_FAILURE);
                }
                strcpy(params->playerPaths[params->cantPlayers], argv[optind]);
                params->cantPlayers++;
                optind++; // Move to next word
            }
            break;
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

int initializeSemaphores(Semaphores **semaphores) {
    int shm_semaphores_fd;
    *semaphores = (Semaphores *) createSHM(SHM_SEMAPHORES_NAME, sizeof(Semaphores), &shm_semaphores_fd);
    sem_init(&(*semaphores)->readyToPrint, 1, 0);
    sem_init(&(*semaphores)->finishedPrinting, 1, 0);
    sem_init(&(*semaphores)->turnstile, 1, 1);
    sem_init(&(*semaphores)->readWriteMutex, 1, 1);
    sem_init(&(*semaphores)->cantReadersMutex, 1, 1);
    (*semaphores)->cantReading = 0;
    for(int i = 0; i < MAX_PLAYERS; i++){
        sem_init(&(*semaphores)->playerTurn[i], 1, 1);
    }
    return shm_semaphores_fd;
}

int initializeGameState(GameState **gameState, Parameters *params) {
    int shm_state_fd;
    *gameState = (GameState *) createSHM(SHM_STATE_NAME,sizeof(GameState) + params->width * params->height * sizeof(int),&shm_state_fd);

    (*gameState)->width = params->width;
    (*gameState)->height = params->height;
    (*gameState)->cantPlayers = params->cantPlayers;
    (*gameState)->gameFinished = false;

    for (int i = 0; i < (*gameState)->cantPlayers; i++) {
        printf("Creating player %s\n", params->playerPaths[i]);
        (*gameState)->players[i] = createNewPlayer(params->playerPaths[i]);
    }

    initializeBoard((*gameState)->board, params->width, params->height, params->seed);
    return shm_state_fd;
}

void forkPlayers(GameState *gameState, char playerPaths[MAX_PLAYERS][50] , int pipesFD[MAX_PLAYERS][2]){
    for (int i = 0; i < gameState->cantPlayers; i++){
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
            printf("Player %d started with pid %d\n",i , getpid());
            for (int j = 0; j < i; j++) {
                close(pipesFD[j][0]);
                close(pipesFD[j][1]);
            }
            char *args[4];
            char aux[50];
            strcpy(aux, playerPaths[i]);
            args[0] = basename(aux); // nombre del ejecutable
            char widthAux[10], heightAux[10];
            snprintf(widthAux, sizeof(widthAux), "%d", gameState->width);
            snprintf(heightAux, sizeof(heightAux), "%d", gameState->height);
            args[1] = widthAux;
            args[2] = heightAux;
            args[3] = NULL;
            close(pipesFD[i][0]);
            dup2(pipesFD[i][1], STDOUT_FILENO);
            close(pipesFD[i][1]);
            
            int error = execve(playerPaths[i], args, NULL);
            if (error == -1){
                perror("player execve failed");
                exit(EXIT_FAILURE);
            }
        }
        else {
            close(pipesFD[i][1]);
        }
    }
}

int forkView(char* viewPath, GameState* gameState){
    int viewPid = fork();
    if (viewPid == -1){
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (viewPid == 0){
        char *args[4];
        char aux[50];
        strcpy(aux, viewPath);
        args[0] = basename(aux); // nombre del ejecutable
        char widthAux[10], heightAux[10];
        snprintf(widthAux, sizeof(widthAux), "%d", gameState->width);
        snprintf(heightAux, sizeof(heightAux), "%d", gameState->height);
        args[1] = widthAux;
        args[2] = heightAux;
        args[3] = NULL;
        int error = execve(viewPath, args, NULL);
        if (error == -1){
            perror("view execve failed");
            exit(EXIT_FAILURE);
        }
    }
    return viewPid;
}

int prepareFDSet(fd_set* set, bool block[MAX_PLAYERS], int cantPLayers,int fds[MAX_PLAYERS][2]) {
    FD_ZERO(set);           
    int maxFD= -1;

    for (size_t i = 0; i < cantPLayers; i++){
        if (!block[i]) {
            FD_SET(fds[i][0], set);
            if (fds[i][0] > maxFD){
                maxFD = fds[i][0];
            }
        }
    }
    
    return maxFD;
}
//Si se agregan mas SHM, tambien cambiar el define de SHM_QUANT
void initializeSHM(GameState **gameState, Parameters *params, Semaphores **semaphores, int SHMfds[SHM_QUANT]) {
    SHMfds[0] = initializeGameState(gameState, params);
    SHMfds[1] = initializeSemaphores(semaphores);
}

int readMoves(int moves[MAX_PLAYERS],fd_set *fdSet,int maxFD, int timeout, int pipes[MAX_PLAYERS][2], int cantPlayers){
    struct timeval tv;
    tv.tv_sec = timeout;
    tv.tv_usec = 0;  

    int toReturn = select(maxFD +1, fdSet, NULL, NULL, &tv);
    if (toReturn <=0){
        return toReturn;
    }
    
    for (size_t i = 0; i < cantPlayers; i++){
        if (FD_ISSET(pipes[i][0], fdSet)){
            unsigned char move;
            read(pipes[i][0], &move, 1);
            moves[i] = (int)move;
        }
        else{
            moves[i] = NOT_MOVED; // no se movio
        }
    }
    return toReturn;
}

void closeSemaphores(Semaphores* semaphores, int cantPlayers){ 
    sem_destroy(&semaphores->readyToPrint);
	sem_destroy(&semaphores->finishedPrinting);
	sem_destroy(&semaphores->turnstile);
	sem_destroy(&semaphores->readWriteMutex);
	sem_destroy(&semaphores->cantReadersMutex);
    for(int i = 0; i < cantPlayers; i++){
		sem_destroy(&semaphores->playerTurn[i]);
	}
}

void closeSharedMemories(GameState* gameState, Semaphores* semaphores, int SHMfds[SHM_QUANT]){
    closeSHM(gameState, sizeof(GameState) + gameState->width * gameState->height * sizeof(int), SHMfds[0], SHM_STATE_NAME);

	closeSHM(semaphores, sizeof(Semaphores), SHMfds[1], SHM_SEMAPHORES_NAME);
}

void closePipes(int pipes[MAX_PLAYERS][2], int cantPlayers){
    for (size_t i = 0; i < cantPlayers; i++){
        close(pipes[i][0]);
    }
}

void waitChilds(GameState *gameState, int viewFlag, int viewPid){
    int status;
    for (size_t i = 0; i < gameState->cantPlayers; i++){
        printf("Waiting for player %s with pid %d\n", gameState->players[i].name, gameState->players[i].pid);
        waitpid(gameState->players[i].pid, &status, 0);
        printf("%s exited with code %d, and scored %d with %d valid moves and %d invalid moves \n", gameState->players[i].name, WEXITSTATUS(status), gameState->players[i].score, gameState->players[i].validMoves, gameState->players[i].invalidMoves);
    }
    if (viewFlag != 0){
        waitpid(viewPid, &status, 0);
        printf("View exited with code %d\n", WEXITSTATUS(status));
    }
    
}

int main (int argc, char *argv[]) {
    GameState* gameState;
    Semaphores* semaphores;
    Parameters params;
    int SHMfds[SHM_QUANT];

    if(!getParameters(argc, argv, &params)){
		return -1;
	}

    initializeSHM(&gameState, &params, &semaphores, SHMfds);

    int pipesFD[MAX_PLAYERS][2];
    forkPlayers(gameState, params.playerPaths, pipesFD);
    int viewPid;
    if (params.viewFlag != 0){
        viewPid=forkView(params.view,gameState);
    }

    fd_set readablePipes;
    int maxFD;
    bool block[MAX_PLAYERS];         // indica si hay que bloquear al jugador i
    int landingSquares[MAX_PLAYERS]; // index de board[] donde el jugador desea moverse (no se verifica si hay un jugador dentro porque podria haber una colision entre jugadores)
    int moves[MAX_PLAYERS];            // movimiento que desea hacer el jugador i (-1 si no se mueve, 0-8 para moverse en alguna direccion)
    int readyPipes;

    //Loop de juego
    while (!gameState->gameFinished) {
        checkBlockedPlayers(block, gameState);
        maxFD = prepareFDSet(&readablePipes, block, gameState->cantPlayers,pipesFD);
        readyPipes= readMoves(moves, &readablePipes, maxFD ,params.timeout, pipesFD,gameState->cantPlayers); 
        if (readyPipes == -1){
            perror("select");
            break;
        }
        else 
        
        validateMoves(moves ,gameState); // verifies the validity of the moves
        calculateNextPosition(landingSquares, moves, gameState); // calculates the next moves without an advantage for any player
        sem_wait(&semaphores->turnstile);
        sem_wait(&semaphores->readWriteMutex);
        
        // Zona critica: Modificar el estado del juego
        for (int i=0; i < gameState->cantPlayers;i++){
            if (block[i]) {
                printf("Player %s is blocked and cannot move\n", gameState->players[i].name);
                gameState->players[i].isBlocked=true;
                sem_post(&semaphores->playerTurn[i]);
                continue;
            }
            if (moves[i]!= NOT_MOVED) { 
                if (moves[i] == INVALID_MOVE || spaceOccupied(landingSquares[i], gameState)){
                    gameState->players[i].invalidMoves++;
                    sem_post(&semaphores->playerTurn[i]);
                    continue;
                }
                movePlayer(i, landingSquares[i], gameState);
                gameState->players[i].validMoves++;
                sem_post(&semaphores->playerTurn[i]); // Le indico al jugador que puede volver a mover
            }
        }
        
        


        sem_post(&semaphores->readWriteMutex);
        sem_post(&semaphores->turnstile);

        
        //Semaforos para notificar a la vista que hay cambios
        if (params.viewFlag != 0){
           sem_post(&semaphores->readyToPrint);
           sem_wait(&semaphores->finishedPrinting);
        }
        if (readyPipes == 0){
            printf("Timeout reached. Ending game.\n");
            gameState->gameFinished= true;
            for (size_t i = 0; i < gameState->cantPlayers; i++){
                sem_post(&semaphores->playerTurn[i]);
            }
        }
        
    }
    waitChilds(gameState, params.viewFlag, viewPid);
    closeSemaphores(semaphores, gameState->cantPlayers);
	closePipes(pipesFD, gameState->cantPlayers);
    closeSharedMemories(gameState, semaphores, SHMfds);
	
    
}