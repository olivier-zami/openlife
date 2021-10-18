//
// Created by olivier on 18/10/2021.
//

#include "peaceTreaty.h"

#include "src/server/server.h"//TODO comment/delete this first
#include "minorGems/util/SimpleVector.h"
//#include "minorGems/system/win32/TimeWin32.cpp"TODO: create geneic time.h
//#include "minorGems/system/unix/TimeUnix.cpp"
//#include "OneLife/gameSource/LivingLifePage.h"//TODO: handle redeclaration of typedef struct LiveObject{}LiveObject
//#include "minorGems/util/stringUtils.cpp"

extern SimpleVector<LiveObject> players;

SimpleVector<WarState> warStates;
SimpleVector<PeaceTreaty> peaceTreaties;
SimpleVector<WarPeaceMessageRecord> warPeaceRecords;

void addPeaceTreaty( int inLineageAEveID, int inLineageBEveID ) {
	PeaceTreaty *p = getMatchingTreaty( inLineageAEveID, inLineageBEveID );

	if( p != NULL ) {
		char peaceBefore = p->dirAToB && p->dirBToA;

		// maybe it has been sealed in a new direction?
		if( p->lineageAEveID == inLineageAEveID ) {
			p->dirAToB = true;
			p->dirBToABroken = false;
		}
		if( p->lineageBEveID == inLineageAEveID ) {
			p->dirBToA = true;
			p->dirBToABroken = false;
		}
		if( p->dirAToB && p->dirBToA &&
			! peaceBefore ) {
			// new peace!

			// clear any war state
			for( int i=0; i<warStates.size(); i++ ) {
				WarState *w = warStates.getElement( i );


				if( ( w->lineageAEveID == inLineageAEveID &&
					  w->lineageBEveID == inLineageBEveID )
					||
					( w->lineageAEveID == inLineageBEveID &&
					  w->lineageBEveID == inLineageAEveID ) ) {

					warStates.deleteElement( i );
					break;
				}
			}

			sendPeaceWarMessage( "PEACE",
								 false,
								 p->lineageAEveID, p->lineageBEveID );
			sendWarReportToAll();
		}
	}
	else {
		// else doesn't exist, create new unidirectional
		PeaceTreaty p = { inLineageAEveID, inLineageBEveID,
						  true, false,
						  false, false };

		peaceTreaties.push_back( p );
	}
}

void removePeaceTreaty( int inLineageAEveID, int inLineageBEveID )
{
	PeaceTreaty *p = getMatchingTreaty( inLineageAEveID, inLineageBEveID );

	char remove = false;

	char messageSent = false;

	if( p != NULL ) {
		if( p->dirAToB && p->dirBToA ) {
			// established

			// maybe it has been broken in a new direction?
			if( p->lineageAEveID == inLineageAEveID ) {
				p->dirAToBBroken = true;
			}
			if( p->lineageBEveID == inLineageAEveID ) {
				p->dirBToABroken = true;
			}

			if( p->dirAToBBroken && p->dirBToABroken ) {
				// fully broken
				// remove it
				remove = true;

				// new war!
				sendPeaceWarMessage( "WAR",
									 true,
									 p->lineageAEveID, p->lineageBEveID );
				messageSent = true;
			}
		}
		else {
			// not fully established
			// remove it

			// this means if one person says PEACE and the other
			// responds with WAR, the first person's PEACE half-way treaty
			// is canceled.  Both need to say PEACE again once WAR has been
			// mentioned
			remove = true;
		}
	}


	if( remove || p == NULL ) {
		// no treaty exists, or it will be removed

		// some elder said "WAR"
		// war state created if it doesn't exist

		char found = false;
		for( int i=0; i<warStates.size(); i++ ) {
			WarState *w = warStates.getElement( i );


			if( ( w->lineageAEveID == inLineageAEveID &&
				  w->lineageBEveID == inLineageBEveID )
				||
				( w->lineageAEveID == inLineageBEveID &&
				  w->lineageBEveID == inLineageAEveID ) ) {
				found = true;
				break;
			}
		}

		if( !found ) {
			// add new one
			WarState w = { inLineageAEveID, inLineageBEveID };
			warStates.push_back( w );

			if( ! messageSent ) {
				sendPeaceWarMessage( "WAR",
									 true,
									 inLineageAEveID, inLineageBEveID );
				messageSent = true;
			}
		}
	}

	if( messageSent ) {
		sendWarReportToAll();
	}

	if( remove ) {
		for( int i=0; i<peaceTreaties.size(); i++ ) {
			PeaceTreaty *otherP = peaceTreaties.getElement( i );

			if( otherP->lineageAEveID == p->lineageAEveID &&
				otherP->lineageBEveID == p->lineageBEveID ) {

				peaceTreaties.deleteElement( i );
				return;
			}
		}
	}
}

