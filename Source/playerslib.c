#include <string.h>
#include "../Libraries/playerslib.h"

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

static void getPlayerName(char *path, char *output)
{
    char aux[50];
    strcpy(aux, path);
    char *name = basename(aux);
    int i = 0;
    while (aux[i] != '\0' && aux[i] != '.' && i < sizeof(output) - 1)
    {
        output[i] = name[i];
        i++;
    }
    output[i] = '\0';
}
Player createNewPlayer(char* path)
{
    char name[NAME_LENGTH];
    getPlayerName(path, name); 
    return createPlayer(name, 0, 0, 0, 0, 0, 0, false);
}