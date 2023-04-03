/*----------------------------------------------------------------------------/
 /  !nso - Game Menu                                                          /
 /-----------------------------------------------------------------------------/
 /	William Zhang
 / 	04/03/2023
 /	game_menu.c
 /
 /	Generates a UART menu for Milestone 1
 /----------------------------------------------------------------------------*/

/*--------------------------------------------------------------*/
/* Include Files				 								*/
/*--------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ff.h"
#include "game_menu.h"
#include "game_logic.h"
#include "xil_printf.h"
#include "graphics.h"
#include "cursor.h"

/*--------------------------------------------------------------*/
/* Definitions					 								*/
/*--------------------------------------------------------------*/
#define maxFiles 5
#define maxFilenameSize 128

/*--------------------------------------------------------------*/
/* Global Variables				 								*/
/*--------------------------------------------------------------*/
extern FATFS FS_instance;
extern int volume;
extern int beatoffset;
extern int *image_buffer_pointer;
extern int *image_mouse_buffer;
extern int accuracy;
extern int score;
extern int maxCombo;

static bool isPress = false;
static bool isRelease = false;
static HitObject *gameHitobjects;
static int currScreen = 0;
#define MENU		1
#define SONGS		2
#define GAME		3
#define SCORE		4
#define SETTINGS	5

/*-------------------------------------------/
 / void main_menu()
 /--------------------------------------------/
 / main menu of UART osu
 /-------------------------------------------*/
void main_menu()
{
	currScreen = MENU;

	while (true) {
		DrawMenu();
		DisplayBufferAndMouse(getMouseX(), getMouseY());

		if (isPress) {
			isPress = false;
			if (RectCollision(1050, 350, 550, 150))
				song_select_menu();
			else if (RectCollision(1050, 565, 550, 150))
				settings_menu();
		}
	}
}

/*-------------------------------------------/
 / song_select_menu()
 /--------------------------------------------/
 / song selection menu, mounts sd card, reads
 / all top 5 .osu files and displays to
 / UART
 /-------------------------------------------*/
void song_select_menu()
{
	currScreen = SONGS;

	char input = ' ';
	DIR dir;
	FRESULT result;
	static char files[maxFiles][maxFilenameSize] = { 0 };
	int filesNum = 0;

	result = f_mount(&FS_instance, "0:/", 1);
	if (result != 0) {
		xil_printf("Couldn't mount SD Card.\r\n");
		return;
	}

	result = f_opendir(&dir, "0:/");
	if (result != FR_OK) {
		xil_printf("Couldn't read root directory. \r\n");
		return;
	}

	while (1) {
		FILINFO fileinfo;
		result = f_readdir(&dir, &fileinfo);
		if (result != FR_OK || fileinfo.fname[0] == 0) {
			break;
		}
		if (fileinfo.fattrib & AM_DIR) {                 // It's a directory
		} else if (strstr(fileinfo.fname, ".osu") != NULL
			|| strstr(fileinfo.fname, ".OSU") != NULL) { // It's a beatmap file
			strcpy(files[filesNum++], fileinfo.fname);
		}
	}
	f_closedir(&dir);

	if (filesNum == 0) {
		xil_printf("No beatmaps found.\r\n");
		return;
	}

	xil_printf("Select Song\n\r");

	for (int i = 0; i < filesNum; ++i) {
		xil_printf("(%d)\t%s\n\r", i + 1, files[i]);
	}

	while (true) {
		DrawSongs();
		DisplayBufferAndMouse(getMouseX(), getMouseY());

		int selection = 0;

		if (isPress) {
			if (RectCollision(460, 25, 1000, 150))
				selection = 1;
			else if (RectCollision(460, 225, 1000, 150))
				selection = 2;
			else if (RectCollision(460, 425, 1000, 150))
				selection = 3;
			else if (RectCollision(460, 625, 1000, 150))
				selection = 4;
			else if (RectCollision(460, 825, 1000, 150))
				selection = 5;
			else if (RectCollision(0, 0, 300, 150))
				return;
			else
				continue;

			if (selection < 1 || selection > filesNum) {
				xil_printf("Invalid Choice.\r\n");
				return;
			} else {
				xil_printf("Selected Song %d\r\n", selection);
			}
			gameHitobjects = parse_beatmaps(files[selection - 1], FS_instance);

			while (true) {
				currScreen = GAME;
				play_game(gameHitobjects);

				if (highscore_menu() == 0)
					break;
			}
		}
	}
}

/*-------------------------------------------/
 / void settings_menu()
 /--------------------------------------------/
 / settings menu, manipulates gloal game
 / setting variables like volume and game
 / offset
 /-------------------------------------------*/
void settings_menu()
{
	currScreen = SETTINGS;

	while (true) {
		DrawSettings(volume, beatoffset);
		DisplayBufferAndMouse(getMouseX(), getMouseY());

		if (isPress) {
			isPress = false;
			if (RectCollision(1160, 350, 200, 100)) {
				if (volume < 10)
					volume++;
			} else if (RectCollision(560, 350, 200, 100)) {
				if (volume > 0)
					volume--;
			} else if (RectCollision(1160, 670, 200, 100)) {
				if (beatoffset < 500)
					beatoffset += 5;
			} else if (RectCollision(560, 670, 200, 100)) {
				if (beatoffset > 0)
					beatoffset -= 5;
			} else if (RectCollision(0, 0, 300, 150)) {
				return;
			}
		}
	}
}

/*-------------------------------------------/
 / void highscore_menu()
 /--------------------------------------------/
 / Literally does so little
 /-------------------------------------------*/
int highscore_menu()
{
	currScreen = SCORE;
	DrawScores(score, maxCombo, accuracy);

	while (true) {
		DrawScores(score, maxCombo, accuracy);
		DisplayBufferAndMouse(getMouseX(), getMouseY());

		if (isPress) {
			isPress = false;
			if (RectCollision(0, 930, 500, 150))
				return 1;
			if (RectCollision(0, 0, 300, 150))
				return 0;
		}
	}
}

void menu_collision(bool press, bool release)
{
	if (press)
		isPress = true;

	if (release)
		isRelease = true;

	switch (currScreen) {
	case MENU:

		break;
	case SONGS:

		break;
	case GAME:
		CheckObjectCollision(press, release);
		break;
	case SCORE:

		break;
	case SETTINGS:

		break;
	}
}
