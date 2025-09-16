#define _POSIX_C_SOURCE 200112L // Si no pongo esta linea no me funciona ftrunctate ni getopt (agus)
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#include "../Libraries/playerslib.h"
#include "../Libraries/goodPlayerLib.h"
#include <limits.h>

#define MAX_PLAYERS 9
#define INFINITY INT_MAX
#define MINUS_INFINITY INT_MIN

#define POSSIBLE_MOVES 8

#define SYNC_NAME "/game_sync"
#define STATE_NAME "/game_state"

typedef struct
{
    sem_t readyToPrint;       // Notify view of changes
    sem_t finishedPrinting;   // Notify master that printing is done
    sem_t turnstile;          // Mutex to prevent starvation
    sem_t readWriteMutex;     // Game state mutex
    sem_t cantReadersMutex;   // Mutex for F
    unsigned int cantReading; // Number of players reading state
} Sync;

typedef struct {
    unsigned short width;        // Ancho del tablero
    unsigned short height;       // Alto del tablero
    unsigned int cantPlayers;    // Cantidad de jugadores
    Player players[MAX_PLAYERS]; // Lista de jugadores
    bool gameFinished;           // Indica si el juego se ha terminado
    int board[];                 // Puntero al comienzo del tablero. fila-0, fila-1, ..., fila-n-1
} GameState;


static int getMyId(GameState *gameState){
    for (size_t i = 0; i < gameState->cantPlayers ; i++)
    {
        if (getpid() == gameState->players[i].pid)
            return i;
    }
    return -1; //para que no de warning
}

void sleep_ms(int milliseconds)
{
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;

    nanosleep(&ts, NULL);
}

int main(int argc, char *argv[])
{
    srand(time(NULL));
    if (argc != 3)
    {
        fprintf(stderr, "there have to be 2 args when calling player: width and height\n");
        return 1;
    }

    // Open shared memory
    int shm_sync = shm_open(SYNC_NAME, O_RDWR, 0666);
    if (shm_sync == -1)
    {
        perror("shm_sync_open");
        return 1;
    }

    // Map shared memory into process space
    Sync *sync = mmap(NULL, sizeof(Sync), PROT_READ | PROT_WRITE, MAP_SHARED, shm_sync, 0);
    if (sync == MAP_FAILED)
    {
        perror("mmap");
        return 1;
    }

    int shm_state = shm_open(STATE_NAME, O_RDONLY, 0666);
    if (shm_state == -1)
    {
        perror("shm_state_open");
        return 1;
    }

    int boardWidth = atoi(argv[1]);
    int boardHeight = atoi(argv[2]);
    int boardSize = boardHeight * boardWidth * sizeof(int);

    // Map shared memory into process space
    GameState *gameState = mmap(NULL, sizeof(GameState) + boardSize, PROT_READ, MAP_SHARED, shm_state, 0);
    if (gameState == MAP_FAILED)
    {
        perror("mmap");
        return 1;
    }
    int id= getMyId(gameState);

    while (1)
    {
        sem_wait(&sync->playerTurn[id]);
        sem_wait(&sync->turnstile);
        sem_post(&sync->turnstile);

        sem_wait(&sync->cantReadersMutex);
        if (sync->cantReading == 0)
        {
            sem_wait(&sync->readWriteMutex);
        }
        sync->cantReading++;

        sem_post(&sync->cantReadersMutex);

        // Zone critica

        if (gameState->gameFinished)
        {
            sem_wait(&sync->cantReadersMutex);
            if (sync->cantReading == 1)
            {
                sem_post(&sync->readWriteMutex);
            }
            sync->cantReading--;
            sem_post(&sync->cantReadersMutex);
            break;
        }

        sem_wait(&sync->cantReadersMutex);
        if (sync->cantReading == 1)
        {
            sem_post(&sync->readWriteMutex);
        }
        sync->cantReading--;
        sem_post(&sync->cantReadersMutex);
		unsigned char chosenMove = chooseGoodMove(gameState, boardWidth, boardHeight, 0); //TODO poner el numero de jugador que seamos (ver como hacer)
		write(1, &chosenMove, 1);

        sleep_ms(10);
    }
    // for (int i = 0; i < height; i++) {
    // free(board[i]);
    //}
    // free(board);
    sem_destroy(&sync->cantReadersMutex);
    sem_destroy(&sync->finishedPrinting);
    sem_destroy(&sync->readWriteMutex);
    sem_destroy(&sync->readyToPrint);
    sem_destroy(&sync->turnstile);
    munmap(sync, sizeof(Sync));
    munmap(gameState,sizeof(GameState)+ boardSize);
    close(shm_sync);
    close(shm_state);
    return 0;
}

/*
ps: procesos
lsof: list open file | pipe
valgrind: chequeos de memory leaks (dinamico)
pvs studio: chequeos antes de entregarlo (estatico)
gdb para el posix el problema ese pedorro
getops
*/