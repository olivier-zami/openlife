//
// Created by olivier on 18/10/2021.
//

#include "leader.h"

#include "src/server/server.h"
#include "src/server/bank/worldMap.h"
//#include "src/server/server.h"//TODO comment/delete this first
#include "minorGems/util/stringUtils.h"

extern SimpleVector<LiveObject> players;

SimpleVector<GridPos> newOwnerPos;
SimpleVector<GridPos> recentlyRemovedOwnerPos;

#define NUM_LEADERSHIP_NAMES 8
const char * leadershipNames[NUM_LEADERSHIP_NAMES][2] = { { "LORD",
																  "LADY" },
														  { "BARON",
																  "BARONESS" },
														  { "COUNT",
																  "COUNTESS" },
														  { "DUKE",
																  "DUCHESS" },
														  { "KING",
																  "QUEEN" },
														  { "EMPEROR",
																  "EMPRESS" },
														  { "HIGH EMPEROR",
																  "HIGH EMPRESS" },
														  { "SUPREME EMPEROR",
																  "SUPREME EMPRESS" } };

/**
 *
 * @param inLeader
 * @note any followers switch to following the leader of this leader exiles are passed down to followers
 */
void leaderDied( LiveObject *inLeader )
{
	char *leaderName = getLeadershipName( inLeader );

	SimpleVector<LiveObject*> oldFollowers;
	SimpleVector<LiveObject*> directFollowers;

	for( int i=0; i<players.size(); i++ ) {
		LiveObject *otherPlayer = players.getElement( i );
		if( otherPlayer != inLeader &&
			! otherPlayer->error ) {

			if( isFollower( inLeader, otherPlayer ) ) {
				oldFollowers.push_back( otherPlayer );
			}
			if( otherPlayer->followingID == inLeader->id ) {
				directFollowers.push_back( otherPlayer );
			}
		}
	}


	// somewhere else in code, we have a condition that sometimes
	// allows someone to keep following a dead and removed player
	// this condition can cause a crash when we go to look up that leader's
	// name
	//
	// check for that here and correct for it, so we don't pass that stale
	// leader on to others (or cause a crash by looking up their name).
	if( inLeader->followingID != -1 ) {
		LiveObject *higherLeader = getLiveObject( inLeader->followingID );

		if( higherLeader == NULL ||
			higherLeader->error ) {

			inLeader->followingID = -1;
		}
	}


	// if leader is following no one (they haven't picked an heir to take over)
	// have them follow their fittest direct follower now automatically
	// ignore followers that this leader sees as exiled
	// (unless we have no choice, and they're all exiled)

	if( inLeader->followingID == -1 &&
		directFollowers.size() > 0 ) {

		LiveObject *fittestFollower = directFollowers.getElementDirect( 0 );
		// -1, because lowest possible score is 0
		// we will find a non-exiled follower this way
		double fittestFitness = -1;

		for( int i=0; i<directFollowers.size(); i++ ) {
			LiveObject *otherPlayer = directFollowers.getElementDirect( i );

			if( otherPlayer->fitnessScore > fittestFitness &&
				! isExiled( inLeader, otherPlayer ) ) {

				fittestFitness = otherPlayer->fitnessScore;
				fittestFollower = otherPlayer;
			}
		}

		// if all are exiled, then we find fittest follower
		if( fittestFitness == -1 ) {
			for( int i=0; i<directFollowers.size(); i++ ) {
				LiveObject *otherPlayer = directFollowers.getElementDirect( i );

				// ignore exile status this time
				if( otherPlayer->fitnessScore > fittestFitness ) {

					fittestFitness = otherPlayer->fitnessScore;
					fittestFollower = otherPlayer;
				}
			}
		}


		inLeader->followingID = fittestFollower->id;

		// they become top of tree, following no one
		fittestFollower->followingID = -1;
		fittestFollower->followingUpdate = true;

		// inform them differently, instead of as part of
		// group of followers below
		oldFollowers.deleteElementEqualTo( fittestFollower );

		const char *pronoun = "HIS";

		if( getFemale( inLeader ) ) {
			pronoun = "HER";
		}

		char *message =
				autoSprintf( "YOUR %s HAS DIED.**"
							 "YOU HAVE INHERITED %s POSITION.",
							 leaderName, pronoun );

		sendGlobalMessage( message, fittestFollower );
		delete [] message;
	}



	// get list of people that this leader has exiled directly
	// and clear this leader off their exiled-by lists
	SimpleVector<LiveObject*> exiledByThisLeader;

	for( int i=0; i<players.size(); i++ ) {

		LiveObject *otherPlayer = players.getElement( i );

		if( otherPlayer != inLeader &&
			! otherPlayer->error ) {

			int exileIndex = otherPlayer->
					exiledByIDs.getElementIndex( inLeader->id );

			if( exileIndex != -1 ) {

				// we have this other exiled
				exiledByThisLeader.push_back( otherPlayer );

				// take ourselves off their list, we're dead
				otherPlayer->exiledByIDs.deleteElement( exileIndex );
				otherPlayer->exileUpdate = true;
			}
		}
	}


	for( int i=0; i<players.size(); i++ ) {

		LiveObject *otherPlayer = players.getElement( i );

		if( otherPlayer != inLeader &&
			! otherPlayer->error ) {

			if( otherPlayer->followingID == inLeader->id ) {
				// they were following us


				// now they follow our leader
				// (or no leader, if we had none)
				otherPlayer->followingID = inLeader->followingID;
				otherPlayer->followingUpdate = true;

				int oID = otherPlayer->id;

				// have them exile whoever we were exiling
				for( int e=0; e<exiledByThisLeader.size(); e++ ) {
					LiveObject *eO =
							exiledByThisLeader.getElementDirect( e );

					if( eO != NULL &&
						// never have them exile themselves
						eO->id != oID &&
						eO->exiledByIDs.getElementIndex( oID ) == -1 ) {
						// this follower is not already exiling this person
						eO->exiledByIDs.push_back( oID );
						eO->exileUpdate = true;
					}
				}
			}
		}
	}



	if( leaderName == NULL ) {
		// no followers to inform
		return;
	}


	char *newLeaderExplain = NULL;
	LiveObject *newLeaderO = NULL;

	if( inLeader->followingID != -1 ) {
		newLeaderO = getLiveObject( inLeader->followingID );

		char *newLeaderName = getLeadershipName( newLeaderO );
		newLeaderExplain = autoSprintf( "YOU NOW FOLLOW YOUR %s.",
										newLeaderName );
		delete [] newLeaderName;
	}


	if( newLeaderO != NULL ) {
		// have a new leader
		// before removeAllOwnership happens externally (which passes property
		// to children)
		// pass +leaderInherit owned stuff to this new leader now

		for( int i=0; i<inLeader->ownedPositions.size(); i++ ) {
			GridPos *p = inLeader->ownedPositions.getElement( i );

			int oID = getMapObject( p->x, p->y );

			if( oID <= 0 ) {
				continue;
			}

			ObjectRecord *o = getObject( oID );

			if( strstr( o->description, "+leaderInherit" ) != NULL ) {
				recentlyRemovedOwnerPos.push_back( *p );
				newOwnerPos.push_back( *p );
				newLeaderO->ownedPositions.push_back( *p );

				inLeader->ownedPositions.deleteElement( i );
			}
		}
	}



	// tell followers about our death
	for( int i=0; i<oldFollowers.size(); i++ ) {
		LiveObject *otherPlayer = oldFollowers.getElementDirect( i );


		char *secondLine;

		if( newLeaderExplain != NULL ) {
			secondLine = stringDuplicate( newLeaderExplain );
		}
		else {
			// no heir for this position.

			// who is their prime leader?
			int primeID = otherPlayer->followingID;
			while( primeID != -1 ) {
				LiveObject *primeO = getLiveObject( primeID );
				if( primeO != NULL ) {
					if( primeO->followingID != -1 ) {
						primeID = primeO->followingID;
					}
					else {
						break;
					}
				}
			}
			newLeaderO = NULL;
			if( primeID != -1 ) {
				newLeaderO = getLiveObject( primeID );
			}
			if( newLeaderO != NULL ) {
				char *primeName = getLeadershipName( newLeaderO );
				secondLine = autoSprintf( "YOUR PRIME LEADER IS NOW YOUR %s.",
										  primeName );
				delete [] primeName;
			}
			else {
				secondLine = autoSprintf( "YOU NOW HAVE NO LEADER." );
			}

		}

		char *message =
				autoSprintf( "YOUR %s HAS DIED.**"
							 "%s",
							 leaderName,
							 secondLine );

		delete [] secondLine;

		sendGlobalMessage( message, otherPlayer );
		delete [] message;


		if( newLeaderO != NULL ) {
			// give them an arrow to their new leader
			char *newLeaderName = getLeadershipName( newLeaderO );

			GridPos lPos = getPlayerPos( newLeaderO );

			char *psMessage =
					autoSprintf( "PS\n"
								 "%d/0 MY %s "
								 "*leader %d *map %d %d\n#",
								 otherPlayer->id,
								 newLeaderName,
								 newLeaderO->id,
								 lPos.x - otherPlayer->birthPos.x,
								 lPos.y - otherPlayer->birthPos.y );

			delete [] newLeaderName;

			sendMessageToPlayer( otherPlayer, psMessage, strlen( psMessage ) );
			delete [] psMessage;
		}
	}


	delete [] newLeaderExplain;
	delete [] leaderName;
}

