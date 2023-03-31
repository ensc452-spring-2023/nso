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

#define DEBUG 1

/*--------------------------------------------------------------*/
/* Include Files				 								*/
/*--------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include "graphics.h"
#include "ff.h"
#include "xil_printf.h"
#include "sd.h"

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
extern int *approachCircle[NUM_A_CIRCLES];

#if DEBUG == 1
unsigned int totalBytes = 0;
#endif

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
		totalBytes += fileInfo.fsize;
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

void loadSprites() {
#if DEBUG == 1
	totalBytes = 0;
#endif

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

	for (int i = 0; i < NUM_A_CIRCLES; i++) {
		char tempPath[32];
		snprintf(tempPath, sizeof(tempPath), "Sprites\\ac\\ac%02d.bin", i);

		loadFileFromSD(tempPath, &approachCircle[i]);
	}

#if DEBUG == 1
	xil_printf("Total Size:%dBYTES\r\n", totalBytes);
#endif
}

int loadWAVEfileintoMemory(char * filename, u32 * songBuffer) {
	FRESULT result;
	FIL file_song;
	headerWave_t headerWave;
	fmtChunk_t fmtChunk;
	genericChunk_t genericChunk;
	uint n_bytes_read = 0;
	int song_counter = 0;

	result = f_mount(&FS_instance, "0:/", 1);
	if (result != 0) {
		print("Couldn't mount SD Card.\r\n");
	}

	FRESULT res = f_open(&file_song, filename, FA_READ);
	if (res != 0) {
		xil_printf("Song not found\r\n");
	} else {
		xil_printf("Loading %s!\r\n", filename);
	}

	// Read the RIFF header and do some basic sanity checks
	res = f_read(&file_song, (void*) &headerWave, sizeof(headerWave),
			&n_bytes_read);
	if (res != 0) {
		xil_printf("Failed to read file\r\n");
	}
	if (memcmp(headerWave.riff, "RIFF", 4) != 0) {
		xil_printf("Illegal file format. RIFF not found\r\n");
	}
	if (memcmp(headerWave.wave, "WAVE", 4) != 0) {
		xil_printf("Illegal file format. WAVE not found\r\n");
	}

	// Walk through the chunks and interpret them
	int progress_bar_counter = 0;

	for (;;) {

		res = f_read(&file_song, (void*) &genericChunk, sizeof(genericChunk),
				&n_bytes_read);
		if (res != 0) {
			xil_printf("Failed to read file\r\n");
		} else {
			if (progress_bar_counter == 1) {
				xil_printf("*");
				progress_bar_counter = 0;
			}
			progress_bar_counter++;

		}
		if (n_bytes_read != sizeof(genericChunk)) {
			break; // probably EOF
		}

		//xil_printf("%c%c%d%c", genericChunk.ckId[0], genericChunk.ckId[1], genericChunk.ckId[2], genericChunk.ckId[3]);

		// The "fmt " is compulsory and contains information about the sample format
		if (memcmp(genericChunk.ckId, "fmt ", 4) == 0) {
			res = f_read(&file_song, (void*) &fmtChunk, genericChunk.cksize,
					&n_bytes_read);
			if (res != 0) {
				xil_printf("Failed to read file\r\n");
			}
			if (n_bytes_read != genericChunk.cksize) {
				xil_printf("EOF reached\r\n");
			}
			if (fmtChunk.wFormatTag != 1) {
				xil_printf("Unsupported format\r\n");
			}
			if (fmtChunk.nChannels != 2) {
				xil_printf("Only stereo files supported\r\n");
			}
			if (fmtChunk.wBitsPerSample != 16) {
				xil_printf("Only 16 bit per samples supported\r\n");
			}
		} else if (memcmp(genericChunk.ckId, "data", 4) == 0) {
			//theBuffer = malloc(genericChunk.cksize);
			//if (!theBuffer){
			//	xil_printf("Could not allocate memory");
			//}
			size_t theBufferSize = genericChunk.cksize;

			res = f_read(&file_song, (void*) songBuffer + song_counter,
					theBufferSize, &n_bytes_read);
			song_counter += theBufferSize;

			if (res != 0) {
				xil_printf("Failed to read file");
			}
			if (n_bytes_read != theBufferSize) {
				xil_printf("Didn't read the complete file");
			} else {
				DWORD fp = f_tell(&file_song);
				f_lseek(&file_song, fp + genericChunk.cksize);
			}
		}

	}

	f_close(&file_song);

	f_unmount("0:/");

	u32 *pSource = (u32*) songBuffer;
	for (int i = 0; i < song_counter; ++i) {
		short left  = (short) ((pSource[i]>>16) & 0xFFFF);
		short right = (short) ((pSource[i]>> 0) & 0xFFFF);
		int left_i = (int)left;
		if (left_i>32767) left_i = 32767;
		if (left_i<-32767) left_i = -32767;
		if (right>32767) right = 32767;
		if (right<-32767) right = -32767;
		left = 0;
		pSource[i] = ((u32)right<<16) + (u32)left;
	}

	return song_counter;
}
