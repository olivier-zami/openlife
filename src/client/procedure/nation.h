//
// Created by olivier on 18/10/2021.
//

#ifndef OPENLIFE_CLIENT_PROCEDURE_NATION_H
#define OPENLIFE_CLIENT_PROCEDURE_NATION_H

#include "OneLife/gameSource/GridPos.h"

typedef struct Homeland
{
	int x, y;
	char *familyName;
} Homeland;

// most recent home at end
typedef struct {
	GridPos pos;
	char ancient;
	char temporary;
	char tempPerson;
	int personID;
	const char *tempPersonKey;
	// 0 if not set
	double temporaryExpireETA;
} HomePos;

Homeland *getHomeland( int inCenterX, int inCenterY );
void processHomePosStack();
HomePos *getHomePosRecord();
GridPos *getHomeLocation(
	char *outTemp,
	char *outTempPerson,
	const char **outTempPersonKey,
	char inAncient );
void removeHomeLocation( int inX, int inY );
void removeAllTempHomeLocations();
void addHomeLocation( int inX, int inY );

#endif //OPENLIFE_CLIENT_PROCEDURE_NATION_H
