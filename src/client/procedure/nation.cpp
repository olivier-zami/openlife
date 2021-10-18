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
