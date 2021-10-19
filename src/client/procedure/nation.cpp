//
// Created by olivier on 18/10/2021.
//

#include "nation.h"

#include "minorGems/util/SimpleVector.h"
#include "minorGems/game/game.h"

SimpleVector<Homeland> homelands;
SimpleVector<HomePos> homePosStack;
SimpleVector<HomePos> oldHomePosStack;

Homeland *getHomeland( int inCenterX, int inCenterY )
{
	for( int i=0; i<homelands.size(); i++ )
	{
		Homeland *h = homelands.getElement( i );

		if( h->x == inCenterX && h->y == inCenterY )
		{
			return h;
		}
	}
	return NULL;
}

void processHomePosStack()
{
	int num = homePosStack.size();
	if( num > 0 ) {
		HomePos *r = homePosStack.getElement( num - 1 );

		if( r->temporary && r->temporaryExpireETA != 0 &&
			game_getCurrentTime() > r->temporaryExpireETA ) {
			// expired
			homePosStack.deleteElement( num - 1 );
		}
	}
}

HomePos *getHomePosRecord()
{
	processHomePosStack();

	int num = homePosStack.size();
	if( num > 0 ) {
		return homePosStack.getElement( num - 1 );
	}
	else {
		return NULL;
	}
}

// returns pointer to record, NOT destroyed by caller, or NULL if
// home unknown
GridPos *getHomeLocation(
	char *outTemp, char *outTempPerson,
	const char **outTempPersonKey,
	char inAncient )
{
	*outTemp = false;

	if( inAncient ) {
		GridPos *returnPos = NULL;

		if( homePosStack.size() > 0 ) {
			HomePos *r = homePosStack.getElement( 0 );
			if( r->ancient ) {
				returnPos = &( r->pos );
			}
		}
		return returnPos;
	}


	HomePos *r = getHomePosRecord();

	// don't consider ancient marker here, if it's the only one
	if( r != NULL && ! r->ancient ) {
		*outTemp = r->temporary;
		*outTempPerson = r->tempPerson;
		*outTempPersonKey = r->tempPersonKey;
		return &( r->pos );
	}
	else {
		return NULL;
	}
}

void removeHomeLocation( int inX, int inY )
{
	for( int i=0; i<homePosStack.size(); i++ ) {
		GridPos p = homePosStack.getElementDirect( i ).pos;

		if( p.x == inX && p.y == inY ) {
			homePosStack.deleteElement( i );
			break;
		}
	}
}

void removeAllTempHomeLocations()
{
	for( int i=0; i<homePosStack.size(); i++ )
	{
		if( homePosStack.getElementDirect( i ).temporary )
		{
			homePosStack.deleteElement( i );
			i --;
			break;
		}
	}
}

void addHomeLocation( int inX, int inY )
{
	removeAllTempHomeLocations();

	removeHomeLocation( inX, inY );
	GridPos newPos = { inX, inY };
	HomePos p;
	p.pos = newPos;
	p.ancient = false;
	p.temporary = false;

	homePosStack.push_back( p );
}

// inPersonKey can be NULL for map temp locations
// enforces priority for different classes of temp home locations
char doesNewTempLocationTrumpPrevious( const char *inPersonKey )
{

	// see what our current one is
	const char *currentKey = NULL;
	char currentFound = false;

	for( int i=0; i<homePosStack.size(); i++ ) {
		if( homePosStack.getElementDirect( i ).temporary ) {
			currentKey = homePosStack.getElementDirect( i ).tempPersonKey;
			currentFound = true;
			break;
		}
	}


	if( ! currentFound ) {
		// no temp location currently
		// all new ones can replace this state
		return true;
	}

	if( getLocationKeyPriority( inPersonKey ) <=
		getLocationKeyPriority( currentKey ) ) {
		return true;
	}
	else {
		return false;
	}
}

