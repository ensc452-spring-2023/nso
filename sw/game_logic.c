/*-----------------------------------------------------------------------------/
 /	!nso - Game Logic														   /
 /-----------------------------------------------------------------------------/
 /	William Zhang
 /	Bowie Gian
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
#include "cursor.h"

#define SHOW_FPS 1 // Show FPS

/*--------------------------------------------------------------*/
/* Global Variables												*/
/*--------------------------------------------------------------*/
extern long time;
extern int numberOfHitobjects;
int ar = 0;
int volume = 10;
int beatoffset = 250;

int score;
int accuracy; // = score / maxScore
int maxCombo;

/*--------------------------------------------------------------*/
/* Local Variables												*/
/*--------------------------------------------------------------*/
static int maxScore;

static bool isPlaying = false;
static bool isSliding = false;
static bool isSpinning = false;


/*--------------------------------------------------------------*/
/* Game Stats													*/
/*--------------------------------------------------------------*/
#define MAX_HEALTH 300
static int health = 300;
static int currObjMaxScore = 0;
static int combo;

static void AddHealth()
{
	health += 10;
	if (health > MAX_HEALTH)
		health = MAX_HEALTH;
}

static void DrainHealth()
{
	health -= 20;
	if (health < 0)
		health = 0;
}

static void BreakCombo()
{
	if (combo > maxCombo)
		maxCombo = combo;

	combo = 0;
}

static void AddMaxScore(int num)
{
	maxScore += num;
	currObjMaxScore += num;
}

