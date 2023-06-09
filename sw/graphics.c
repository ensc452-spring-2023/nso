#include "graphics.h"
#include "sd.h"
#include "xil_printf.h"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "hdmi.h"
#include "linked_list.h"

#define ALPHA_THRESHOLD 100

// Note: Next buffer + 0x7E9004
int *image_output_buffer = (int *)VDMA_BUFFER_0;
int *image_mouse_buffer = (int *)0x03FD2008;
int *image_buffer_pointer = (int *)VDMA_BUFFER_1;

Node_t * masterRenderList;

// Loaded from sd card
int *imageMenu;
int *bgScores;
int *bgSettings;
int *bgSongs;
int *imageCircle;
int *imageCircleOverlay;
int *cursor;
int *spinner[2];
int *imageNum[10];
int *percent;
int *comboX;
int *approachCircle[NUM_A_CIRCLES];
int *reverse;

void SetPixel(int *pixelAddr, int colour) {
	*pixelAddr = colour;
}

void DisplayBuffer() {
	memcpy(image_output_buffer, image_buffer_pointer, NUM_BYTES_BUFFER);
}

unsigned int mouseColour = 0xFFFF00;
void DrawMouse(int x, int y) {
//	memcpy(image_mouse_buffer, image_buffer_pointer, NUM_BYTES_BUFFER);
//	*(image_mouse_buffer + x + y * VGA_WIDTH) = mouseColour;
//	*(image_mouse_buffer + x + (y+1) * VGA_WIDTH) = mouseColour;
//
//	if (x != VGA_WIDTH - 1) {
//		*(image_mouse_buffer + x+1 + y * VGA_WIDTH) = mouseColour;
//		*(image_mouse_buffer + x+1 + (y+1) * VGA_WIDTH) = mouseColour;
//	}

	//Performance 2x2 cursor
	//*(image_output_buffer + x + y * VGA_WIDTH) = mouseColour;
	//*(image_output_buffer + x + (y+1) * VGA_WIDTH) = mouseColour;

	//if (x != VGA_WIDTH - 1) {
	//	*(image_output_buffer + x+1 + y * VGA_WIDTH) = mouseColour;
	//	*(image_output_buffer + x+1 + (y+1) * VGA_WIDTH) = mouseColour;
	//}

	DrawSprite(cursor, CURSOR_WIDTH, CURSOR_WIDTH, x - CURSOR_HALF, y - CURSOR_HALF);
}

void DisplayBufferAndMouse(int x, int y) {
//	DrawMouse(x, y);
//	memcpy(image_output_buffer, image_mouse_buffer, NUM_BYTES_BUFFER);

	//Performance
	DrawMouse(x, y);
	DisplayBuffer();
}

void FillScreen(int colour) {
	memset(image_buffer_pointer, colour, NUM_BYTES_BUFFER);
}

void DrawRectangle(int x, int y, int width, int height, int colour) {
	int *end = image_buffer_pointer + x + (y + height) * VGA_WIDTH;
	for (int *temp_pointer = image_buffer_pointer + x + y * VGA_WIDTH; temp_pointer < end; temp_pointer += VGA_WIDTH) {
		memset(temp_pointer, colour, width * PIXEL_BYTES);
	}
}

// Draws a pixel on top of another considering transparency
// Referenced https://gist.github.com/XProger/96253e93baccfbf338de
//void PixelAlpha(int *under, int *over) {
//	int alpha = (*over & 0xFF000000) >> 24;
//
//	if (alpha == 0)
//		return;
//
//	if (alpha == 0xFF) {
//		*under = *over;
//		return;
//	}
//
//	unsigned int rb = *under & 0x00FF00FF;
//	unsigned int g = *under & 0x0000FF00;
//
//	rb += ((*over & 0x00FF00FF) - rb) * alpha >> 8;
//	g += ((*over & 0x0000FF00) - g) * alpha >> 8;
//
//	*under = (rb & 0x00FF00FF) | (g & 0x0000FF00);
//}

// Performance (Ignores alpha and uses a threshold)
// Tested 1st slider on Dandelions 14ms -> 10ms
void PixelAlpha(int *under, int *over) {
	int alpha = (*over & 0xFF000000) >> 24;

	if (alpha <= ALPHA_THRESHOLD)
			return;
	else {
		*under = *over;
		return;
	}
}

