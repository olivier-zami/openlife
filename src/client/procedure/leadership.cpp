//
// Created by olivier on 18/10/2021.
//

#include "leadership.h"

#include "src/client/agent/player.h"
#include <cstddef>
#include "minorGems/util/stringUtils.h"
#include "minorGems/game/game.h"
#include "OneLife/gameSource/LivingLifePage.h"

extern int ourID;
extern SimpleVector<LiveObject> gameObjects;//from OneLife/gameSource/LivingLifePage.cpp

#define NUM_LEADERSHIP_NAMES 8
static const char * leadershipNameKeys[NUM_LEADERSHIP_NAMES][2] = {
	{ "lord", "lady" },
	{ "baron", "baroness" },
	{ "count", "countess" },
	{ "duke", "duchess" },
	{ "king", "queen" },
	{ "emperor", "empress" },
	{ "highEmperor", "highEmpress" },
	{ "supremeEmperor", "supremeEmpress" }};

static void prependLeadershipTag( LiveObject *inPlayer, const char *inPrefix )
{
	LiveObject *o = inPlayer;

	char *newTag;

	if( o->leadershipNameTag != NULL ) {

		newTag = autoSprintf( "%s %s",
							  inPrefix,
							  o->leadershipNameTag );

		delete [] o->leadershipNameTag;
	}
	else {
		newTag = autoSprintf( "%s", inPrefix );
	}

	o->leadershipNameTag = newTag;
}

