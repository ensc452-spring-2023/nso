/*----------------------------------------------------------------------------/
 /  !nso - SD                                                          /
 /-----------------------------------------------------------------------------/
 /	William Zhang
 /	Bowie Gian
 / 	05/03/2023
 /	sd.c
 /
 /	Helper functions for loading stuff from sd card
 /----------------------------------------------------------------------------*/

#define DEBUG 0

/*--------------------------------------------------------------*/
/* Include Files				 								*/
/*--------------------------------------------------------------*/
#include <stdlib.h>
#include "ff.h"
#include "xil_printf.h"

/*--------------------------------------------------------------*/
/* Global Variables				 								*/
/*--------------------------------------------------------------*/
extern FATFS FS_instance;
extern int *imageMenu;
extern int *imageBg;
extern int *imageCircle;
extern int *imageCircleOverlay;
extern int *spinner;
extern int *imageRanking;
extern int *imageNum[10];

/* -------------------------------------------/
 * loadFileFromSD()
 * -------------------------------------------/
 * Loads a file from the sd card and puts it
 * on to the heap/
 * ------------------------------------------*/

void loadFileFromSD(char * filename, int ** address) {
	FRESULT result;
	FIL fileToLoad;
	FILINFO fileInfo;

	//file size in bytes
	int fileSize = 0;

	//Mounts SD Card
	result = f_mount(&FS_instance, "0:/", 1);
	if (result != 0) {
		xil_printf("ERROR: Couldn't mount SD Card.\r\n");
		return;
	}

	//Checks file size and exists
	result = f_stat(filename, &fileInfo);
	if (result == FR_OK) {
		#if DEBUG == 1
				xil_printf("Loading %s \tSize:%dBYTES\r\n", fileInfo.fname,
						fileInfo.fsize);
		#endif
		fileSize = fileInfo.fsize;
	} else if (result == FR_NO_FILE) {
		xil_printf("ERROR: File not found (f_stat)\r\n");
		return;
	}

	//Opens File as Read Only
	result = f_open(&fileToLoad, filename, FA_READ);
	if (result != 0) {
		xil_printf("ERROR: File not found (f_open)\r\n");
		return;
	}

	//mallocs space for the file
	*address = (int *) malloc(fileSize);
	if (*address == NULL) {
		xil_printf("ERROR: Could not malloc space for file.\r\n");
	}

	//reads file into memory
	unsigned int numOfBytesRead;
	f_read(&fileToLoad, *address, fileSize, &numOfBytesRead);
	if (numOfBytesRead != fileSize) {
		xil_printf("ERROR: File read not different amount of bytes.\r\n");
	}

	//close file
	f_close(&fileToLoad);

}


void loadSprites()
{
  	loadFileFromSD("Sprites\\hc.bin", &imageCircle);
  	loadFileFromSD("Sprites\\hco.bin", &imageCircleOverlay);
  	loadFileFromSD("Sprites\\spin.bin", &spinner);
  	loadFileFromSD("Sprites\\num0.bin", &imageNum[0]);
  	loadFileFromSD("Sprites\\num1.bin", &imageNum[1]);
  	loadFileFromSD("Sprites\\num2.bin", &imageNum[2]);
  	loadFileFromSD("Sprites\\num3.bin", &imageNum[3]);
  	loadFileFromSD("Sprites\\num4.bin", &imageNum[4]);
  	loadFileFromSD("Sprites\\num5.bin", &imageNum[5]);
  	loadFileFromSD("Sprites\\num6.bin", &imageNum[6]);
  	loadFileFromSD("Sprites\\num7.bin", &imageNum[7]);
  	loadFileFromSD("Sprites\\num8.bin", &imageNum[8]);
  	loadFileFromSD("Sprites\\num9.bin", &imageNum[9]);
  	loadFileFromSD("Sprites\\rank.bin", &imageRanking);
  	loadFileFromSD("Sprites\\menu.bin", &imageMenu);
  	loadFileFromSD("Sprites\\bgG.bin", &imageBg);
}