// Draws a sprite on top of the output frame
//void DrawSprite(int *sprite, int width, int height, int posX, int posY) {
//	int *temp_pointer = (int *)image_buffer_pointer;
//	int *sprite_pointer = (int *)sprite;
//	temp_pointer += posX + posY * VGA_WIDTH;
//
//	for (int currX = posX; currX < posX + height; currX++) {
//		for (int currY = posY; currY < posY + width; currY++) {
//			PixelAlpha(temp_pointer, sprite_pointer);
//
//			temp_pointer++;
//			sprite_pointer++;
//		}
//		temp_pointer += VGA_WIDTH - width;
//	}
//}

// Performance - much faster for hitcircles
void DrawSprite(int *sprite, int width, int height, int posX, int posY) {
	int *temp_pointer = (int *)image_buffer_pointer;
	int *sprite_pointer = (int *)sprite;
	int copySize = 0;

	temp_pointer += posX + posY * VGA_WIDTH;

	for (int currX = posX; currX < posX + height; currX++) {
		for (int currY = posY; currY < posY + width; currY++) {
			int alpha = (*(sprite_pointer + copySize) & 0xFF000000) >> 24;

			if (alpha > ALPHA_THRESHOLD) {
				copySize++;
				continue;
			}

			if (copySize > 0) {
				memcpy(temp_pointer, sprite_pointer, copySize * PIXEL_BYTES);

				temp_pointer += copySize;
				sprite_pointer += copySize;
				copySize = 0;
			}

			temp_pointer++;
			sprite_pointer++;
		}

		if (copySize > 0) {
			memcpy(temp_pointer, sprite_pointer, copySize * PIXEL_BYTES);

			temp_pointer += copySize;
			sprite_pointer += copySize;
			copySize = 0;
		}

		temp_pointer += VGA_WIDTH - width;
	}
}

void DrawSliderEnd(int x, int y) {
	int spriteX = x - CIRCLE_HALF;
	int spriteY = y - CIRCLE_HALF;

	DrawSprite(imageCircleOverlay, CIRCLE_WIDTH, CIRCLE_WIDTH, spriteX, spriteY);
}

void DrawCircle(int x, int y) {
	int spriteX = x - CIRCLE_HALF;
	int spriteY = y - CIRCLE_HALF;

	DrawSprite(imageCircle, CIRCLE_WIDTH, CIRCLE_WIDTH, spriteX, spriteY);
}

void DrawApproachCircle(int x, int y, int index) {
	int spriteX = x - A_CIRCLE_HALF;
	int spriteY = y - A_CIRCLE_HALF;

	DrawSprite(approachCircle[index], A_CIRCLE_WIDTH, A_CIRCLE_WIDTH, spriteX, spriteY);
}

void DrawReverse(int x, int y)
{
	int spriteX = x - REVERSE_HALF;
	int spriteY = y - REVERSE_HALF;

	DrawSprite(reverse, REVERSE_WIDTH, REVERSE_WIDTH, spriteX, spriteY);
}

void DrawSpinner(int x, int y, int index) {
	int spriteX = x - SPINNER_HALF;
	int spriteY = y - SPINNER_HALF;

	if (index == 0)
		DrawSprite(spinner[0], SPINNER_WIDTH, SPINNER_WIDTH, spriteX, spriteY);
	else
		DrawSprite(spinner[1], SPINNER_WIDTH, SPINNER_WIDTH, spriteX, spriteY);
}

void DrawInt(unsigned int num, int length, int posX, int posY) {
	posX += DIGIT_WIDTH * (length - 1);

	for (int digitPos = 1; digitPos <= length; digitPos++) {
		int digit = num % 10;
		DrawSprite(imageNum[digit], DIGIT_WIDTH, DIGIT_HEIGHT, posX, posY);

		num /= 10;
		posX -= DIGIT_WIDTH;
	}
}

void DrawPercent(unsigned int num, int posX, int posY)
{
	if (num > 99)
		DrawInt(num, 3, posX, posY);
	else if (num > 9)
		DrawInt(num, 2, posX + DIGIT_WIDTH, posY);
	else if (num >= 0)
		DrawInt(num, 1, posX + 2 * DIGIT_WIDTH, posY);

	DrawSprite(percent, PERCENT_WIDTH, PERCENT_HEIGHT, posX + 3 * DIGIT_WIDTH, posY);
}

void DrawCombo(unsigned int num, int posX, int posY)
{
	int digits = 0;

	if (num > 999)
		digits = 4;
	else if (num > 99)
		digits = 3;
	else if (num > 9)
		digits = 2;
	else if (num >= 0)
		digits = 1;

	DrawInt(num, digits, posX, posY);
	DrawSprite(comboX, COMBOX_WIDTH, COMBOX_HEIGHT,
			posX + digits * DIGIT_WIDTH, posY + 25);
}

