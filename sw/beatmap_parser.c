/*-----------------------------------------------------------------------------/
 /	!nso - Game Logic														   /
 /-----------------------------------------------------------------------------/
 /	William Zhang
 /	Bowie Gian
 /	04/03/2023
 /	beatmap_parser.c
 /
 /	Parses required info from .osu beatmap files into data structures
 /----------------------------------------------------------------------------*/

/*--------------------------------------------------------------*/
/* Include Files												*/
/*--------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include "xil_printf.h"
#include "beatmap_parser.h"

#define PARSER_BUFF_SIZE 4096

/*--------------------------------------------------------------*/
/* Global Variables												*/
/*--------------------------------------------------------------*/
extern int numberOfHitobjects;
extern double sliderSpeed;

/*--------------------------------------------------------------*/
/* Local Variables												*/
/*--------------------------------------------------------------*/
static HitObject *gameHitobjects;
static HitObject object;
static FIL beatmapFile;
static char buffer[PARSER_BUFF_SIZE];

static int sliderMultiplier = 0;
static int beatLength = 0;

static void parse_difficulty()
{
	while (f_gets(buffer, PARSER_BUFF_SIZE, &beatmapFile) != NULL) {
		char *setting = strtok(buffer, ":");
		if (strcmp(setting, "SliderMultiplier") == 0) {
			float tempMultiplier = atof(strtok(NULL, "\n"));
			sliderMultiplier = (int)(tempMultiplier * 100 * PLAY_AREA_SCALER);
			break;
		}
	}
}

static void parse_timing()
{
	f_gets(buffer, PARSER_BUFF_SIZE, &beatmapFile);
	strtok(buffer, ",");
	beatLength = atoi(strtok(NULL, ","));

	sliderSpeed = (double)sliderMultiplier * (double)PLAY_AREA_SCALER / (double)beatLength;
}

static void parse_slider()
{
	char objectParamsBuffer[PARSER_BUFF_SIZE];

	object.type = 1;

	strcpy(objectParamsBuffer, strtok(NULL, ","));
	object.curveType = objectParamsBuffer[0];
	object.slides = atoi(strtok(NULL, ","));
	object.length = atoi(strtok(NULL, ","));
	//object.edgeSounds = strtok(NULL, ",");
	//object.edgeSets = strtok(NULL, ",");

	//xil_printf("\r\n");
	//xil_printf("              >> %s\r\n", objectParamsBuffer);

	char *pObjectParams, *point, *temp;
	pObjectParams = strdup(objectParamsBuffer);
	temp = pObjectParams;

	object.curveNumPoints = 0;

	strsep(&temp, "|");

	while ((point = strsep(&temp, "|")) != NULL) {
		int x = 0;
		int y = 0;
		sscanf(point, "%d:%d", &x, &y);

		//osupixel is equivalent to 640x480
		Node_t *currNode = ll_append(&object.curvePointsTail, sizeof(CurvePoint));
		if (currNode == NULL)
			break;

		CurvePoint *point = (CurvePoint *)currNode->data;

		point->x = x * PLAY_AREA_SCALER + PLAY_AREA_OFFSET_X;
		point->y = y * PLAY_AREA_SCALER + PLAY_AREA_OFFSET_Y;

		if (object.curveNumPoints == 0)
			object.curvePointsHead = object.curvePointsTail;

		object.curveNumPoints++;
	}

	free(pObjectParams);

	int sv = 1; // slider velocity multiplier given by the effective inherited timing point (assumed 1 until we parse them)
	object.endTime = object.length * beatLength / (sliderMultiplier * sv) + object.time;

	//xil_printf("\r\n");
}

