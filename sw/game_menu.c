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

static HitObject *gameHitobjects;


/*-------------------------------------------/
 / void main_menu()
 /--------------------------------------------/
 / main menu of UART osu
 /-------------------------------------------*/
void main_menu()
{
	DrawMenu();
	char input = ' ';

	while (true) {
		xil_printf("#################\n\r");
		xil_printf("Play \t (p)\n\r");
		xil_printf("Settings (s)\n\r");
		xil_printf("Quit \t (q)\n\r");
		xil_printf("#################\n\r");

		scanf(" %c", &input);

		switch (input) {
		case 'p':
			song_select_menu();
			break;
		case 's':
			settings_menu();
			break;
		case 'q':
			xil_printf("EXITING APPLICATION\r\n");
			return;
		default:
			break;
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

	while (true) {
		xil_printf("Select Song\n\r");

		for (int i = 0; i < filesNum; ++i) {
			xil_printf("(%d)\t%s\n\r", i + 1, files[i]);
		}

		xil_printf("(b)\tBack\n\r");
		xil_printf("#################\n\r");

		scanf(" %c", &input);
		int selection = 0;

		switch (input) {
		case '1':
			selection = 1;
			break;
		case '2':
			selection = 2;
			break;
		case '3':
			selection = 3;
			break;
		case '4':
			selection = 4;
			break;
		case '5':
			selection = 5;
			break;
		case 'b':
			return;
		default:
			continue;
		}

		if (selection < 1 || selection > filesNum) {
			xil_printf("Invalid Choice.\r\n");
			return;
		} else {
			xil_printf("Selected Song %d\r\n", selection);
		}
		gameHitobjects = parse_beatmaps(files[selection - 1], FS_instance);

		while (true) {
			play_game(gameHitobjects);

			if (highscore_menu() == 0)
				break;
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
	char input = ' ';

	

	while (true) {
		xil_printf("Volume:%d\t\t(1)(2)\n\r", volume);
		xil_printf("Music Offset:%dms\t(3)(4)\n\r", beatoffset);
		xil_printf("Back\t\t\t(b)\n\r");
		xil_printf("#################\n\r");

		scanf(" %c", &input);

		switch (input) {
		case '1':
			if (volume < 10) {
				volume++;
			}
			break;
		case '2':
			if (volume > 0) {
				volume--;
			}
			break;
		case '3':
			if (beatoffset < 100) {
				beatoffset += 5;
			}
			break;
		case '4':
			if (beatoffset > -100) {
				beatoffset -= 5;
			}
			break;
		case 'b':
			return;
		default:
			break;
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
	DrawStats(score, maxCombo, accuracy);
	char input = ' ';

	while (true) {
		xil_printf("Play again? \t(p)\n\r");
		xil_printf("Song Select \t(b)\n\r");
		xil_printf("#################\n\r");

		scanf(" %c", &input);

		switch (input) {
		case 'p':
			return 1;
			break;
		case 'b':
			return 0;
			break;
		default:
			break;
		}
	}
}