void DrawMenu() {
	memcpy(image_buffer_pointer, imageMenu, NUM_BYTES_BUFFER);
}

void DrawScores(int score, int combo, int accuracy) {
	memcpy(image_buffer_pointer, bgScores, NUM_BYTES_BUFFER);
	//DrawSprite(imageRanking, RANKING_WIDTH, RANKING_HEIGHT, 0, 200); // 93, 258
	DrawInt(score, 7, 243, 268);
	DrawCombo(combo, 113, 693);
	DrawPercent(accuracy, 353, 693);
}

void DrawSettings(int volume, int offset)
{
	memcpy(image_buffer_pointer, bgSettings, NUM_BYTES_BUFFER);
	DrawInt(volume, 2, 913, 350);
	DrawInt(offset, 3, 889, 680);
}

void DrawSongs()
{
	memcpy(image_buffer_pointer, bgSongs, NUM_BYTES_BUFFER);
}

void plotLine(int x0, int y0, int x1, int y1, int colour) {

	int dx = x1 - x0;
	int dy = y1 - y0;
	int D = (2 * dy) - dx;
	int y = y0;

	for (int x = x0; x < x1; ++x) {
		SetPixel(image_buffer_pointer+((VGA_WIDTH*y)+(x)), colour);
		if (D > 0) {
			y = y + 1;
			D = D - (2 * dx);
		}
		D = D + (2 * dy);
	}

}

void drawline(int x0, int y0, int x1, int y1, int colour)
{
	int dx = x1 - x0;
	int dy = y1 - y0;

	if (abs(dy) > abs(dx)) {
		// Line algorithm needs y0 < y1
		// XOR swap x and y to draw it left to right
		if (y0 > y1) {
			x0 ^= x1;
			x1 ^= x0;
			x0 ^= x1;

			y0 ^= y1;
			y1 ^= y0;
			y0 ^= y1;

			dx *= -1;
			dy *= -1;
		}

		int x;
		for (int y = y0; y < y1; ++y)
		{
			x = x0 + dx * (y-y0)/dy;
			SetPixel(image_buffer_pointer+((VGA_WIDTH*y)+(x)), colour);
		}
	} else {
		// Line algorithm needs x0 < x1
		// XOR swap x and y to draw it left to right
		if (x0 > x1) {
			x0 ^= x1;
			x1 ^= x0;
			x0 ^= x1;

			y0 ^= y1;
			y1 ^= y0;
			y0 ^= y1;

			dx *= -1;
			dy *= -1;
		}

		int y;
		for (int x = x0; x < x1; ++x)
		{
			y = y0 + dy * (x-x0)/dx;
			SetPixel(image_buffer_pointer+((VGA_WIDTH*y)+(x)), colour);
		}
	}
}

RenderNode * createRenderNode(u32 * address, u32 size, u32 id)
{
	RenderNode *newNode = (RenderNode *)malloc(sizeof(RenderNode));
	newNode->address = address;
	newNode->size = size;
	newNode->delete = FALSE;
	newNode->drawnBuffer0 = FALSE;
	newNode->drawnBuffer1 = FALSE;
	newNode->id = id;
	return newNode;
}

Node_t * addRenderNode(Node_t ** masterRenderHead, RenderNode * renderNode)
{
	return ll_append_object(masterRenderHead, renderNode);

}

Node_t * removeRenderNodeByNode(Node_t * node)
{
	return ll_deleteNode(node);

}

Node_t * removeRenderNodeByRenderNode(Node_t ** masterRenderHead, RenderNode * node)
{
	if(*masterRenderHead == NULL){
		return NULL;
	}

	// other insertions
	Node_t * current = *masterRenderHead;
	while (TRUE) {
		if(current->data == node){
			return ll_deleteNodeHead(masterRenderHead, current);
		}
		if (current->next == NULL) {
			return NULL;
		}
		current = current->next;
	}
}

Node_t * removeRenderNodeByID(Node_t ** masterRenderHead, u32 id)
{
	if(*masterRenderHead == NULL){
		return NULL;
	}

	// other insertions
	Node_t * current = *masterRenderHead;
	while (TRUE) {
		if(((RenderNode *)(current->data))->id == id){
			return ll_deleteNodeHead(masterRenderHead, current);
		}
		if (current->next == NULL) {
			return NULL;
		}
		current = current->next;
	}
}
