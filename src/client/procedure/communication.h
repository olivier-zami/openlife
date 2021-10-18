//
// Created by olivier on 18/10/2021.
//

#ifndef OPENLIFE_CLIENT_PROCEDURE_COMMUNICATION_H
#define OPENLIFE_CLIENT_PROCEDURE_COMMUNICATION_H

#include "minorGems/game/doublePair.h"

typedef struct LocationSpeech
{
	doublePair pos;
	char *speech;
	double fade;
	// wall clock time when speech should start fading
	double fadeETATime;
} LocationSpeech;

void clearLocationSpeech();

#endif //OPENLIFE_CLIENT_PROCEDURE_COMMUNICATION_H
