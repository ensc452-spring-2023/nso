/*-----------------------------------------------------------------------------/
 /	!nso - Cursor															   /
 /-----------------------------------------------------------------------------/
 /	William Zhang
 /	Bowie Gian
 /	04/03/2023
 /	cursor.c
 /
 /	Holds the cursor location and checks for collisions
 /----------------------------------------------------------------------------*/

/*--------------------------------------------------------------*/
/* Include Files												*/
/*--------------------------------------------------------------*/
#include <stdbool.h>
#include "cursor.h"
#include "graphics.h"
#include "game_logic.h"
#include "game_menu.h"

/*--------------------------------------------------------------*/
/* Local Variables												*/
/*--------------------------------------------------------------*/
static int mouseX = 100;
static int mouseY = 100;

static bool isLMB = false;
static bool isRMB = false;
static bool wasLMB = false;
static bool wasRMB = false;

/*--------------------------------------------------------------*/
/* Functions													*/
/*--------------------------------------------------------------*/

int getMouseX()
{
	return mouseX;
}

int getMouseY()
{
	return mouseY;
}

void UpdateMouse(bool isLMBIn, bool isRMBIn, int dx, int dy)
{
	wasLMB = isLMB;
	wasRMB = isRMB;
	isLMB = isLMBIn;
	isRMB = isRMBIn;
	mouseX += dx;
	mouseY += dy;

	if (mouseX < CURSOR_HALF)
		mouseX = CURSOR_HALF;
	else if (mouseX > VGA_WIDTH - CURSOR_HALF - 1)
		mouseX = VGA_WIDTH - CURSOR_HALF - 1;

	if (mouseY < CURSOR_HALF)
		mouseY = CURSOR_HALF;
	else if (mouseY > VGA_HEIGHT - CURSOR_HALF - 1)
		mouseY = VGA_HEIGHT - CURSOR_HALF - 1;

	if (isLMB && !wasLMB) {
		menu_collision(true, false);
	}

	if (!isLMB && wasLMB) {
		menu_collision(false, true);
	}

	//xil_printf("Cursor at (%d, %d)\r\n", mouseX, mouseY);
}

void UpdateTablet(int x, int y)
{
	// tablet max x:1DAE y:128C
	// divide 4 max x:1899 y:1187
	mouseX = x / 4;
	mouseY = y / 4;

	if (mouseX < CURSOR_HALF)
		mouseX = CURSOR_HALF;
	else if (mouseX > VGA_WIDTH - CURSOR_HALF)
		mouseX = VGA_WIDTH - CURSOR_HALF;

	if (mouseY < CURSOR_HALF)
		mouseY = CURSOR_HALF;
	else if (mouseY > VGA_HEIGHT - CURSOR_HALF)
		mouseY = VGA_HEIGHT - CURSOR_HALF;
}

bool RectCollision(int rectX, int rectY, int width, int height)
{
	if (mouseX < rectX)
		return false;

	if (mouseY < rectY)
		return false;

	if (mouseX > rectX + width)
		return false;

	if (mouseY > rectY + height)
		return false;

	return true;
}
