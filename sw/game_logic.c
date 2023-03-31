/*-----------------------------------------------------------------------------/
 /	!nso - Game Logic														   /
 /-----------------------------------------------------------------------------/
 /	William Zhang
 /	04/03/2023
 /	game_logic.c
 /
 /	Contains all the code regarding game logic of the game.
 /----------------------------------------------------------------------------*/

/*--------------------------------------------------------------*/
/* Include Files												*/
/*--------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include "xil_printf.h"
#include "game_logic.h"
#include "graphics.h"
#include "audio.h"
#include "sd.h"

/*--------------------------------------------------------------*/
/* Global Variables												*/
/*--------------------------------------------------------------*/
extern long time;
extern int numberOfHitobjects;
extern int score;

static HitObject *gameHitobjects;
int volume = 10;
int beatoffset = 0;

static bool isLMB = false;
static bool isRMB = false;
static bool wasLMB = false;
static bool wasRMB = false;
static int mouseX = 0;
static int mouseY = 0;

static bool isPlaying = false;
static bool isSliding = false;
static bool isSpinning = false;

// raising edge flags to be handled/cleared during gameplay
static bool isLMBPress = false;
static bool isRMBPress = false;
static bool isLMBRelease = false;

#define ROTATION_CCW 1
#define ROTATION_CW 2

static u32 rotation = 0; // 4x u8 buffer for storing quadrant changes
static int quadrant = 0;
static int prevQuadrant = 0;
static int spins = 0;

char audioFileName[maxAudioFilenameSize];

double sliderSpeed = .5;
static double sliderFollowerX = 0.0;
static double sliderFollowerY = 0.0;
static Node_t *nextCurvePoint = NULL;
static int slides = 0;

bool isScreenChanged = false;
int objectsDrawn = 0;
int objectsDeleted = 0;
// Change to linked list? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int drawnObjectHead = 0;
int drawnObjectTail = 0;
int drawnObjectIndices[DRAWN_OBJECTS_MAX] =
	{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

#define MAX_HEALTH 300
int health = 300;

static void AddHealth()
{
	health += 10;
	if (health > MAX_HEALTH)
		health = MAX_HEALTH;
}

static void DrainHealth()
{
	health -= 0;
	if (health < 0)
		health = 0;
}

static void AddScore(int time)
{
	xil_printf("Hit Timing:%4d ", time);

	if (abs(time) < 55) {
		score += 300;
		xil_printf("+300");
	} else if (abs(time) < 107) {
		score += 100;
		xil_printf("+100");
	} else if (abs(time) < 159) {
		score += 50;
		xil_printf(" +50");
	} else {
		xil_printf("Too Early!");
	}
	xil_printf("\r\n");
}

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

	objectsDrawn++;
	isScreenChanged = true;
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
	FillScreen(0x3F3F3F);

	for (int displayIndex = DRAWN_OBJECTS_MAX - 1; displayIndex >= 0; displayIndex--) {
		if (drawnObjectIndices[displayIndex] == -1)
			continue;

		generateObject(&gameHitobjects[drawnObjectIndices[displayIndex]]);
	}

	if (isSliding) {
		DrawSliderEnd((int)sliderFollowerX, (int)sliderFollowerY);
	}

	DrawRectangle(0, 20, health, 20, 0xFFFFFFFF);
	DrawInt(score, 7, 1590, 0);
	DisplayBufferAndMouse(mouseX, mouseY);
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
				AddScore(time - currentObjectPtr->time);
				AddHealth();
				xil_printf("Score: %d\r\n", score);
				DeleteObject();
			} else if (currentObjectPtr->type == OBJ_TYPE_SLIDER) {
				xil_printf("Starting slider, hold it...\r\n");
				sliderFollowerX = currentObjectPtr->x;
				sliderFollowerY = currentObjectPtr->y;
				nextCurvePoint = currentObjectPtr->curvePointsHead->next;
				slides = 0;
				isSliding = true;
			}
		} else {
			xil_printf("Miss!\r\n");
		}

		isLMBPress = false;
	} else if (isLMBRelease && isSliding) {
		score += 100;
		xil_printf("Slider broke! +100\r\nScore: %d\r\n", score);
		DeleteObject();
		isLMBRelease = false;
	} else if (isLMBRelease && isSpinning) {
		isSpinning = false;
		isLMBRelease = false;
	}
}

