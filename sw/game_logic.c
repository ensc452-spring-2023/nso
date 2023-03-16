/*----------------------------------------------------------------------------/
 /  !nso - Game Logic                                                         /
 /----------------------------------------------------------------------------/
 /	William Zhang
 /	04/03/2023
 /	game_logic.h
 /
 /	Contains all the code regarding game logic of the game.
 /----------------------------------------------------------------------------*/

/*--------------------------------------------------------------*/
/* Include Files				 								*/
/*--------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ff.h"
#include "game_logic.h"
#include "xil_printf.h"
#include "sleep.h"
#include "graphics.h"

/*--------------------------------------------------------------*/
/* Global Variables				 								*/
/*--------------------------------------------------------------*/
extern long time;
extern HitObject * gameHitobjects;
extern int numberOfHitobjects;
extern int score;
int volume = 10;
int beatoffset = 0;

static bool isLMB = false;
static bool isRMB = false;
static bool wasLMB = false;
static bool wasRMB = false;
static int mouseX = 0;
static int mouseY = 0;

static int curvePointIndex = 0;
static bool isSliding = false;
static bool isSpinning = false;

// raising edge flags to be handled/cleared during gameplay
static bool isLMBPress = false;
static bool isRMBPress = false;
static bool isLMBRelease = false;

#define ROTATION_CCW 1
#define ROTATION_CW 2
// 4x u8 buffer for storing quadrant changes
u32 rotation = 0;
int quadrant = 0;
int prevQuadrant = 0;
int spins = 0;

/*-------------------------------------------/
 / parse_beatmaps()
 /--------------------------------------------/
 / Takes an input .osu file and parses the
 / game data into a global hit object
 / array. Will mount the sd card into 0:/
 /-------------------------------------------*/
