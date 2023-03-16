#include "graphics.h"
#include "sd.h"
#include "xil_printf.h"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

// Note: Next buffer + 0x7E9004
int *image_output_buffer = (int *)0x02000000;
int *image_mouse_buffer = (int *)0x02FD2008;
int *image_buffer_pointer = (int *)0x037BB00C;

extern int *imageMenu;
extern int *imageBg;
extern int *imageCircle;
extern int *imageCircleOverlay;
extern int *spinner;
extern int *imageRanking;
extern int *imageNum[10];

void SetPixel(int *pixelAddr, int colour) {
	*pixelAddr = colour;
}

void DisplayBuffer() {
	memcpy(image_output_buffer, image_buffer_pointer, NUM_BYTES_BUFFER);
}

unsigned int mouseColour = 0xFFFF00;
static void DrawMouse(int x, int y) {
//	memcpy(image_mouse_buffer, image_buffer_pointer, NUM_BYTES_BUFFER);
//	*(image_mouse_buffer + x + y * VGA_WIDTH) = mouseColour;
//	*(image_mouse_buffer + x + (y+1) * VGA_WIDTH) = mouseColour;
//
//	if (x != VGA_WIDTH - 1) {
//		*(image_mouse_buffer + x+1 + y * VGA_WIDTH) = mouseColour;
//		*(image_mouse_buffer + x+1 + (y+1) * VGA_WIDTH) = mouseColour;
//	}

	//Performance
	*(image_output_buffer + x + y * VGA_WIDTH) = mouseColour;
	*(image_output_buffer + x + (y+1) * VGA_WIDTH) = mouseColour;

	if (x != VGA_WIDTH - 1) {
		*(image_output_buffer + x+1 + y * VGA_WIDTH) = mouseColour;
		*(image_output_buffer + x+1 + (y+1) * VGA_WIDTH) = mouseColour;
	}
}

void DisplayBufferAndMouse(int x, int y) {
//	DrawMouse(x, y);
//	memcpy(image_output_buffer, image_mouse_buffer, NUM_BYTES_BUFFER);

	//Performance
	DisplayBuffer();
	DrawMouse(x, y);
}

void FillScreen(int colour) {
	memset(image_buffer_pointer, colour, NUM_BYTES_BUFFER);
}

// Draws a pixel on top of another considering transparency
// Referenced https://gist.github.com/XProger/96253e93baccfbf338de
void PixelAlpha(int *under, int *over) {
	int alpha = (*over & 0xFF000000) >> 24;

	if (alpha == 0)
		return;

	if (alpha == 0xFF) {
		*under = *over;
		return;
	}

	unsigned int rb = *under & 0x00FF00FF;
	unsigned int g = *under & 0x0000FF00;

	rb += ((*over & 0x00FF00FF) - rb) * alpha >> 8;
	g += ((*over & 0x0000FF00) - g) * alpha >> 8;

	*under = (rb & 0x00FF00FF) | (g & 0x0000FF00);
}

// Draws a sprite on top of the output frame
void DrawSprite(int *sprite, int width, int height, int posX, int posY) {
	int *temp_pointer = (int *)image_buffer_pointer;
	int *sprite_pointer = (int *)sprite;
	temp_pointer += posX + posY * VGA_WIDTH;

	for (int currX = posX; currX < posX + height; currX++) {
		for (int currY = posY; currY < posY + width; currY++) {
			PixelAlpha(temp_pointer, sprite_pointer);

			temp_pointer++;
			sprite_pointer++;
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
	DrawSprite(imageCircleOverlay, CIRCLE_WIDTH, CIRCLE_WIDTH, spriteX, spriteY);
}

void DrawSpinner(int x, int y) {
	int spriteX = x - SPINNER_HALF;
	int spriteY = y - SPINNER_HALF;

	DrawSprite(spinner, SPINNER_WIDTH, SPINNER_WIDTH, spriteX, spriteY);
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

void DrawMenu() {
	memcpy(image_buffer_pointer, imageMenu, NUM_BYTES_BUFFER);
	DisplayBuffer();
}

void DrawGame(int score) {
	memcpy(image_buffer_pointer, imageBg, NUM_BYTES_BUFFER);
	DrawInt(score, 7, 951, 0);
	DrawSprite(imageCircle, CIRCLE_WIDTH, CIRCLE_WIDTH, 400, 500);
	DrawSprite(imageCircleOverlay, CIRCLE_WIDTH, CIRCLE_WIDTH, 400, 500);
	DrawSprite(imageCircle, CIRCLE_WIDTH, CIRCLE_WIDTH, 700, 250);
	DrawSprite(imageCircleOverlay, CIRCLE_WIDTH, CIRCLE_WIDTH, 700, 250);
	DisplayBuffer();
}

void DrawStats(int score) {
	memcpy(image_buffer_pointer, imageBg, NUM_BYTES_BUFFER);
	DrawSprite(imageRanking, RANKING_WIDTH, RANKING_HEIGHT, 0, 200);
	DrawInt(score, 7, 150, 210);
	DrawInt(score, 3, 20, 635);
	DrawInt(score, 3, 310, 635);
	DisplayBuffer();
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