char *getLeadershipName(
		LiveObject *nextPlayer,
		char inNoName)
{
	int level = 0;

	SimpleVector<LiveObject *>possibleLeaders;
	possibleLeaders.push_back( nextPlayer );
	SimpleVector<LiveObject *>nextLeaders;

	while( possibleLeaders.size() > 0 )
	{
		for( int p=0; p<possibleLeaders.size(); p++ ) {
			LiveObject *possibleL = possibleLeaders.getElementDirect( p );

			for( int i=0; i<players.size(); i++ ) {
				LiveObject *o = players.getElement( i );

				if( o->error ) {
					continue;
				}
				if( o->followingID == possibleL->id ) {
					nextLeaders.push_back( o );
				}
			}
		}
		possibleLeaders.deleteAll();

		if( nextLeaders.size() > 0 ) {
			level ++;

			possibleLeaders.push_back_other( &nextLeaders );
			nextLeaders.deleteAll();
		}
	}

	if( level == 0 ) {
		// no followers
		return NULL;
	}

	int gender = 0;
	if( getFemale( nextPlayer ) ) {
		gender = 1;
	}

	if( level > NUM_LEADERSHIP_NAMES ) {
		level = NUM_LEADERSHIP_NAMES;
	}

	int index = level - 1;
	const char *title = leadershipNames[index][gender];

	if( nextPlayer->name == NULL || inNoName ) {
		return stringDuplicate( title );
	}
	else {
		return autoSprintf( "%s %s", title, nextPlayer->name );
	}
}

