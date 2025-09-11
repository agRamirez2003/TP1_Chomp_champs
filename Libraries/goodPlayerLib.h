#ifndef GOOD_PLAYER_LIB_H
#define GOOD_PLAYER_LIB_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdbool.h>
#include "gamelib.h"
#include "queuelib.h"

unsigned char chooseGoodMove(GameState *gameState, int boardWidth, int boardHeight, int playerNum);

#endif