void addTempHomeLocation( int inX, int inY,
								 char inPerson, int inPersonID,
								 LiveObject *inPersonO,
								 const char *inPersonKey )
{
	if( ! doesNewTempLocationTrumpPrevious( inPersonKey ) ) {
		// existing key has higher priority
		// don't replace with this new key
		return;
	}

	removeAllTempHomeLocations();

	GridPos newPos = { inX, inY };
	HomePos p;
	p.pos = newPos;
	p.ancient = false;
	p.temporary = true;
	// no expiration for now
	// until we drop the map
	p.temporaryExpireETA = 0;

	p.tempPerson = inPerson;

	p.personID = -1;

	p.tempPersonKey = NULL;

	if( inPerson ) {
		// person pointer does not depend on held map
		p.temporaryExpireETA = game_getCurrentTime() + 60;

		if( strcmp( inPersonKey, "expt" ) == 0 ) {
			// 3 minutes total when searching for an expert
			p.temporaryExpireETA += 120;
		}

		p.personID = inPersonID;
		p.tempPersonKey = inPersonKey;

		if( inPersonO != NULL &&
			inPersonO->currentSpeech != NULL &&
			inPersonO->speechIsOverheadLabel ) {
			// clear any old label speech
			// to make room for new label
			delete [] inPersonO->currentSpeech;
			inPersonO->currentSpeech = NULL;
			inPersonO->speechIsOverheadLabel = false;
		}
	}

	homePosStack.push_back( p );
}

void updatePersonHomeLocation( int inPersonID, int inX, int inY )
{
	for( int i=0; i<homePosStack.size(); i++ ) {
		HomePos *p = homePosStack.getElement( i );

		if( p->tempPerson && p->personID == inPersonID ) {
			p->pos.x = inX;
			p->pos.y = inY;
		}
	}
}

void addAncientHomeLocation( int inX, int inY )
{
	removeHomeLocation( inX, inY );

	// remove all ancient pos
	// there can be only one ancient
	for( int i=0; i<homePosStack.size(); i++ ) {
		if( homePosStack.getElementDirect( i ).ancient ) {
			homePosStack.deleteElement( i );
			i--;
		}
	}

	GridPos newPos = { inX, inY };
	HomePos p;
	p.pos = newPos;
	p.ancient = true;
	p.temporary = false;

	homePosStack.push_front( p );
}

// returns if -1 no home needs to be shown (home unknown)
// otherwise, returns 0..7 index of arrow
int getHomeDir( doublePair inCurrentPlayerPos,
					   double *outTileDistance,
					   char *outTooClose,
					   char *outTemp,
					   char *outTempPerson,
					   const char **outTempPersonKey,
					   int inIndex)// 1 for ancient marker
{
	char temporary = false;

	char tempPerson = false;
	const char *tempPersonKey;

	GridPos *p = getHomeLocation( &temporary, &tempPerson, &tempPersonKey,
								  ( inIndex == 1 ) );

	if( p == NULL ) {
		return -1;
	}

	if( outTemp != NULL ) {
		*outTemp = temporary;
	}
	if( outTempPerson != NULL ) {
		*outTempPerson = tempPerson;
	}

	if( outTooClose != NULL ) {
		*outTooClose = false;
	}

	if( outTempPersonKey != NULL ) {
		*outTempPersonKey = tempPersonKey;
	}


	doublePair homePos = { (double)p->x, (double)p->y };

	doublePair vector = sub( homePos, inCurrentPlayerPos );

	double dist = length( vector );

	if( outTileDistance != NULL ) {
		*outTileDistance = dist;
	}

	if( dist < 5 ) {
		// too close

		if( outTooClose != NULL ) {
			*outTooClose = true;
		}

		if( dist == 0 ) {
			// can't compute angle
			return -1;
		}
	}


	double a = angle( vector );

	// north is 0
	a -= M_PI / 2;


	if( a <  - M_PI / 8 ) {
		a += 2 * M_PI;
	}

	int index = lrint( 8 * a / ( 2 * M_PI ) );

	return index;
}
