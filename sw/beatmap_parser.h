/*-----------------------------------------------------------------------------/
 /	!nso - Beatmap Parser													   /
 /-----------------------------------------------------------------------------/
 /	William Zhang
 /	Bowie Gian
 /	04/03/2023
 /	beatmap_parser.h
 /
 /	Parses required info from .osu beatmap files into data structures
 /----------------------------------------------------------------------------*/

#ifndef BEATMAP_PARSER_H
#define BEATMAP_PARSER_H

/*--------------------------------------------------------------*/
/* Include Files												*/
/*--------------------------------------------------------------*/
#include "ff.h"
#include "linked_list.h"

/*--------------------------------------------------------------*/
/* Definitions													*/
/*--------------------------------------------------------------*/
#define BM_STR_SIZE 32
#define PLAY_AREA_SCALER 2
#define PLAY_AREA_OFFSET_X 448
#define PLAY_AREA_OFFSET_Y 156

#define OBJ_TYPE_CIRCLE 0
#define OBJ_TYPE_SLIDER 1
#define OBJ_TYPE_SPINNER 3

/*--------------------------------------------------------------*/
/* Structs														*/
/*--------------------------------------------------------------*/
typedef struct
{

} Beatmap;

typedef struct
{
	char audioFilename[BM_STR_SIZE];
	int audioLeadIn;
	int previewTime;
	int countdown;
	char sampleSet[BM_STR_SIZE];
	float stackLeniency;
	int mode;
	int LetterboxInBreaks;
	int useSkinSprites;
	char overlayPosition[BM_STR_SIZE];
	char skinPreference[BM_STR_SIZE];
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

typedef struct
{
	int x;
	int y;
	int time;
	int type;
	int hitSound;
	char *objectParams;
	char *hitSample;
	int comboLabel;
	char curveType;
	Node_t *curvePointsHead;
	Node_t *curvePointsTail;
	int curveNumPoints;
	int slides;
	int length;
	int *edgeSounds;
	char *edgeSets;
	int endTime;
} HitObject;

/*--------------------------------------------/
 / parse_beatmaps()
 /--------------------------------------------/
 / Takes an input .osu file and parses the
 / game data into a hit object
 / array. User must free the memory with free_hitobjects.
 /-------------------------------------------*/
HitObject *parse_beatmaps(char *filename, FATFS FS_instance);
void free_hitobjects(HitObject *gameHitobjects);

#endif
