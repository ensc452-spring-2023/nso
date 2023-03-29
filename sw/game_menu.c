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

/*--------------------------------------------------------------*/
/* Global Variables				 								*/
/*--------------------------------------------------------------*/
extern FATFS FS_instance;
extern int volume;
extern int beatoffset;
extern int *image_buffer_pointer;
extern int *image_mouse_buffer;

int score = 0;

/*-------------------------------------------/
 / void main_menu()
 /--------------------------------------------/
 / main menu of UART osu
 /-------------------------------------------*/
void main_menu()
{
	char input = ' ';

	xil_printf("#################\n\r");
	xil_printf("Play \t (p)\n\r");
	xil_printf("Settings (s)\n\r");
	xil_printf("Black    (d)\n\r");
	xil_printf("Quit \t (q)\n\r");
	xil_printf("#################\n\r");

	scanf(" %c", &input);

	switch (input)
	{
	case 'p':
		song_select_menu();
		break;
	case 's':
		settings_menu();
		break;
	case 'd':
		while(1)
		{
			memset((int*) 0x2000000, 0x1F1F1F, NUM_BYTES_BUFFER);
			DrawMouse(getMouseX(), getMouseY());
		}
		//SetPixel(image_output_pointer + ((VGA_WIDTH*200)+(100)), 0xFFFFFF);
		//SetPixel(image_output_pointer + ((VGA_WIDTH*200)+(200)), 0x00FF00);
		//SetPixel(temp_pointer + (VGA_WIDTH *100), 0x00FF00);
		break;
	case 'q':
		xil_printf("EXITING APPLICATION (but not really)\r\n");
		break;
	default:
		main_menu();
		break;
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
	static char files[maxFiles][32] =
	{ 0 };
	int filesNum = 0;

	xil_printf("Select Song\n\r");

	result = f_mount(&FS_instance, "0:/", 1);
	if (result != 0)
	{
		xil_printf("Couldn't mount SD Card.\r\n");
		return;
	}

	result = f_opendir(&dir, "0:/");
	if (result != FR_OK)
	{
		xil_printf("Couldn't read root directory. \r\n");
		return;
	}

	while (1)
	{
		FILINFO fileinfo;
		result = f_readdir(&dir, &fileinfo);
		if (result != FR_OK || fileinfo.fname[0] == 0)
		{
			break;
		}
		if (fileinfo.fattrib & AM_DIR)
		{                 // It's a directory
		}
		else if (strstr(fileinfo.fname, ".osu") != NULL
				|| strstr(fileinfo.fname, ".OSU") != NULL)
		{ // It's a beatmap file
			strcpy(files[filesNum++], fileinfo.fname);
		}
	}
	f_closedir(&dir);

	if (filesNum == 0)
	{
		xil_printf("No beatmaps found.\r\n");
		return;
	}

	for (int i = 0; i < filesNum; ++i)
	{
		xil_printf("%s \t (%d)\n\r", files[i], i + 1);
	}

	xil_printf("Back  \t\t (b)\n\r");
	xil_printf("#################\n\r");

	scanf(" %c", &input);

	switch (input)
	{
	case '1':
		if (filesNum < 1)
		{
			xil_printf("Invalid Choice.\r\n");
			return;
		}
		else
		{
			xil_printf("Selected Song 1\r\n");
		}
		parse_beatmaps(files[0], FS_instance);
		play_game();
		highscore_menu();
		break;
	case '2':
		if (filesNum < 2)
		{
			xil_printf("Invalid Choice.\r\n");
			return;
		}
		else
		{
			xil_printf("Selected Song 2\r\n");
		}
		parse_beatmaps(files[1], FS_instance);
		play_game();
		highscore_menu();
		break;
	case '3':
		if (filesNum < 3)
		{
			xil_printf("Invalid Choice.\r\n");
			return;
		}
		else
		{
			xil_printf("Selected Song 3\r\n");
		}
		parse_beatmaps(files[2], FS_instance);
		play_game();
		highscore_menu();
		break;
	case '4':
		if (filesNum < 4)
		{
			xil_printf("Invalid Choice.\r\n");
			return;
		}
		else
		{
			xil_printf("Selected Song 4\r\n");
		}
		parse_beatmaps(files[3], FS_instance);
		play_game();
		highscore_menu();
		break;
	case '5':
		if (filesNum < 5)
		{
			xil_printf("Invalid Choice.\r\n");
			return;
		}
		else
		{
			xil_printf("Selected Song 5\r\n");
		}
		parse_beatmaps(files[4], FS_instance);
		play_game();
		highscore_menu();
		break;
	case 'b':
		return;
		break;
	default:
		return;
		break;
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

	xil_printf("Volume:%d\t(1)(2)\n\r", volume);
	xil_printf("Music Offset:%d\t(3)(4)\n\r", beatoffset);
	xil_printf("Back\t\t(b)\n\r");
	xil_printf("#################\n\r");

	scanf(" %c", &input);

	switch (input)
	{
	case '1':
		if (volume < 10)
		{
			volume++;
		}
		settings_menu();
		break;
	case '2':
		if (volume > 0)
		{
			volume--;
		}
		settings_menu();
		break;
	case '3':
		if (beatoffset < 5)
		{
			beatoffset++;
		}
		settings_menu();
		break;
	case '4':
		if (beatoffset > -5)
		{
			beatoffset--;
		}
		settings_menu();
		break;
	case 'b':
		return;
		break;
	default:
		main_menu();
		break;
	}
}

/*-------------------------------------------/
 / void highscore_menu()
 /--------------------------------------------/
 / Literally does so little
 /-------------------------------------------*/
void highscore_menu()
{
	DrawStats(score);
	xil_printf("Score:%d\n\r", score);
	xil_printf("Accuracy:%c\n\r", '-');
	xil_printf("Play again? \t(p)\n\r");
	xil_printf("Main Menu \t(m)\n\r");
	xil_printf("#################\n\r");

	char input = ' ';
	scanf(" %c", &input);

	switch (input)
	{
	case 'p':
		play_game();
		highscore_menu();
		break;
	case 'm':
		break;
	default:
		main_menu();
		break;
	}
}
