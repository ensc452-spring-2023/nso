/*----------------------------------------------------------------------------/
 /  !nso - Graphics	                                                          /
 /----------------------------------------------------------------------------/
 /	William Zhang
 /	Bowie Gian
 /	04/03/2023
 /	graphics.c
 /
 /	Stolen from Bowie's graphics stuff
 /---------------------------------------------------------------------------*/

#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdint.h>
/*--------------------------------------------------------------*/
/* Definitions					 								*/
/*--------------------------------------------------------------*/
#define NUM_STRIPES 8
#define VGA_WIDTH 1920
#define VGA_HEIGHT 1080
#define NUM_BYTES_BUFFER 8294400

#define DIGIT_WIDTH 47
#define DIGIT_HEIGHT 71
#define CIRCLE_WIDTH 155
#define CIRCLE_HALF 77
#define CIRCLE_RAD_SQUARED 4096 //64^2
#define SPINNER_WIDTH 689
#define SPINNER_HALF 344
#define CENTER_X 640
#define CENTER_Y 512
#define RANKING_WIDTH 660
#define RANKING_HEIGHT 700

#define PIXEL_BYTES 4

/*--------------------------------------------------------------*/
/* Functions Prototypes											*/
/*--------------------------------------------------------------*/
void SetPixel(int *pixelAddr, int colour);
void DisplayBuffer();
void DisplayBufferAndMouse();
void FillScreen(int colour);
void PixelAlpha(int *under, int *over);

// Top Left
void DrawSprite(int *sprite, int width, int height, int posX, int posY);
// Centered
void DrawSliderEnd(int x, int y);
// Centered
void DrawCircle(int x, int y);
// Centered
void DrawSpinner(int x, int y);
// Top Left
void DrawInt(unsigned int num, int length, int posX, int posY);

void DrawMenu();
void DrawGame(int score);
void DrawStats(int score);

void plotLine(int x0, int y0, int x1, int y1, int colour);
void drawline(int x0, int y0, int x1, int y1, int colour);

#endif
