/*-----------------------------------------------------------------------------/
 /	!nso - SD																   /
 /-----------------------------------------------------------------------------/
 /	William Zhang
 /	Bowie Gian
 / 	05/03/2023
 /	sd.h
 /
 /	Helper functions for loading stuff from sd card
 /----------------------------------------------------------------------------*/

/*--------------------------------------------------------------*/
/* 				Structs											*/
/*--------------------------------------------------------------*/

#include "xil_types.h"

#ifndef SD_H
#define SD_H

typedef struct {
	char riff[4];
	u32 riffSize;
	char wave[4];
} headerWave_t;

typedef struct {
	char ckId[4];
	u32 cksize;
} genericChunk_t;

typedef struct {
	u16 wFormatTag;
	u16 nChannels;
	u32 nSamplesPerSec;
	u32 nAvgBytesPerSec;
	u16 nBlockAlign;
	u16 wBitsPerSample;
	u16 cbSize;
	u16 wValidBitsPerSample;
	u32 dwChannelMask;
	u8 SubFormat[16];
} fmtChunk_t;

/*--------------------------------------------------------------*/
/* Definitions													*/
/*--------------------------------------------------------------*/

#define maxAudioFilenameSize 128

/*--------------------------------------------------------------*/
/* Function Prototypes											*/
/*--------------------------------------------------------------*/

/* -------------------------------------------/
 * loadFileFromSD()
 * -------------------------------------------/
 * Loads a file from the sd card and puts it
 * on to the heap
 * ------------------------------------------*/
void loadFileFromSD (char * filename, int** address);
void loadSprites();
int loadWAVEfileintoMemory(char * filename, u32 * songBuffer);

#endif
