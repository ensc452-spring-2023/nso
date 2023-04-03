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
#include "xil_types.h"
#include "linked_list.h"

/*--------------------------------------------------------------*/
/* Structs					 								*/
/*--------------------------------------------------------------*/

typedef struct RenderNode{
	u32 * 	address;
	u16 	size_x;			//sprite x
	u16 	size_y;			//sprite y
	u32 	size;			//in bytes
	u16		x;
	u16		y;
	u8		drawnBuffer0;
	u8		drawnBuffer1;
	u8		delete;			//request to delete
	u8		screenWidth;	//object to render is the size of the screen y
	u32		id;
} RenderNode;
/*--------------------------------------------------------------*/
/* Definitions					 								*/
/*--------------------------------------------------------------*/
#define VGA_WIDTH 1920
#define VGA_HEIGHT 1080
#define NUM_BYTES_BUFFER 8294400

#define DIGIT_WIDTH 47
#define DIGIT_HEIGHT 71
#define PERCENT_WIDTH 55
#define PERCENT_HEIGHT 71
#define COMBOX_WIDTH 35
#define COMBOX_HEIGHT 37
#define CIRCLE_WIDTH 155
#define CIRCLE_HALF 77
//#define SPINNER_WIDTH 689
//#define SPINNER_HALF 344
#define SPINNER_WIDTH 560
#define SPINNER_HALF 280
#define REVERSE_WIDTH 90
#define REVERSE_HALF 45
#define CENTER_X 960
#define CENTER_Y 540
#define RANKING_WIDTH 660
#define RANKING_HEIGHT 700

#define NUM_A_CIRCLES 66
#define A_CIRCLE_WIDTH 255
#define A_CIRCLE_HALF 127

#define PIXEL_BYTES 4

/*--------------------------------------------------------------*/
/* Functions Prototypes											*/
/*--------------------------------------------------------------*/
void SetPixel(int *pixelAddr, int colour);
void DisplayBuffer();
void DisplayBufferAndMouse(int x, int y);
void FillScreen(int colour);
void DrawRectangle(int x, int y, int width, int height, int colour);
void PixelAlpha(int *under, int *over);

// Top Left
void DrawSprite(int *sprite, int width, int height, int posX, int posY);
// Centered
void DrawSliderEnd(int x, int y);
// Centered
void DrawCircle(int x, int y);
// Centered
void DrawApproachCircle(int x, int y, int index);
// Centered
void DrawReverse(int x, int y);
// Centered
void DrawSpinner(int x, int y, int index);
// Top Left
void DrawInt(unsigned int num, int length, int posX, int posY);
// Top Left
void DrawPercent(unsigned int num, int posX, int posY);
// Top Left
void DrawCombo(unsigned int num, int posX, int posY);

void DrawMenu();
void DrawGame(int score);
void DrawStats(int score, int combo, int accuracy);

void plotLine(int x0, int y0, int x1, int y1, int colour);
void drawline(int x0, int y0, int x1, int y1, int colour);

void DrawMouse(int x, int y);

RenderNode * createRenderNode(u32 * address, u32 size, u32 id);
Node_t * addRenderNode(Node_t ** masterRenderHead, RenderNode * renderNode);
Node_t * removeRenderNodeByNode(Node_t * node);
Node_t * removeRenderNodeByRenderNode(Node_t ** masterRenderHead, RenderNode * node);
Node_t * removeRenderNodeByID(Node_t ** masterRenderHead, u32 id);

void Example_CallBack(void *CallBackRef, u32 IrqMask, int *NumBdPtr);

#endif
