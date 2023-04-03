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

/*--------------------------------------------------------------*/
/* Local Variables												*/
/*--------------------------------------------------------------*/
static int x = 0;
static int y = 0;

static bool isLMB = false;
static bool isRMB = false;
static bool wasLMB = false;
static bool wasRMB = false;

/*--------------------------------------------------------------*/
/* Functions													*/
/*--------------------------------------------------------------*/

int getMouseX()
{
	return x;
}

int getMouseY()
{
	return y;
}

void UpdateMouse(bool isLMBIn, bool isRMBIn, int dx, int dy)
{
	wasLMB = isLMB;
	wasRMB = isRMB;
	isLMB = isLMBIn;
	isRMB = isRMBIn;
	x += dx;
	y += dy;

	if (x < 0)
		x = 0;
	else if (x > VGA_WIDTH - 1)
		x = VGA_WIDTH - 1;

	if (y < 0)
		y = 0;
	else if (y > VGA_HEIGHT - 1)
		y = VGA_HEIGHT - 1;

	if (isLMB && !wasLMB) {
		CheckObjectCollision(true, false);
	}

	if (!isLMB && wasLMB) {
		CheckObjectCollision(false, true);
	}

	//xil_printf("Cursor at (%d, %d)\r\n", x, y);
}

void UpdateTablet(int x, int y)
{
	// tablet max x:1DAE y:128C
	// divide 4 max x:1899 y:1187
	x = x / 4;
	y = y / 4;

	if (y > VGA_HEIGHT - 1)
		y = VGA_HEIGHT - 1;
}