void parse_beatmaps(char *filename, FATFS FS_instance)
{
	FRESULT result;
	FIL beatmapFile;
	char buffer[4096];

	free(gameHitobjects);
	gameHitobjects = (HitObject *) malloc(512 * sizeof(HitObject));

	if (gameHitobjects == NULL) {
		xil_printf("ERROR: could not malloc memory for HitObject Array %d\r\n", sizeof(HitObject));
		return;
	}


	numberOfHitobjects = 0;

	xil_printf("Parsing beatmap: %s\n\r", filename);

	result = f_mount(&FS_instance, "0:/", 1);
	if (result != 0)
	{
		xil_printf("Couldn't mount SD Card.\r\n");
		free(gameHitobjects);
		return;
	}

	result = f_open(&beatmapFile, filename, FA_READ);
	if (result != 0)
	{
		xil_printf("File not found");
		free(gameHitobjects);
		return;
	}

	f_gets(buffer, 32, &beatmapFile);
	xil_printf("Top of File:", buffer);

	HitObject test;

	while (f_gets(buffer, 32, &beatmapFile) != NULL)
	{
		if (buffer[0] == '[')
		{
			xil_printf("Reading Section: %s", buffer);
			if (buffer[1] == 'H')
			{ //Hit Objects Section
				while (f_gets(buffer, 4096, &beatmapFile) != NULL)
				{

					u8 temp_type = 0xFF;
					char objectParamsBuffer[4096];

					//osupixel is equivalent to 640x480
					test.x = atoi(strtok(buffer, ",")) * PLAY_AREA_SCALER + PLAY_AREA_OFFSET;
					test.y = atoi(strtok(NULL, ",")) * PLAY_AREA_SCALER + PLAY_AREA_OFFSET;
					test.time = atoi(strtok(NULL, ","));
					temp_type = atoi(strtok(NULL, ","));
					test.hitSound = atoi(strtok(NULL, ","));

					//bit 2 - New Combo
					if ((temp_type & 0b00000100) == 0b00000100)
					{
						test.newCombo = 1;
					}
					else
					{
						test.newCombo = 0;
					}

					//bit 0 - Hit Circle
					if ((temp_type & 0b00000001) == 0b00000001)
					{
						test.type = 0;
					}
					//bit 1 - Slider
					if ((temp_type & 0b00000010) == 0b00000010)
					{
						test.type = 1;

						strcpy(objectParamsBuffer, strtok(NULL, ","));
						test.curveType = objectParamsBuffer[0];
						test.slides = atoi(strtok(NULL, ","));
						test.length = atoi(strtok(NULL, ","));
						//test.edgeSounds = strtok(NULL, ",");
						//test.edgeSets = strtok(NULL, ",");

						//xil_printf("\r\n");
						//xil_printf("              >> %s\r\n", objectParamsBuffer);

						char * pObjectParams, *p, *temp;
						pObjectParams = strdup(objectParamsBuffer);
						temp = pObjectParams;

						test.curvePoints = (CurvePoint *) malloc(10 * sizeof(CurvePoint));
						if (test.curvePoints == NULL) {
							xil_printf("ERROR: could not malloc memory for curvePoints array\r\n");
							return;
						}
						test.curveNumPoints = -1;

						while ((p = strsep(&pObjectParams, "|")) != NULL)
						{
							//printf("<<%s>>\r\n", p);

							if (test.curveNumPoints >= 0 && test.curveNumPoints < 10)
							{
								int x = 0;
								int y = 0;
								sscanf(p, "%d:%d", &x, &y);

								//osupixel is equivalent to 640x480
								test.curvePoints[test.curveNumPoints].x = x * PLAY_AREA_SCALER + PLAY_AREA_OFFSET;
								test.curvePoints[test.curveNumPoints].y = y * PLAY_AREA_SCALER + PLAY_AREA_OFFSET;

								//test.curvePoints[test.curveNumPoints].x = x;
								//test.curvePoints[test.curveNumPoints].y = y;
								//xil_printf("x: %d y:%d ", test.curvePoints[test.curveNumPoints].x, test.curvePoints[test.curveNumPoints].y);
							}
							test.curveNumPoints++;
						}

						free(temp);
						//xil_printf("\r\n");
					}

					//bit 3 - Spinner
					if ((temp_type & 0b00001000) == 0b00001000)
					{
						test.type = 3;
					}
					//bit 7 - osu!mania hold
					if ((temp_type & 0b10000000) == 0b10000000)
					{
						test.type = 7;
						xil_printf("ERROR: WHY ARE YOU HERE?\r\n");
					}

					if ((temp_type & 0b01110000) >> 4 > 0)
					{
						xil_printf("Skip %d combos\r\n", (temp_type & 0b01110000) >> 3);
					}

					memcpy(&gameHitobjects[numberOfHitobjects++], &test, sizeof(HitObject));

					if (test.newCombo)
					{
						xil_printf("New Combo >> ");
					}

					if (test.type == 0)
					{
						xil_printf("Hit Circle - Position: [%d,%d] Time: [%d]\r\n", test.x, test.y, test.time);
					}
					else if (test.type == 1)
					{
						xil_printf("Slider - Position: [%d,%d] Time: [%d] CT:[%c] S:[%d] L:[%d] \r\n", test.x, test.y, test.time, test.curveType,
								test.slides, test.length);
						xil_printf("\t\t>>>> From Points: (%d,%d)", test.x, test.y);
						for (int i = 0; i < test.curveNumPoints; ++i) {
							if (i < 10) {
								xil_printf("-(%d,%d)",test.curvePoints[i].x,test.curvePoints[i].y);
							}
						}
						xil_printf("\r\n");
					}
					else if (test.type == 3)
					{
						xil_printf("Spinner - Position: [%d,%d] Time: [%d]\r\n", test.x, test.y, test.time);
					}
					else
					{
						xil_printf("Unknown Hit Object\r\n");
					}
				}
			}
		}
	}

	f_close(&beatmapFile);
}

