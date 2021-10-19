//
// Created by olivier on 18/10/2021.
//

#ifndef OPENLIFE_CLIENT_PROCEDURE_NATION_H
#define OPENLIFE_CLIENT_PROCEDURE_NATION_H

#include "OneLife/gameSource/GridPos.h"
#include "minorGems/game/doublePair.h"
#include "src/client/agent/player.h"
#include "src/client/procedure/leadership.h"//TODO: handle nation leader ranking in leadership

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
char doesNewTempLocationTrumpPrevious( const char *inPersonKey );
void addTempHomeLocation(
	int inX,
	int inY,
	char inPerson,
	int inPersonID,
	LiveObject *inPersonO,
	const char *inPersonKey );
void updatePersonHomeLocation( int inPersonID, int inX, int inY );
void addAncientHomeLocation( int inX, int inY );
int getHomeDir(
	doublePair inCurrentPlayerPos,
	double *outTileDistance = NULL,
	char *outTooClose = NULL,
	char *outTemp = NULL,
	char *outTempPerson = NULL,
	const char **outTempPersonKey = NULL,
	int inIndex = 0 );// 1 for ancient marker

#endif //OPENLIFE_CLIENT_PROCEDURE_NATION_H