// may be partial
PeaceTreaty *getMatchingTreaty( int inLineageAEveID,
								int inLineageBEveID )
{

	for( int i=0; i<peaceTreaties.size(); i++ ) {
		PeaceTreaty *p = peaceTreaties.getElement( i );


		if( ( p->lineageAEveID == inLineageAEveID &&
			  p->lineageBEveID == inLineageBEveID )
			||
			( p->lineageAEveID == inLineageBEveID &&
			  p->lineageBEveID == inLineageAEveID ) ) {
			// they match a treaty.
			return p;
		}
	}
	return NULL;
}

void sendPeaceWarMessage( const char *inPeaceOrWar,
						  char inWar,
						  int inLineageAEveID, int inLineageBEveID )
{

	double curTime = Time::getCurrentTime();

	for( int i=0; i<warPeaceRecords.size(); i++ ) {
		WarPeaceMessageRecord *r = warPeaceRecords.getElement( i );

		if( inWar != r->war ) {
			continue;
		}

		if( ( r->lineageAEveID == inLineageAEveID &&
			  r->lineageBEveID == inLineageBEveID )
			||
			( r->lineageAEveID == inLineageBEveID &&
			  r->lineageBEveID == inLineageAEveID ) ) {

			if( r->t > curTime - 3 * 60 ) {
// stil fresh, last similar message happened
// less than three minutes ago
				return;
			}
			else {
// stale
// remove it
				warPeaceRecords.deleteElement( i );
				break;
			}
		}
	}
	WarPeaceMessageRecord r = { inWar, inLineageAEveID, inLineageBEveID,
								curTime };
	warPeaceRecords.push_back( r );


	const char *nameA = "NAMELESS";
	const char *nameB = "NAMELESS";

	for( int j=0; j<players.size(); j++ ) {
		LiveObject *o = players.getElement( j );

		if( ! o->error &&
			o->lineageEveID == inLineageAEveID &&
			o->familyName != NULL ) {
			nameA = o->familyName;
			break;
		}
	}
	for( int j=0; j<players.size(); j++ ) {
		LiveObject *o = players.getElement( j );

		if( ! o->error &&
			o->lineageEveID == inLineageBEveID &&
			o->familyName != NULL ) {
			nameB = o->familyName;
			break;
		}
	}

	char *message = autoSprintf( "%s BETWEEN %s**AND %s FAMILIES",
								 inPeaceOrWar,
								 nameA, nameB );

	sendGlobalMessage( message );

	delete [] message;
}

void sendWarReportToAll()
{
	char *w = getWarReportMessage();
	int len = strlen( w );

	for( int i=0; i<players.size(); i++ ) {
		LiveObject *o = players.getElement( i );

		if( ! o->error && o->connected ) {
			sendMessageToPlayer( o, w, len );
		}
	}
	delete [] w;
}

// parial treaty returned if it's requested
char isPeaceTreaty( int inLineageAEveID, int inLineageBEveID,
					PeaceTreaty **outPartialTreaty)
{
	PeaceTreaty *p = getMatchingTreaty( inLineageAEveID, inLineageBEveID );

	if( p != NULL ) {

		if( !( p->dirAToB && p->dirBToA ) ) {
			// partial treaty
			if( outPartialTreaty != NULL ) {
				*outPartialTreaty = p;
			}
			return false;
		}
		return true;
	}
	return false;
}

// result destroyed by caller
char *getWarReportMessage()
{
	SimpleVector<char> workingMessage;

	SimpleVector<int> lineageEveIDs;
	for( int i=0; i<players.size(); i++ ) {
		LiveObject *o = players.getElement( i );

		if( o->error ) {
			continue;
		}

		if( lineageEveIDs.getElementIndex( o->lineageEveID ) == -1 ) {
			lineageEveIDs.push_back( o->lineageEveID );
		}
	}

	workingMessage.appendElementString( "WR\n" );

// check each unique pair of families
	for( int a=0; a<lineageEveIDs.size(); a++ ) {
		int linA = lineageEveIDs.getElementDirect( a );
		for( int b=a+1; b<lineageEveIDs.size(); b++ ) {
			int linB = lineageEveIDs.getElementDirect( b );

			char *line = NULL;
			if( isWarState( linA, linB ) ) {
				line = autoSprintf( "%d %d war\n", linA, linB );
			}
			else if( isPeaceTreaty( linA, linB ) ) {
				line = autoSprintf( "%d %d peace\n", linA, linB );
			}
// no line if neutral
			if( line != NULL ) {
				workingMessage.appendElementString( line );
				delete [] line;
			}
		}
	}

	workingMessage.appendElementString( "#" );

	return workingMessage.getElementString();
}