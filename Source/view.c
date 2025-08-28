#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>


#define MAX_PLAYERS 9

#define SHM_STATE_NAME "/game_state"
#define SHM_SEMAPHORES_NAME "/game_sync"


//Colores para los distintos jugadores
static const char *colors[MAX_PLAYERS] = {
    "\033[31m", // Rojo
    "\033[32m", // Verde
    "\033[33m", // Amarillo
    "\033[34m", // Azul
    "\033[35m", // Magenta
    "\033[36m", // Cyan
    "\033[37m", // Blanco
    "\033[90m", // Gris
    "\033[91m"  // Rojo claro
};

typedef struct {
char playerName[16]; // Nombre del jugador
unsigned int score; // Puntaje
unsigned int invalidMoves; // Cantidad de solicitudes de movimientos inválidas realizadas
unsigned int validMoves; // Cantidad de solicitudes de movimientos válidas realizadas
unsigned short x, y; // Coordenadas x e y en el tablero
pid_t pid; // Identificador de proceso
bool isBlocked; // Indica si el jugador está bloqueado
} Player;



typedef struct {
unsigned short width; // Ancho del tablero
unsigned short height; // Alto del tablero
unsigned int cantPlayers; // Cantidad de jugadores
Player players[MAX_PLAYERS]; // Lista de jugadores
bool gameFinished; // Indica si el juego se ha terminado
int board[]; // Puntero al comienzo del tablero. fila-0, fila-1, ..., fila-n-1
} GameState;


typedef struct {
sem_t readyToPrint; // El máster le indica a la vista que hay cambios por imprimir      ]
sem_t finishedPrinting; // La vista le indica al máster que terminó de imprimir         ] Solo me interesan estos 2 
sem_t turnstile; // Mutex para evitar inanición del máster al acceder al estado
sem_t readWriteMutex; // Mutex para el estado del juego
sem_t cantReadersMutex; // Mutex para la siguiente variable
unsigned int cantReading; // Cantidad de jugadores leyendo el estado
sem_t playerTurn[MAX_PLAYERS]; // Le indican a cada jugador que puede enviar 1 movimiento
} Semaphores;



//

static void clearScreen(){
    printf("\033[2J\033[H");
    fflush(stdout); // por si me queda en el buffer
}



static void showPlayers(const GameState *g) {
    printf("=== JUGADORES ===\n");
    for (int i = 0; i < g->cantPlayers; i++) {
        Player p = g->players[i];
        printf("Jugador %d: %s\n", i, p.playerName);
        printf("  Puntaje: %u\n", p.score);
        printf("  Posición: (%hu, %hu)\n", p.x, p.y);
        printf("  Movimientos válidos: %u\n", p.validMoves);
        printf("  Movimientos inválidos: %u\n", p.invalidMoves);
        printf("  Estado: %s\n", p.isBlocked ? "BLOQUEADO" : "ACTIVO");
        printf("---\n");
    }
}           

static void showGameState(const GameState *g, const char *colors[MAX_PLAYERS]) {
    clearScreen();
    printf("=== ESTADO ===\n");
    printf("Dimensiones: %hu x %hu\n", g->width, g->height);
    printf("Jugadores: %u\n", g->cantPlayers);
    printf("Terminado: %s\n", g->gameFinished ? "sí" : "no");
    showPlayers(g);
    showBoard(g, colors);
    printf("==============\n");

}

//revisarlo despues denuevo <*:)
void showBoard(const GameState *g, const char *colors[MAX_PLAYERS]) {
    int width = g->width;
    int height = g->height;
    const char *white = "\033[37m";
    const char *gray  = "\033[90m";
    const char *reset = "\033[0m";

    printf("=== TABLERO ===\n");

    
    printf("   ");
    for (int x = 0; x < width; x++) {
        printf("%2d ", x);
    }
    printf("\n");

    for (int y = 0; y < height; y++) {
        
        printf("%2d ", y);

        for (int x = 0; x < width; x++) {
            int v = g->board[y * width + x];

            if (v > 0) {
                // Recompensa 1-9
                printf("%s%2d%s ", white, v, reset);
            } else {
                // Celda ocupada por el jugadorr
                int owner = -v;

                if (owner >= 0 && owner < (int)g->cantPlayers && owner < MAX_PLAYERS) {
                    printf("%sJ%-1d%s ", colors[owner], owner, reset);
                } else {
                    // valor out of bound - checkear
                    printf("%s ? %s ", gray, reset);
                }
            }
        }
        printf("\n");
    }
}

//Main

int main(int argc, char *argv[]) {
    if (argc != 3){
        fprintf(stderr, "Uso debe ser: %s <width> <height>\n", argv[0]);
        return 1;
    } 

    // Abro SHM  r/w (sync)
    int fd_sync = shm_open(SHM_SEMAPHORES_NAME, O_RDWR, 0666);
    if (fd_sync == -1){
         perror("view: shm_open(sync)");
          return 1;
         }
    Semaphores *sync = mmap(NULL, sizeof(Semaphores), PROT_READ|PROT_WRITE,
                            MAP_SHARED, fd_sync, 0);
    if (sync == MAP_FAILED){
         perror("view: mmap(sync)");
          return EXIT_FAILURE; }

    // Abro SHM de estado RO
    int fd_state = shm_open(SHM_STATE_NAME, O_RDONLY, 0666);
    if (fd_state == -1){ 
        perror("view: shm_open(state)");
         return EXIT_FAILURE; }
    struct stat sb;
    if (fstat(fd_state, &sb) == -1){
         perror("view: fstat(state)");
          return 1; }
    GameState *game = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fd_state, 0);
    if (game == MAP_FAILED){
         perror("view: mmap(state)"); 
         return EXIT_FAILURE; 
        }
 
    while(1) {
     sem_wait(&sync->readyToPrint);
     showGameState(game, colors);                 // espero para printear
    if (game->gameFinished) {
        sem_post(&sync->finishedPrinting);
        break;
    }
    sem_post(&sync->finishedPrinting);
}


    
        //destroys en master?
    munmap(game, sb.st_size);      
    munmap(sync, sizeof(*sync));
    close(fd_state);
    close(fd_sync);

    return 0;
}