// Moves the slider follower towards point
void MoveSlider(HitObject* currentObjectPtr, int x1, int y1) {
	double dx = x1 - sliderFollowerX;
	double dy = y1 - sliderFollowerY;

	// Ignores if no move
	if (dx == 0.0 && dy == 0.0)
		return;

	if (abs(dy) > abs(dx)) {
		if (abs(dy) < sliderSpeed) {
			sliderFollowerX = x1;
			sliderFollowerY = y1;
		} else if (sliderFollowerY < y1) {
			sliderFollowerY += sliderSpeed;
			sliderFollowerX = sliderFollowerX + dx * sliderSpeed / dy;
		} else {
			sliderFollowerY -= sliderSpeed;
			sliderFollowerX = sliderFollowerX - dx * sliderSpeed / dy;
		}
	} else {
		if (abs(dx) < sliderSpeed) {
			sliderFollowerX = x1;
			sliderFollowerY = y1;
		} else if (sliderFollowerX < x1) {
			sliderFollowerX += sliderSpeed;
			sliderFollowerY = sliderFollowerY + dy * sliderSpeed / dx;
		} else {
			sliderFollowerX -= sliderSpeed;
			sliderFollowerY = sliderFollowerY - dy * sliderSpeed / dx;
		}
	}
}

static void CheckSlider(HitObject *currentObjectPtr) {
	CurvePoint *point = (CurvePoint *)nextCurvePoint->data;

	int x0 = (int)sliderFollowerX;
	int y0 = (int)sliderFollowerY;

	int dx = mouseX - x0;
	int dy = mouseY - y0;

	if (dx*dx + dy*dy > CIRCLE_RAD_SQUARED) {
		score += 100;
		xil_printf("Slider broke! +100\r\nScore: %d\r\n", score);
		DeleteObject();
	}

	int x1 = point->x;
	int y1 = point->y;

	if (x0 == x1 && y0 == y1) {
		Node_t *currCurvePoint = nextCurvePoint;

		if ((slides % 2) == 0)
			nextCurvePoint = nextCurvePoint->next;
		else
			nextCurvePoint = nextCurvePoint->prev;

		if (nextCurvePoint == NULL) {
			slides++;

			if (slides >= currentObjectPtr->slides) {
				score += 300;
				AddHealth();
				xil_printf("Slider complete! +300\r\nScore: %d\r\n", score);
				DeleteObject();
			} else {
				if ((slides % 2) == 0)
					nextCurvePoint = currCurvePoint->next;
				else
					nextCurvePoint = currCurvePoint->prev;

				point = (CurvePoint *)nextCurvePoint->data;
				x1 = point->x;
				y1 = point->y;
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

	u8 *tempRotation = (u8 *)&rotation;
	xil_printf("%d -> %d, %d%d%d%d\r\n", prevQuadrant, quadrant, tempRotation[0], tempRotation[1], tempRotation[2], tempRotation[3]);

	if (rotation == 0x01010101) {
		spins++;
		rotation = 0;
		xil_printf("Completed CCW Spin! +100 Spins: %d\r\nScore: %d\r\n", spins, score);
	} else if (rotation == 0x02020202) {
		spins++;
		rotation = 0;
		xil_printf("Completed CW Spin! +100 Spins: %d\r\nScore: %d\r\n", spins, score);
	}

	if (spins >= 5) {
		AddHealth();
		DeleteObject();
	}
}

void GameTick()
{
	if (!isPlaying)
		return;

	if ((time >= gameHitobjects[objectsDrawn].time - 16 * NUM_A_CIRCLES) && objectsDrawn < numberOfHitobjects) {
		AddObject();
	}

	if (time >= gameHitobjects[objectsDeleted].time + 150) {
		if (isSpinning) {
			if (time >= gameHitobjects[objectsDeleted].endTime) {
				DeleteObject();
				DrainHealth();
				xil_printf("Object expired!\r\n");
			}
		} else if (!isSliding) {
			DeleteObject();
			DrainHealth();
			xil_printf("Object expired!\r\n");
		}
	}

	//if (isLMBPress) {
	//	CheckCollision(&gameHitobjects[objectsDeleted]);
	//}

	if (objectsDrawn != objectsDeleted) {
		isScreenChanged = true;
	}

	if (isSliding) {
		CheckSlider(&gameHitobjects[objectsDeleted]);
		isScreenChanged = true;
	}

	if (isSpinning) {
		isScreenChanged = false;
		CheckSpin();
	}

	if (objectsDeleted >= numberOfHitobjects)
		isPlaying = false;

	if (health == 0) {
		xil_printf("HP = 0, Game Over!\r\n");
		isPlaying = false;
	}
}

// Resets gameplay variables to defaults
static void game_init()
{
	isPlaying = true;
	isSliding = false;
	isSpinning = false;

	isLMBPress = false;
	isRMBPress = false;
	isLMBRelease = false;

	objectsDrawn = 0;
	objectsDeleted = 0;
	drawnObjectHead = 0;
	drawnObjectTail = 0;

	for (int i = 0; i < DRAWN_OBJECTS_MAX; i++) {
		drawnObjectIndices[i] = -1;
	}

	time = -2500;
	score = 0;
	health = 300;
}

/*-------------------------------------------/
 / play_game()
 /--------------------------------------------/
 / Mainly a test function that iterates
 / through a array of hit objects at the
 / correct time. Not final game.
 /-------------------------------------------*/
void play_game(HitObject *gameHitobjectsIn)
{
	if (gameHitobjectsIn == NULL)
		return;

	gameHitobjects = gameHitobjectsIn;

	char input = ' ';
	xil_printf("Ready? (y)\(n)\r\n");

	scanf(" %c", &input);

	switch (input)
	{
	case 'y':
		game_init();

		//xil_printf("%s\r\n", audioFileName);

		AudioDMATransmitSong((u32 *) 0x0B000000, loadWAVEfileintoMemory(audioFileName, (u32 *) 0x0B000000));
		RedrawGameplay();

		while (objectsDeleted < numberOfHitobjects && isPlaying)
		{
			//int startTime = time;

			if (isScreenChanged) {
				isScreenChanged = false;
				RedrawGameplay();
			} else {
				DisplayBufferAndMouse(mouseX, mouseY);
			}

			//int duration = (time - startTime) * 1000 / CLOCK_HZ;
			//int fps = 1000 / duration;
			//xil_printf("					Draw Time:%3dms FPS:%3d\r\n", duration, fps);
		}

		isPlaying = false;

		FillScreen(0x3F3F3F);
		DisplayBufferAndMouse(mouseX, mouseY);

		//if (objectsDrawn != objectsDeleted)
		//	xil_printf("ERROR: objectsDrawn:%d != objectsDeleted:%d\n", objectsDrawn, objectsDeleted);

		break;
	case 'n':
		xil_printf("Quitted!\r\n");
		break;
	default:
		play_game(gameHitobjects);
		break;
	}
}

/* -------------------------------------------/
 * generateHitCircle()
 * -------------------------------------------/
 * Creates a hit circle on the screen.
 * ------------------------------------------*/
void generateHitCircle(int x, int y, int index){
	DrawCircle(x, y);

	if (index != 0)
		DrawApproachCircle(x, y, index);
}

/* -------------------------------------------/
 * generateSlider()
 * -------------------------------------------/
 * Creates a slider on the screen.
 * ------------------------------------------*/
void generateSlider(int x, int y, int index, int curveNumPoints, Node_t *curvePointsHead) {
	Node_t *currNode = curvePointsHead;
	CurvePoint *point = (CurvePoint *)currNode->data;

	currNode = currNode->next;

	//draw other line segments
	for (int i = 0; i < curveNumPoints - 1; ++i) {
		CurvePoint *prevPoint = point;
		point = (CurvePoint *)currNode->data;

		drawline(prevPoint->x, prevPoint->y, point->x, point->y, 0x0000FF);

		currNode = currNode->next;
	}

	DrawSliderEnd(point->x, point->y);

	generateHitCircle(x, y, index);
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
	int dt = 0;
	int aCircleIndex = 0;

	switch (currentObjectPtr->type) {
		case 0:
//			xil_printf("Drawing Object[HitCircle]\r\n");
			dt = currentObjectPtr->time - time;
			if (dt <= 0) {
				aCircleIndex = 0;
			} else if (dt >= 16 * NUM_A_CIRCLES) {
				aCircleIndex = NUM_A_CIRCLES - 1;
			} else {
				// close approach circle about faster than 1/60 s
				aCircleIndex = dt / 16;
			}

			generateHitCircle(currentObjectPtr->x, currentObjectPtr->y, aCircleIndex);

			break;
		case 1:
//			xil_printf("Drawing Object[Slider]\r\n");
			dt = currentObjectPtr->time - time;
			if (dt <= 0) {
				aCircleIndex = 0;
			}
			else if (dt >= 16 * NUM_A_CIRCLES) {
				aCircleIndex = NUM_A_CIRCLES - 1;
			}
			else {
				// close approach circle about faster than 1/60 s
				aCircleIndex = dt / 16;
			}

			generateSlider(currentObjectPtr->x, currentObjectPtr->y, aCircleIndex,
					currentObjectPtr->curveNumPoints, currentObjectPtr->curvePointsHead);
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

int getMouseX()
{
	return mouseX;
}

int getMouseY()
{
	return mouseY;
}
