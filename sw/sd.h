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

#ifndef SD_H
#define SD_H

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

#endif
