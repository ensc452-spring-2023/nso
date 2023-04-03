/*-----------------------------------------------------------------------------/
 /	!nso - Cursor															   /
 /-----------------------------------------------------------------------------/
 /	William Zhang
 /	Bowie Gian
 /	04/03/2023
 /	cursor.h
 /
 /	Holds the cursor location and checks for collisions
 /----------------------------------------------------------------------------*/

#ifndef CURSOR_H
#define CURSOR_H

/*--------------------------------------------------------------*/
/* Function Prototypes											*/
/*--------------------------------------------------------------*/
int getMouseX();
int getMouseY();
void UpdateMouse(bool isLMBIn, bool isRMBIn, int dx, int dy);
void UpdateTablet(int x, int y);
// Checks if mouse is in a rectangle anchored at the top left
bool RectCollision(int rectX, int rectY, int width, int height);

#endif