// Recursively walks up leader tree to find out if inLeader is a leader
char isFollower( LiveObject *inLeader, LiveObject *inTestFollower )
{
	int nextID = inTestFollower->followingID;

	if( nextID > 0 ) {
		if( nextID == inLeader->id ) {
			return true;
		}

		LiveObject *next = getLiveObject( nextID );

		if( next == NULL ) {
			return false;
		}
		return isFollower( inLeader, next );
	}
	return false;
}

// does inViewer see inTarget as exiled?
char isExiled( LiveObject *inViewer, LiveObject *inTarget )
{
	for( int e=0; e< inTarget->exiledByIDs.size(); e++ ) {
		int exilerID = inTarget->exiledByIDs.getElementDirect( e );

// they have exiled us, or the person who exiled
// us is their leader
		if( exilerID == inViewer->id
			||
			isLeader( inViewer,
					  exilerID ) ) {
			return true;
		}
	}

	return false;
}

void removeAllOwnership( LiveObject *inPlayer, char inProcessInherit)
{
	double startTime = Time::getCurrentTime();
	int num = inPlayer->ownedPositions.size();

	SimpleVector<LiveObject*> heirList;


	for( int i=0; i<inPlayer->ownedPositions.size(); i++ ) {
		GridPos *p = inPlayer->ownedPositions.getElement( i );

		recentlyRemovedOwnerPos.push_back( *p );

		int oID = getMapObject( p->x, p->y );

		if( oID <= 0 ) {
			continue;
		}

		char noOtherOwners = true;

		for( int j=0; j<players.size(); j++ ) {
			LiveObject *otherPlayer = players.getElement( j );

			if( otherPlayer != inPlayer ) {
				if( isOwned( otherPlayer, *p ) ) {
					noOtherOwners = false;
					break;
				}
			}
		}


		if( noOtherOwners && inProcessInherit ) {
			// find closest relative

			LiveObject *heir = findHeir( inPlayer );

			if( heir != NULL ) {
				heir->ownedPositions.push_back( *p );
				newOwnerPos.push_back( *p );

				noOtherOwners = false;

				// only send them message about first piece of property
				// they inherit from this death (don't overwhelm them with
				// lots of messages)
				if( heirList.getElementIndex( heir ) == -1 ) {
					heirList.push_back( heir );

					const char *name = "SOMEONE";

					if( inPlayer->name != NULL ) {
						name = inPlayer->name;
					}

					char *message =
							autoSprintf( "%s JUST DIED.**"
										 "YOU INHERITED THEIR PROPERTY.",
										 name);

					sendGlobalMessage( message, heir );
					delete [] message;

					// send them a map pointer too
					message = autoSprintf( "PS\n"
										   "%d/0 MY INHERITED PROPERTY "
										   "*prop %d *map %d %d\n#",
										   heir->id,
										   0,
										   p->x - heir->birthPos.x,
										   p->y - heir->birthPos.y );
					sendMessageToPlayer( heir, message, strlen( message ) );
					delete [] message;
				}
			}
		}



		if( noOtherOwners ) {
			// last owner of p just died
			// force end transition

			endOwnership( p->x, p->y, oID );
		}
	}

	inPlayer->ownedPositions.deleteAll();

	AppLog::infoF( "Removing all ownership (%d owned) for "
				   "player %d (%s) took %lf sec",
				   num, inPlayer->id, inPlayer->email,
				   Time::getCurrentTime() - startTime );
}

