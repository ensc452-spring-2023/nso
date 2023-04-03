/*----------------------------------------------------------------------------/
 /  !nso - Game Menu                                                          /
 /-----------------------------------------------------------------------------/
 /	William Zhang
 / 	04/03/2023
 /	game_menu.h
 /
 /	Generates a UART menu for Milestone 1
 /----------------------------------------------------------------------------*/

#include <stdbool.h>

/*--------------------------------------------------------------*/
/* Functions Prototypes 										*/
/*--------------------------------------------------------------*/
void main_menu();
void song_select_menu();
void settings_menu();
int highscore_menu();
void menu_collision(bool press, bool release);
