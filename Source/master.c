#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include "../Libraries/playerslib.h"

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
    // funcion para inicializar los parametros
}

void initializeSemaphores(Semaphores *semaphores) {
    //inicializa los semaforos
}

int main (int argc, char *argv[]){
    GameState gameState;
    Semaphores semaphores;
    Parameters params;

    getParameters(argc, argv, &params);

    initializeSemaphores(&semaphores);


    //Loop de juego
    while (!gameState.gameFinished)
    {

        sem_wait(&semaphores.turnstile);
        sem_wait(&semaphores.readWriteMutex);

        // Zona critica: Modificar el estado del juego

        sem_post(&semaphores.readWriteMutex);
        sem_post(&semaphores.turnstile);


    }
    
}