char isLeader( LiveObject *inPlayer, int inPossibleLeaderID )
{
	if( inPlayer->followingID == inPossibleLeaderID ) {
		return true;
	}
	else if( inPlayer->followingID == -1 ) {
		return false;
	}
	else {
		LiveObject *leader = getLiveObject( inPlayer->followingID );
		if( leader == NULL ) {
			return false;
		}
		else {
			return isLeader( leader, inPossibleLeaderID );
		}
	}
}

char isOwned( LiveObject *inPlayer, int inX, int inY )
{
	for( int i=0; i<inPlayer->ownedPositions.size(); i++ ) {
		GridPos *p = inPlayer->ownedPositions.getElement( i );

		if( p->x == inX && p->y == inY ) {
			return true;
		}
	}
	return false;
}

char isOwned( LiveObject *inPlayer, GridPos inPos )
{
	return isOwned( inPlayer, inPos.x, inPos.y );
}

LiveObject *findHeir( LiveObject *inPlayer )
{
	LiveObject *offspring = NULL;

	if( getFemale( inPlayer ) ) {
		offspring = findFittestOffspring( inPlayer->id, inPlayer->id );
	}

	if( offspring == NULL ) {
		// no direct offspring found

		// walk up through lineage and find oldest close relative
		// oldest person who shares our mother
		// oldest person who shares our gma
		// oldest person who shares our ggma

		// start with ma
		int lineageStep = 0;

		while( offspring == NULL &&
			   lineageStep < inPlayer->lineage->size() ) {

			offspring = findFittestOffspring(
					inPlayer->lineage->getElementDirect( lineageStep ),
					inPlayer->id );

			lineageStep++;
		}
	}

	return offspring;
}

void endOwnership( int inX, int inY, int inObjectID )
{
	SimpleVector<int> *deathMarkers = getAllPossibleDeathIDs();
	for( int j=0; j<deathMarkers->size(); j++ ) {
		int deathID = deathMarkers->getElementDirect( j );
		TransRecord *t = getTrans( deathID, inObjectID );

		if( t != NULL ) {

			setMapObject( inX, inY, t->newTarget );
			break;
		}
	}
}

// find most fit offspring
LiveObject *findFittestOffspring( int inPlayerID, int inSkipID )
{
	LiveObject *fittestOffspring = NULL;
	double fittestOffspringFitness = 0;

	for( int j=0; j<players.size(); j++ ) {
		LiveObject *otherPlayer = players.getElement( j );

		if( ! otherPlayer->error &&
			otherPlayer->id != inPlayerID &&
			otherPlayer->id != inSkipID ) {

			if( otherPlayer->fitnessScore > fittestOffspringFitness ) {

				if( otherPlayer->lineage->getElementIndex( inPlayerID )
					!= -1 ) {

					// player is direct offspring of inPlayer
					// (child, grandchild, etc).
					fittestOffspring = otherPlayer;
					fittestOffspringFitness = otherPlayer->fitnessScore;
				}
			}
		}
	}
	return fittestOffspring;
}
