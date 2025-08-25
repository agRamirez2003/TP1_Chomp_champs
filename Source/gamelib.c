#include "../Libraries/gamelib.h"
#include "../Libraries/playerslib.h"

void initializeBoard(int* board, int width, int height, int seed) {
    srand(seed);
    for (int i = 0; i < width * height; i++) {
        board[i] = rand() % 10; 
    }
}