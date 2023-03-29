/*----------------------------------------------------------------------------/
 /  !nso - Game Logic                                                          /
 /-----------------------------------------------------------------------------/
 /	William Zhang
 / 	04/03/2023
 /	game_logic.h
 /
 /	Contains all the code regarding game logic of the game.
 /----------------------------------------------------------------------------*/

/*--------------------------------------------------------------*/
/* Include Files				 								*/
/*--------------------------------------------------------------*/
#include "ff.h"
#include <stdbool.h>

/*--------------------------------------------------------------*/
/* Definitions					 								*/
/*--------------------------------------------------------------*/
#define bufferSize 32
#define PLAY_AREA_SCALER 2
#define PLAY_AREA_OFFSET_X 448
#define PLAY_AREA_OFFSET_Y 156
#define DRAWN_OBJECTS_MAX 20 // manually init array to -1 in play_game (change to linked list soon)
#define CLOCK_HZ 1000

/*--------------------------------------------------------------*/
/* Structs				 										*/
/*--------------------------------------------------------------*/
typedef struct
{

} Beatmap;

typedef struct
{
	char audioFilename[bufferSize];
	int audioLeadIn;
	int previewTime;
	int countdown;
	char sampleSet[bufferSize];
	float stackLeniency;
	int mode;
	int LetterboxInBreaks;
	int useSkinSprites;
	char overlayPosition[bufferSize];
	char skinPreference[bufferSize];
	int epilepsyWarning;
	int countdownOffset;
	int specialStyle;
	int widescreenStoryboard;
	int samplesMatchPlaybackRate;
} BeatmapGeneral;

typedef struct
{
	int x;
	int y;
} CurvePoint;

/* New Combo
 * 0 = not a new combo
 * 1 = next combo/ new combo
 *
 * Type
 * 0 = Hit Circle
 * 1 = Slider
 * 3 = Spinner
 *
 */

#define OBJ_TYPE_CIRCLE 0
#define OBJ_TYPE_SLIDER 1
#define OBJ_TYPE_SPINNER 3

typedef struct
{
	int x;
	int y;
	int time;
	int type;
	int hitSound;
	char * objectParams;
	char * hitSample;
	int newCombo;
	char curveType;
	CurvePoint * curvePoints;
	int curveNumPoints;
	int slides;
	int length;
	int * edgeSounds;
	char * edgeSets;
	int endTime;

} HitObject;

/*--------------------------------------------------------------*/
/* Functions Prototypes 										*/
/*--------------------------------------------------------------*/

void parse_beatmaps(char *filename, FATFS FS_instance);
void GameTick();
void play_game();
void generateHitCircle(int x, int y, int index);
void generateSlider(int x, int y, int index, int curveNumPoints, CurvePoint* curvePoints);
void generateSpinner(int x, int y);
void generateObject(HitObject *currentObjectPtr);
void UpdateMouse(bool isLMBIn, bool isRMBIn, int dx, int dy);
int getMouseX();
int getMouseY();