void LivingLifePage::updateLeadership()
{
	for( int i=0; i<gameObjects.size(); i++ ) {
		LiveObject *o = gameObjects.getElement( i );

		// reset for now
		// we will rebuild these
		o->leadershipLevel = 0;
		o->highestLeaderID = -1;
		o->hasBadge = false;
		o->isExiled = false;
		o->isDubious = false;
		o->followingUs = false;
	}


	// compute leadership levels
	char change = true;

	while( change ) {
		change = false;
		for( int i=0; i<gameObjects.size(); i++ ) {
			LiveObject *o = gameObjects.getElement( i );

			if( o->followingID != -1 ) {

				LiveObject *l = getGameObject( o->followingID );

				if( l != NULL ) {
					if( l->leadershipLevel <= o->leadershipLevel ) {
						l->leadershipLevel = o->leadershipLevel + 1;
						change = true;
					}
				}
			}
		}
	}

	// now form names based on leadership levels
	for( int i=0; i<gameObjects.size(); i++ ) {
		LiveObject *o = gameObjects.getElement( i );

		if( o->leadershipNameTag != NULL ) {
			delete [] o->leadershipNameTag;
			o->leadershipNameTag = NULL;
		}

		if( o->leadershipLevel > 0 ) {

			int lv = o->leadershipLevel;

			if( lv > NUM_LEADERSHIP_NAMES ) {
				lv = NUM_LEADERSHIP_NAMES;
			}

			lv -= 1;

			int gIndex = 0;
			if( ! getObject( o->displayID )->male ) {
				gIndex = 1;
			}
			o->leadershipNameTag =
					stringDuplicate( translate( leadershipNameKeys[lv][gIndex] ) );
		}
	}

	for( int i=0; i<gameObjects.size(); i++ ) {
		LiveObject *o = gameObjects.getElement( i );

		int nextID = o->followingID;

		while( nextID != -1 ) {
			if( nextID == ourID ) {
				o->followingUs = true;
			}

			LiveObject *l = getGameObject( nextID );
			if( l != NULL ) {
				o->highestLeaderID = nextID;

				if( l->highestLeaderID != -1 ) {
					o->highestLeaderID = l->highestLeaderID;
					if( l->followingUs ) {
						o->followingUs = true;
					}
					nextID = -1;
				}
				else {
					nextID = l->followingID;
				}
			}
			else {
				nextID = -1;
			}
		}
		if( o->highestLeaderID != -1 ) {
			LiveObject *l = getGameObject( o->highestLeaderID );
			if( l != NULL ) {
				o->hasBadge = true;
				o->badgeColor = l->personalLeadershipColor;
			}
		}
		else if( o->leadershipLevel > 0 ) {
			// a leader with no other leaders above
			o->hasBadge = true;
			o->badgeColor = o->personalLeadershipColor;
		}
	}


	SimpleVector<int> ourLeadershipChain = getOurLeadershipChain();

	LiveObject *ourLiveObject = getOurLiveObject();


	// find our followers
	for( int i=0; i<gameObjects.size(); i++ ) {
		LiveObject *o = gameObjects.getElement( i );
		if( o->followingUs ) {

			prependLeadershipTag( o, translate( "follower" ) );
		}
	}



	// find exiled people.  We might see ourselves as exiled.
	for( int i=0; i<gameObjects.size(); i++ ) {
		LiveObject *o = gameObjects.getElement( i );

		// see if any of our leaders (or us) have this person exiled
		for( int e=0; e < o->exiledByIDs.size(); e++ ) {
			int eID = o->exiledByIDs.getElementDirect( e );

			if( eID == ourID ||
				ourLeadershipChain.getElementIndex( eID ) != -1 ) {

				o->isExiled = true;

				prependLeadershipTag( o, translate( "exiled" ) );
				break;
			}
		}
	}
	// now find dubious people who are following those we see as exiled
	// we can be dubious too
	for( int i=0; i<gameObjects.size(); i++ ) {
		LiveObject *o = gameObjects.getElement( i );

		if( o->isExiled ) {
			continue;
		}

		// not seen as exiled by us

		// follow their leadership chain up
		// look for exiled leaders
		int nextID = o->followingID;

		while( nextID != -1 ) {

			LiveObject *l = getGameObject( nextID );
			if( l != NULL ) {
				if( l->isExiled ) {

					o->isDubious = true;

					prependLeadershipTag( o, translate( "dubious" ) );

					break;
				}
				nextID = l->followingID;
			}
			else {
				nextID = -1;
			}
		}
	}


	// add YOUR in front of our leaders and followers, even if exiled
	for( int i=0; i<ourLeadershipChain.size(); i++ ) {

		LiveObject *l = getGameObject(
				ourLeadershipChain.getElementDirect( i ) );

		if( l != NULL ) {
			if( l->leadershipNameTag != NULL ) {

				prependLeadershipTag( l, translate( "your" ) );
			}
		}
	}
	for( int i=0; i<gameObjects.size(); i++ ) {
		LiveObject *o = gameObjects.getElement( i );
		if( o->followingUs ) {

			if( o->leadershipNameTag != NULL ) {

				prependLeadershipTag( o, translate( "your" ) );
			}
		}
	}




	// find our allies
	if( ourLiveObject->highestLeaderID != -1 ) {
		for( int i=0; i<gameObjects.size(); i++ ) {
			LiveObject *o = gameObjects.getElement( i );
			if( o != ourLiveObject &&
				o->highestLeaderID == ourLiveObject->highestLeaderID &&
				! o->isExiled &&
				! o->followingUs &&
				o->leadershipNameTag == NULL ) {

				o->leadershipNameTag = autoSprintf( "%s %s",
													translate( "your" ),
													translate( "ally" ) );
			}
		}
	}
}

SimpleVector<int> LivingLifePage::getOurLeadershipChain()
{

	SimpleVector<int> ourLeadershipChain;

	LiveObject *ourLiveObject = getOurLiveObject();

	int nextID = ourLiveObject->followingID;

	while( nextID != -1 ) {
		ourLeadershipChain.push_back( nextID );

		LiveObject *l = getGameObject( nextID );

		if( l != NULL ) {
			nextID = l->followingID;
		}
		else {
			nextID = -1;
		}
	}
	return ourLeadershipChain;
}