static void JudgeTiming(int time)
{
	xil_printf("Hit Timing:%4dms ", time);

	AddMaxScore(300);

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


/*--------------------------------------------------------------*/
/* Manipulating Objects in draw buffer							*/
/*--------------------------------------------------------------*/
static int objectsDrawn = 0;
static int objectsDeleted = 0;
static bool isScreenChanged = false;

static HitObject *gameHitobjects;
// Change to linked list? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
static int drawnObjectHead = 0;
static int drawnObjectTail = 0;
static int drawnObjectIndices[DRAWN_OBJECTS_MAX] =
	{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

static void AddObject()
{
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

static void DeleteObject(bool hit)
{
	if (hit)
		combo++;
	else
		BreakCombo();

	drawnObjectIndices[drawnObjectHead] = -1;

	if (++drawnObjectHead >= DRAWN_OBJECTS_MAX)
		drawnObjectHead = 0;

//	xil_printf("Deleting Object. Position: [%d,%d] Time: [%dms]\r\n", gameHitobjects[objectsDeleted].x,
//			gameHitobjects[objectsDeleted].y,gameHitobjects[objectsDeleted].time);

	if (gameHitobjects[objectsDeleted].type == OBJ_TYPE_CIRCLE) {
		maxScore += 300 - currObjMaxScore;
	} else if (gameHitobjects[objectsDeleted].type == OBJ_TYPE_SLIDER) {
		maxScore += 300 * (gameHitobjects[objectsDeleted].slides + 1) - currObjMaxScore;
		isSliding = false;
	} else if (gameHitobjects[objectsDeleted].type == OBJ_TYPE_SPINNER) {
		maxScore += 500 - currObjMaxScore;
		isSpinning = false;
	}

	currObjMaxScore = 0;
	objectsDeleted++;
	isScreenChanged = true;
}

bool IsDisplayed(int objectIndex)
{
	for (int displayIndex = 0; displayIndex < DRAWN_OBJECTS_MAX; displayIndex++) {
		if (drawnObjectIndices[displayIndex] == -1)
			continue;
		else if (drawnObjectIndices[displayIndex] == objectIndex)
			return true;
	}

	return false;
}


/*--------------------------------------------------------------*/
/* Object Drawing												*/
/*--------------------------------------------------------------*/
#define AC_MS 10 // Approach Circle Update ms

static int currSlides = 0;
static Node_t *nextCurvePoint = NULL;

static double sliderFollowerX = 0.0;
static double sliderFollowerY = 0.0;
static int spinnerIndex = 0;

// Creates a hit circle on the screen.
static void generateHitCircle(int x, int y, int acIndex, int comboIndex)
{
	DrawCircle(x, y);

	if (acIndex != 0)
		DrawApproachCircle(x, y, acIndex);

	if (comboIndex < 1)
		return;
	else if (comboIndex < 10) {
		DrawInt(comboIndex, 1, x - DIGIT_WIDTH / 2, y - DIGIT_HEIGHT / 2);
	} else if (comboIndex < 100) {
		DrawInt(comboIndex, 2, x - DIGIT_WIDTH, y - DIGIT_HEIGHT / 2);
	}
}

// Creates a slider on the screen.
static void generateSlider(HitObject *currentObjectPtr, int acIndex, bool sliding)
{
	int x = currentObjectPtr->x;
	int y = currentObjectPtr->y;
	int comboIndex = currentObjectPtr->comboLabel;
	int objSlides = currentObjectPtr->slides;
	int curveNumPoints = currentObjectPtr->curveNumPoints;
	Node_t *curvePointsHead = currentObjectPtr->curvePointsHead;

	Node_t *currNode = curvePointsHead;
	CurvePoint *point = (CurvePoint *)currNode->data;

	currNode = currNode->next;

	// Draw line segments
	for (int i = 0; i < curveNumPoints - 1; ++i) {
		CurvePoint *prevPoint = point;
		point = (CurvePoint *)currNode->data;

		drawline(prevPoint->x, prevPoint->y, point->x, point->y, 0x0000FF);

		currNode = currNode->next;
	}

	DrawSliderEnd(point->x, point->y);

	if (!sliding) {
		generateHitCircle(x, y, acIndex, comboIndex);

		if (objSlides > 1)
			DrawReverse(point->x, point->y);

		return;
	}

	DrawSliderEnd(x, y);

	int slidesLeft = objSlides - currSlides;

	if (slidesLeft < 2)
		return;

	if (slidesLeft > 2) {
		DrawReverse(x, y);
		DrawReverse(point->x, point->y);
		return;
	}

	if (currSlides % 2)
		DrawReverse(x, y);
	else
		DrawReverse(point->x, point->y);
}

// Creates a spinner on the screen.
static void generateSpinner(int x, int y, int spinnerIndex)
{
	DrawSpinner(x, y, spinnerIndex);
}

static void generateObject(HitObject *currentObjectPtr, bool sliding)
{
	int dt = 0;
	int aCircleIndex = 0;

	switch (currentObjectPtr->type) {
	case 0:
		//			xil_printf("Drawing Object[HitCircle]\r\n");
		dt = currentObjectPtr->time - time;
		if (dt <= 0) {
			aCircleIndex = 0;
		} else if (dt >= AC_MS * NUM_A_CIRCLES) {
			aCircleIndex = NUM_A_CIRCLES - 1;
		} else {
			// close approach circle every AC_MS
			aCircleIndex = dt / AC_MS;
		}

		generateHitCircle(currentObjectPtr->x, currentObjectPtr->y,
			aCircleIndex, currentObjectPtr->comboLabel);

		break;
	case 1:
		//			xil_printf("Drawing Object[Slider]\r\n");
		dt = currentObjectPtr->time - time;
		if (dt <= 0) {
			aCircleIndex = 0;
		} else if (dt >= AC_MS * NUM_A_CIRCLES) {
			aCircleIndex = NUM_A_CIRCLES - 1;
		} else {
			// close approach circle every AC_MS
			aCircleIndex = dt / AC_MS;
		}

		generateSlider(currentObjectPtr, aCircleIndex, sliding);
		break;
	case 3:
		//			xil_printf("Drawing Object[Spinner]\r\n");
		generateSpinner(currentObjectPtr->x, currentObjectPtr->y, spinnerIndex);
		break;
	default:
		xil_printf("Drawing Object[?]\r\n");
		break;
	}
}

static void RedrawGameplay()
{
	FillScreen(0x3F3F3F);

	for (int displayIndex = DRAWN_OBJECTS_MAX - 1; displayIndex >= 0; displayIndex--) {
		if (drawnObjectIndices[displayIndex] == -1)
			continue;

		if (drawnObjectIndices[displayIndex] == objectsDeleted)
			generateObject(&gameHitobjects[drawnObjectIndices[displayIndex]], isSliding);
		else
			generateObject(&gameHitobjects[drawnObjectIndices[displayIndex]], false);
	}

	if (isSliding) {
		DrawApproachCircle((int)sliderFollowerX, (int)sliderFollowerY, NUM_A_CIRCLES - 1);
	}

	// Health bar
	DrawRectangle(0, 20, health, 20, 0xFFFFFFFF);
	// Score
	DrawInt(score, 7, 1580, 0);
	// Accuracy
	accuracy = score * 100 / maxScore;
	DrawPercent(accuracy, 1721, 72);
	// Combo
	DrawCombo(combo, 0, VGA_HEIGHT - DIGIT_HEIGHT);

	DisplayBufferAndMouse(getMouseX(), getMouseY());
}


/*--------------------------------------------------------------*/
/* Game Logic													*/
/*--------------------------------------------------------------*/
#define CIRCLE_RAD_SQUARED 4096		//  64^2 - Hit circle hitbox
#define SLIDER_RAD_SQUARED 15625	// 125^2 - Sliding hitbox

#define ROTATION_CCW 1
#define ROTATION_CW 2

static u32 rotation = 0; // 4x u8 buffer for storing quadrant changes
static int quadrant = 0;
static int prevQuadrant = 0;
static int spins = 0;

double sliderSpeed = .5;

// Moves the slider follower towards point
void MoveSlider(HitObject* currentObjectPtr, int x1, int y1)
{
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

static void CheckSlider(HitObject *currentObjectPtr)
{
	CurvePoint *point = (CurvePoint *)nextCurvePoint->data;

	int x0 = (int)sliderFollowerX;
	int y0 = (int)sliderFollowerY;

	int dx = getMouseX() - x0;
	int dy = getMouseY() - y0;

	if (dx*dx + dy*dy > SLIDER_RAD_SQUARED) {
		AddMaxScore(300);
		score += 100;
		xil_printf("Slider broke! +100\r\nScore: %d\r\n", score);
		DeleteObject(false);
	}

	int x1 = point->x;
	int y1 = point->y;

	if (x0 == x1 && y0 == y1) {
		Node_t *currCurvePoint = nextCurvePoint;

		if ((currSlides % 2) == 0)
			nextCurvePoint = nextCurvePoint->next;
		else
			nextCurvePoint = nextCurvePoint->prev;

		if (nextCurvePoint == NULL) {
			currSlides++;

			AddMaxScore(300);
			score += 300;
			AddHealth();
			combo++;

			if (currSlides >= currentObjectPtr->slides) {
				xil_printf("Slider complete! +300\r\nScore: %d\r\n", score);
				DeleteObject(true);
			} else {
				xil_printf("Slider reverse! +300\r\nScore: %d\r\n", score);

				if ((currSlides % 2) == 0)
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

static void CheckSpin()
{
	int dx = getMouseX() - CENTER_X;
	int dy = getMouseY() - CENTER_Y;
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

	// Spin animation
	isScreenChanged = true;
	if (spinnerIndex == 0)
		spinnerIndex++;
	else
		spinnerIndex = 0;

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
		AddMaxScore(100);
		score += 100;
		xil_printf("Completed CCW Spin! +100 Spins: %d\r\nScore: %d\r\n", spins, score);
	} else if (rotation == 0x02020202) {
		spins++;
		rotation = 0;
		AddMaxScore(100);
		score += 100;
		xil_printf("Completed CW Spin! +100 Spins: %d\r\nScore: %d\r\n", spins, score);
	}

	if (spins >= 5) {
		AddHealth();
		DeleteObject(true);
	}
}


/*--------------------------------------------------------------*/
/* Mouse Functions												*/
/*--------------------------------------------------------------*/

// raising edge flags to be handled/cleared during gameplay
static bool isLMBPress = false;
static bool isLMBRelease = false;

void CheckObjectCollision(bool press, bool release)
{
	if (!IsDisplayed(objectsDeleted)) {
		return;
	}

	isLMBPress = press;
	isLMBRelease = release;
	HitObject *currentObjectPtr = &gameHitobjects[objectsDeleted];

	if (isLMBPress) {
		if (currentObjectPtr->type == OBJ_TYPE_SPINNER) {
			isSpinning = true;
			quadrant = 0;
			rotation = 0;
			spins = 0;

			isLMBPress = false;
			return;
		}

		int dx = getMouseX() - currentObjectPtr->x;
		int dy = getMouseY() - currentObjectPtr->y;

		if (dx * dx + dy * dy <= CIRCLE_RAD_SQUARED) {
			JudgeTiming(time - currentObjectPtr->time);
			xil_printf("Score: %d\r\n", score);
			AddHealth();

			if (currentObjectPtr->type == OBJ_TYPE_CIRCLE) {
				DeleteObject(true);
			} else if (currentObjectPtr->type == OBJ_TYPE_SLIDER) {
				xil_printf("Starting slider, hold it...\r\n");
				sliderFollowerX = currentObjectPtr->x;
				sliderFollowerY = currentObjectPtr->y;
				nextCurvePoint = currentObjectPtr->curvePointsHead->next;
				currSlides = 0;
				isSliding = true;
				combo++;
			}
		} else {
			xil_printf("Miss!\r\n");
		}

		isLMBPress = false;
	} else if (isLMBRelease && isSliding) {
		AddMaxScore(300);
		score += 100;
		xil_printf("Slider broke! +100\r\nScore: %d\r\n", score);
		DeleteObject(false);
		isLMBRelease = false;
	} else if (isLMBRelease && isSpinning) {
		isSpinning = false;
		isLMBRelease = false;
	}
}


/*-------------------------------------------/
 * game_tick()
 * Called by a 1ms interrupt timer.
 * Processes the game logic
/-------------------------------------------*/
char audioFileName[maxAudioFilenameSize];
int songLength;

void game_tick()
{
	if (objectsDeleted >= numberOfHitobjects)
		isPlaying = false;

	if (!isPlaying)
		return;

	if (health == 0) {
		xil_printf("HP = 0, Game Over!\r\n");
		isPlaying = false;
	}

	if (time == 0)
		AudioDMATransmitSong((u32 *)0x0B000000, songLength);

	if ((time >= gameHitobjects[objectsDrawn].time - AC_MS * NUM_A_CIRCLES) && objectsDrawn < numberOfHitobjects) {
		AddObject();
	}

	if (time >= gameHitobjects[objectsDeleted].time + 150) {
		if (isSpinning) {
			if (time >= gameHitobjects[objectsDeleted].endTime) {
				DeleteObject(false);
				DrainHealth();
				xil_printf("Object expired!\r\n");
			}
		} else if (!isSliding) {
			DeleteObject(false);
			DrainHealth();
			xil_printf("Object expired!\r\n");
		}
	}

	//if (isLMBPress) {
	//	CheckObjectCollision(&gameHitobjects[objectsDeleted]);
	//}

	if (isSliding) {
		CheckSlider(&gameHitobjects[objectsDeleted]);
		isScreenChanged = true;
		return;
	}

	if (isSpinning) {
		CheckSpin();
		return;
	}

	if (objectsDrawn != objectsDeleted) {
		isScreenChanged = true;
	}
}

// Resets gameplay variables to defaults
static void game_init()
{
	isPlaying = true;
	isSliding = false;
	isSpinning = false;

	isLMBPress = false;
	isLMBRelease = false;

	nextCurvePoint = NULL;

	objectsDrawn = 0;
	objectsDeleted = 0;
	drawnObjectHead = 0;
	drawnObjectTail = 0;

	for (int i = 0; i < DRAWN_OBJECTS_MAX; i++) {
		drawnObjectIndices[i] = -1;
	}

	time = -1500 + beatoffset;
	maxScore = 0;
	score = 0;
	maxCombo = 0;
	combo = 0;
	health = 300;
}


/*-------------------------------------------/
 * play_game()
 * Sets up game and draws the frames when
 * not processing a game_tick()
/-------------------------------------------*/
void play_game(HitObject *gameHitobjectsIn)
{
	if (gameHitobjectsIn == NULL)
		return;

	gameHitobjects = gameHitobjectsIn;

	songLength = loadWAVEfileintoMemory(audioFileName, (u32 *)0x0B000000);

	game_init();
	RedrawGameplay();

	while (objectsDeleted < numberOfHitobjects && isPlaying)
	{
#if SHOW_FPS == 1
		int startTime = time;
#endif

		//if (isScreenChanged) {
		//	isScreenChanged = false;
		//	RedrawGameplay();
		//} else {
		//	DisplayBufferAndMouse(getMouseX(), getMouseY());
		//}

		RedrawGameplay();

#if SHOW_FPS == 1
		int duration = (time - startTime) * 1000 / CLOCK_HZ;
		int fps = 1000 / duration;
		xil_printf("					Draw Time:%3dms FPS:%3d\r\n", duration, fps);
#endif
	}

	isPlaying = false;

	FillScreen(0x3F3F3F);
	DisplayBufferAndMouse(getMouseX(), getMouseY());

	//if (objectsDrawn != objectsDeleted)
	//	xil_printf("ERROR: objectsDrawn:%d != objectsDeleted:%d\n", objectsDrawn, objectsDeleted);

	AudioDMAReset();
}
