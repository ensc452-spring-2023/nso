/*-----------------------------------------------------------------------------/
 /	!nso - Game Logic														   /
 /-----------------------------------------------------------------------------/
 /	William Zhang
 /	04/03/2023
 /	game_logic.h
 /
 /	Contains all the code regarding game logic of the game.
 /----------------------------------------------------------------------------*/

#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

/*--------------------------------------------------------------*/
/* Include Files												*/
/*--------------------------------------------------------------*/
#include <stdbool.h>
#include "beatmap_parser.h"

/*--------------------------------------------------------------*/
/* Definitions													*/
/*--------------------------------------------------------------*/
#define DRAWN_OBJECTS_MAX 20 // manually init array to -1 in play_game (change to linked list soon)
#define CLOCK_HZ 1000

/*--------------------------------------------------------------*/
/* Function Prototypes											*/
/*--------------------------------------------------------------*/
void game_tick();
void play_game(HitObject *gameHitobjectsIn);
void CheckObjectCollision(bool press, bool release);

#endif