bool isScreenChanged = false;
int objectsDrawn = 0;
int objectsDeleted = 0;
// Change to linked list? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	int drawnObjectHead = 0;
	int drawnObjectTail = 0;
	int drawnObjectIndices[DRAWN_OBJECTS_MAX] =
		{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

static void AddObject() {
	if (drawnObjectIndices[drawnObjectTail] != -1) {
		xil_printf("Error: drawnObjectIndices array overloaded! Overwriting...\n");

		if (++drawnObjectHead >= DRAWN_OBJECTS_MAX)
			drawnObjectHead = 0;
	}

	drawnObjectIndices[drawnObjectTail] = objectsDrawn;

	if (++drawnObjectTail >= DRAWN_OBJECTS_MAX)
		drawnObjectTail = 0;

//	xil_printf("Adding Object. Position: [%d,%d] Time: [%dms]\r\n", gameHitobjects[objectsDrawn].x,
//			gameHitobjects[objectsDrawn].y,gameHitobjects[objectsDrawn].time);

	generateObject(&gameHitobjects[objectsDrawn]);
	DisplayBufferAndMouse(mouseX, mouseY);

	objectsDrawn++;
}

static void DeleteObject() {
	drawnObjectIndices[drawnObjectHead] = -1;

	if (++drawnObjectHead >= DRAWN_OBJECTS_MAX)
		drawnObjectHead = 0;

//	xil_printf("Deleting Object. Position: [%d,%d] Time: [%dms]\r\n", gameHitobjects[objectsDeleted].x,
//			gameHitobjects[objectsDeleted].y,gameHitobjects[objectsDeleted].time);

	if (gameHitobjects[objectsDeleted].type == OBJ_TYPE_SLIDER) {
		isSliding = false;
	} else if (gameHitobjects[objectsDeleted].type == OBJ_TYPE_SPINNER) {
		isSpinning = false;
	}

	objectsDeleted++;
	isScreenChanged = true;
}

static void RedrawGameplay() {
	int startTime = time;

	FillScreen(0x3F3F3F);

	for (int displayIndex = 0; displayIndex < DRAWN_OBJECTS_MAX; displayIndex++) {
		if (drawnObjectIndices[displayIndex] == -1)
			continue;

		generateObject(&gameHitobjects[drawnObjectIndices[displayIndex]]);
	}

	DisplayBufferAndMouse(mouseX, mouseY);

	int duration = time - startTime;
	int fps = 1000 / duration;

//	xil_printf("Draw Time:%3dms FPS:%3d\r\n", duration, fps);
}

static void CheckCollision(HitObject *currentObjectPtr) {
	if (isLMBPress) {
		if (currentObjectPtr->type == OBJ_TYPE_SPINNER) {
			isSpinning = true;
			quadrant = 0;
			rotation = 0;
			spins = 0;

			isLMBPress = false;
			return;
		}

		int dx = mouseX - currentObjectPtr->x;
		int dy = mouseY - currentObjectPtr->y;

		if (dx*dx + dy*dy <= CIRCLE_RAD_SQUARED) {
			if (currentObjectPtr->type == OBJ_TYPE_CIRCLE) {
				score += 300;
				xil_printf("Hit! Score: %d\r\n", score);
				DeleteObject();
			} else if (currentObjectPtr->type == OBJ_TYPE_SLIDER) {
				xil_printf("Starting slider, hold it...\r\n");
				curvePointIndex = 0;
				isSliding = true;
			}
		} else {
			xil_printf("Miss!\r\n");
		}

		isLMBPress = false;
	} else if (isLMBRelease && isSliding) {
		score += 100;
		xil_printf("Slider broke! Score: %d\r\n", score);
		DeleteObject();
		isLMBRelease = false;
	} else if (isLMBRelease && isSpinning) {
		isSpinning = false;
		isLMBRelease = false;
	}
}

#define SLIDER_SPEED 10

// Moves the slider start towards point
void MoveSlider(HitObject* currentObjectPtr, int x1, int y1) {
	int x0 = currentObjectPtr->x;
	int y0 = currentObjectPtr->y;

	int dx = x1 - x0;
	int dy = y1 - y0;

	// Ignores if no move
	if (dx == 0 && dy == 0)
		return;

	if (abs(dy) > abs(dx)) {
		if (abs(dy) < SLIDER_SPEED) {
			currentObjectPtr->x = x1;
			currentObjectPtr->y = y1;
		} else if (y0 < y1) {
			currentObjectPtr->y += SLIDER_SPEED;
			currentObjectPtr->x = x0 + dx * SLIDER_SPEED / dy;
		} else {
			currentObjectPtr->y -= SLIDER_SPEED;
			currentObjectPtr->x = x0 - dx * SLIDER_SPEED / dy;
		}
	} else {
		if (abs(dx) < SLIDER_SPEED) {
			currentObjectPtr->x = x1;
			currentObjectPtr->y = y1;
		} else if (x0 < x1) {
			currentObjectPtr->x += SLIDER_SPEED;
			currentObjectPtr->y = y0 + dy * SLIDER_SPEED / dx;
		} else {
			currentObjectPtr->x -= SLIDER_SPEED;
			currentObjectPtr->y = y0 - dy * SLIDER_SPEED / dx;
		}
	}
}

static void CheckSlider(HitObject *currentObjectPtr) {
	int curveNumPoints = currentObjectPtr->curveNumPoints;
	CurvePoint *curvePoints = currentObjectPtr->curvePoints;

	int x0 = currentObjectPtr->x;
	int y0 = currentObjectPtr->y;

	int dx = mouseX - x0;
	int dy = mouseY - y0;

	if (dx*dx + dy*dy > CIRCLE_RAD_SQUARED) {
		score += 100;
		xil_printf("Slider broke! Score: %d\r\n", score);
		DeleteObject();
	}

	int x1 = curvePoints[0].x;
	int y1 = curvePoints[0].y;

	if (x0 == x1 && y0 == y1) {
		curvePointIndex++;

		if (curveNumPoints <= 1 || curvePointIndex >= 10) {
			score += 300;
			xil_printf("Slider Complete! Score: %d \r\n", score);
			DeleteObject();
		} else {
			currentObjectPtr->curveNumPoints--;
			for (int i = 0; i < curveNumPoints; i++) {
				curvePoints[i] = curvePoints[i+1];
			}
		}
	}

	MoveSlider(currentObjectPtr, x1, y1);
}

static void CheckSpin() {
	int dx = mouseX - CENTER_X;
	int dy = mouseY - CENTER_Y;
	prevQuadrant = quadrant;

	if (dx >= 0 && dy > 0) {
		quadrant = 1;
	} else if (dx < 0 && dy >= 0) {
		quadrant = 2;
	} else if (dx <= 0 && dy < 0) {
		quadrant = 3;
	} else if (dx > 0 && dy <= 0) {
		quadrant = 4;
	} else {
		quadrant = 0;
		xil_printf("ERROR in CheckSpin\r\n");
		return;
	}

	// Mouse didn't change quadrant
	if (quadrant == prevQuadrant)
		return;

	// First time run
	if (prevQuadrant == 0)
		return;

	// Adjacent quadrant changes
	if (prevQuadrant == 2 || prevQuadrant == 3) {
		if (quadrant == (prevQuadrant + 1)) {
			rotation <<= 8;
			rotation += ROTATION_CCW;
		} else if (quadrant == (prevQuadrant - 1)) {
			rotation <<= 8;
			rotation += ROTATION_CW;
		}
	} else if (prevQuadrant == 1) {
		if (quadrant == 2) {
			rotation <<= 8;
			rotation += ROTATION_CCW;
		} else if (quadrant == 4) {
			rotation <<= 8;
			rotation += ROTATION_CW;
		}
	} else if (prevQuadrant == 4) {
		if (quadrant == 1) {
			rotation <<= 8;
			rotation += ROTATION_CCW;
		} else if (quadrant == 3) {
			rotation <<= 8;
			rotation += ROTATION_CW;
		}
	}

	u8 *tempRotation = &rotation;
	xil_printf("%d -> %d, %d%d%d%d\r\n", prevQuadrant, quadrant, tempRotation[0], tempRotation[1], tempRotation[2], tempRotation[3]);

	if (rotation == 0x01010101) {
		spins++;
		rotation = 0;
		xil_printf("Completed CCW Spin! Spins: %d\r\n", spins);
	} else if (rotation == 0x02020202) {
		spins++;
		rotation = 0;
		xil_printf("Completed CW Spin! Spins: %d\r\n", spins);
	}

	if (spins >= 5)
		DeleteObject();
}

/*-------------------------------------------/
 / play_game()
 /--------------------------------------------/
 / Mainly a test function that iterates
 / through a array of hit objects at the
 / correct time. Not final game.
 /-------------------------------------------*/
void play_game()
{
	char input = ' ';
	xil_printf("Ready? (y)\(n)\r\n");

	scanf(" %c", &input);

	objectsDrawn = 0;
	objectsDeleted = 0;
	time = 0;
	score = 0;

	switch (input)
	{
	case 'y':
		// clear screen
		FillScreen(0x3F3F3F);
		DisplayBufferAndMouse(mouseX, mouseY);

		while (objectsDeleted < numberOfHitobjects)
		{
			if ((time >= gameHitobjects[objectsDrawn].time) && objectsDrawn < numberOfHitobjects) {
				AddObject();
			}

			if (time >= gameHitobjects[objectsDeleted].time + 2500) {
				DeleteObject();
				xil_printf("Object expired!\r\n");
			}

//			if (isLMBPress) {
//				CheckCollision(&gameHitobjects[objectsDeleted]);
//			}

			if (isSliding) {
				CheckSlider(&gameHitobjects[objectsDeleted]);
				isScreenChanged = true;
			}

			if (isSpinning)
				CheckSpin();

			if (isScreenChanged) {
				isScreenChanged = false;
				RedrawGameplay();
			} else {
				DisplayBufferAndMouse(mouseX, mouseY);
			}
		}

		FillScreen(0x3F3F3F);
		DisplayBufferAndMouse(mouseX, mouseY);

		if (objectsDrawn != objectsDeleted)
			xil_printf("ERROR: objectsDrawn:%d != objectsDeleted:%d\n", objectsDrawn, objectsDeleted);

		break;
	case 'n':
		xil_printf("Quitted!\r\n");
		break;
	default:
		play_game();
		break;
	}
}

/* -------------------------------------------/
 * generateSlider()
 * -------------------------------------------/
 * Creates a slider on the screen.
 * ------------------------------------------*/
void generateSlider(int x, int y, int curveNumPoints, CurvePoint * curvePoints){
	DrawCircle(x, y);

	//draw first line segment,
	drawline(x, y, curvePoints[0].x, curvePoints[0].y, 0x0000FF);

	//draw other line segments
	for (int i = 0; i < curveNumPoints - 1; ++i) {
		if (i < 8)
		{
			drawline(curvePoints[i].x, curvePoints[i].y,
					curvePoints[i+1].x, curvePoints[i+1].y, 0x0000FF);
		}
	}

	if (curveNumPoints < 10) {
		DrawSliderEnd(curvePoints[curveNumPoints - 1].x, curvePoints[curveNumPoints - 1].y);
	} else {
		DrawSliderEnd(curvePoints[9].x, curvePoints[9].y);
	}
}

/* -------------------------------------------/
 * generateHitCircle()
 * -------------------------------------------/
 * Creates a hit circle on the screen.
 * ------------------------------------------*/
void generateHitCircle(int x, int y){
	DrawCircle(x, y);
}

/* -------------------------------------------/
 * generateSpinner()
 * -------------------------------------------/
 * Creates a spinner on the screen.
 * ------------------------------------------*/
void generateSpinner(int x, int y){
	DrawSpinner(x, y);

}

void generateObject(HitObject *currentObjectPtr) {
	switch (currentObjectPtr->type) {
		case 0:
//			xil_printf("Drawing Object[HitCircle]\r\n");
			generateHitCircle(currentObjectPtr->x, currentObjectPtr->y);
			break;
		case 1:
//			xil_printf("Drawing Object[Slider]\r\n");
			generateSlider(currentObjectPtr->x, currentObjectPtr->y,
					currentObjectPtr->curveNumPoints, currentObjectPtr->curvePoints);
			break;
		case 3:
//			xil_printf("Drawing Object[Spinner]\r\n");
			generateSpinner(currentObjectPtr->x, currentObjectPtr->y);
			break;
		default:
			xil_printf("Drawing Object[?]\r\n");
			break;
	}
}

bool IsDisplayed(int objectIndex) {
	for (int displayIndex = 0; displayIndex < DRAWN_OBJECTS_MAX; displayIndex++) {
		if (drawnObjectIndices[displayIndex] == -1)
			continue;
		else if (drawnObjectIndices[displayIndex] == objectIndex)
			return true;
	}

	return false;
}

void UpdateMouse(bool isLMBIn, bool isRMBIn, int dx, int dy) {
	wasLMB = isLMB;
	wasRMB = isRMB;
	isLMB = isLMBIn;
	isRMB = isRMBIn;
	mouseX += dx;
	mouseY += dy;

	if (mouseX < 0)
		mouseX = 0;
	else if (mouseX > VGA_WIDTH - 1)
		mouseX = VGA_WIDTH - 1;

	if (mouseY < 0)
		mouseY = 0;
	else if (mouseY > VGA_HEIGHT - 1)
		mouseY = VGA_HEIGHT - 1;

	// If the next object is not displayed, skip check
	if (!IsDisplayed(objectsDeleted)) {
		return;
	}

	if (isLMB && !wasLMB) {
		isLMBPress = true;
		CheckCollision(&gameHitobjects[objectsDeleted]);
	}

	if (!isLMB && wasLMB) {
		isLMBRelease = true;
		CheckCollision(&gameHitobjects[objectsDeleted]);
	}

	if (isRMB && !wasRMB) {
		isRMBPress = true;
	}

	//xil_printf("Cursor at (%d, %d)\r\n", mouseX, mouseY);
}