static void print_object(HitObject object)
{
	if (object.newCombo) {
		xil_printf("New Combo >> ");
	}

	if (object.type == 0) {
		xil_printf("Hit Circle - Position: [%d,%d] Time: [%d]\r\n", object.x, object.y, object.time);
	} else if (object.type == 1) {
		xil_printf("Slider - Position: [%d,%d] Time: [%d] CT:[%c] S:[%d] L:[%d] \r\n", object.x, object.y, object.time, object.curveType,
			object.slides, object.length);
		xil_printf("\t\t>>>> From Points: (%d,%d)", object.x, object.y);

		Node_t *currNode = object.curvePointsHead;
		for (int i = 0; i < object.curveNumPoints; ++i) {
			CurvePoint *point = (CurvePoint *)currNode->data;
			xil_printf("-(%d,%d)", point->x, point->y);
			currNode = currNode->next;
		}
		xil_printf("\r\n");
	} else if (object.type == 3) {
		xil_printf("Spinner - Position: [%d,%d] Time: [%d]\r\n", object.x, object.y, object.time);
	} else {
		xil_printf("Unknown Hit Object\r\n");
	}
}

static void parse_objects()
{
	while (f_gets(buffer, PARSER_BUFF_SIZE, &beatmapFile) != NULL) {

		u8 temp_type = 0xFF;

		//osupixel is equivalent to 640x480
		object.x = atoi(strtok(buffer, ",")) * PLAY_AREA_SCALER + PLAY_AREA_OFFSET_X;
		object.y = atoi(strtok(NULL, ",")) * PLAY_AREA_SCALER + PLAY_AREA_OFFSET_Y;
		object.time = atoi(strtok(NULL, ","));
		temp_type = atoi(strtok(NULL, ","));
		object.hitSound = atoi(strtok(NULL, ","));
		object.curvePointsHead = NULL;
		object.curvePointsTail = NULL;

		//bit 2 - New Combo
		if ((temp_type & 0b00000100) == 0b00000100) {
			object.newCombo = 1;
		} else {
			object.newCombo = 0;
		}

		//bit 0 - Hit Circle
		if ((temp_type & 0b00000001) == 0b00000001) {
			object.type = 0;
		}
		//bit 1 - Slider
		if ((temp_type & 0b00000010) == 0b00000010) {
			parse_slider();
		}

		//bit 3 - Spinner
		if ((temp_type & 0b00001000) == 0b00001000) {
			object.type = 3;
			object.endTime = atoi(strtok(NULL, ","));
		}
		//bit 7 - osu!mania hold
		if ((temp_type & 0b10000000) == 0b10000000) {
			object.type = 7;
			xil_printf("ERROR: WHY ARE YOU HERE?\r\n");
		}

		if ((temp_type & 0b01110000) >> 4 > 0) {
			xil_printf("Skip %d combos\r\n", (temp_type & 0b01110000) >> 3);
		}

		memcpy(&gameHitobjects[numberOfHitobjects++], &object, sizeof(HitObject));
		print_object(object);
	}
}

static void parse_section()
{
	if (buffer[0] == '[') {
		xil_printf("Reading Section: %s", buffer);
		if (buffer[1] == 'D') { // Difficulty
			parse_difficulty();
		} else if (buffer[1] == 'T') { // TimingPoints
			parse_timing();
		} else if (buffer[1] == 'H') { //Hit Objects Section
			parse_objects();
		}
	}
}

HitObject *parse_beatmaps(char *filename, FATFS FS_instance)
{
	gameHitobjects = (HitObject *)malloc(512 * sizeof(HitObject));

	if (gameHitobjects == NULL) {
		xil_printf("ERROR: could not malloc memory for HitObject Array %d\r\n", sizeof(HitObject));
		return NULL;
	}

	numberOfHitobjects = 0;
	sliderMultiplier = 0;
	beatLength = 0;


	xil_printf("Parsing beatmap: %s\n\r", filename);

	FRESULT result;
	result = f_mount(&FS_instance, "", 1);
	if (result != 0) {
		xil_printf("Couldn't mount SD Card.\r\n");
		free(gameHitobjects);
		return NULL;
	}

	result = f_open(&beatmapFile, filename, FA_READ);
	if (result != 0) {
		xil_printf("File not found");
		free(gameHitobjects);
		return NULL;
	}


	f_gets(buffer, 32, &beatmapFile);
	xil_printf("Top of File: %s\r\n", buffer);

	while (f_gets(buffer, 32, &beatmapFile) != NULL) {
		parse_section();
	}

	f_close(&beatmapFile);
	f_unmount("");

	return gameHitobjects;
}

void free_hitobjects(HitObject *gameHitobjects)
{
	free(gameHitobjects);


}
