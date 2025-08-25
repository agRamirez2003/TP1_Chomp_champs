#include <string.h>
#include "Libraries/playerslib.h"

static Player createPlayer(char name[], unsigned int score, unsigned int invalidMoves, unsigned int validMoves, unsigned short x, unsigned short y, pid_t pid, bool isBlocked)
{
    Player toReturn;
    strcpy(toReturn.name, name);
    toReturn.score = score;
    toReturn.invalidMoves = invalidMoves;
    toReturn.validMoves = validMoves;
    toReturn.x = x;
    toReturn.y = y;
    toReturn.pid = pid;
    toReturn.isBlocked = isBlocked;
    return toReturn;
}

Player createNewPlayer(char* path)
{
    char name[NAME_LENGTH];
    getPlayerName(path, name); //TODO
    return createPlayer(name, 0, 0, 0, 0, 0, 0, false);
}