//
// Created by olivier on 18/10/2021.
//

#ifndef OPENLIFE_SERVER_FEATURE_PEACETREATY_H
#define OPENLIFE_SERVER_FEATURE_PEACETREATY_H

#include <cstddef>

typedef struct PeaceTreaty
{
	int lineageAEveID;
	int lineageBEveID;

	// they have to say it in both directions
	// before it comes into effect
	char dirAToB;
	char dirBToA;

	// track directions of breaking it later
	char dirAToBBroken;
	char dirBToABroken;
} PeaceTreaty;

typedef struct WarState
{
	int lineageAEveID;
	int lineageBEveID;
} WarState;

typedef struct WarPeaceMessageRecord {
	char war;
	int lineageAEveID;
	int lineageBEveID;
	double t;
}WarPeaceMessageRecord;

void addPeaceTreaty( int inLineageAEveID, int inLineageBEveID );
void removePeaceTreaty( int inLineageAEveID, int inLineageBEveID );
PeaceTreaty *getMatchingTreaty( int inLineageAEveID, int inLineageBEveID );
void sendPeaceWarMessage( const char *inPeaceOrWar, char inWar, int inLineageAEveID, int inLineageBEveID );
void sendWarReportToAll();
char isPeaceTreaty( int inLineageAEveID, int inLineageBEveID, PeaceTreaty **outPartialTreaty = NULL );
char *getWarReportMessage();

#endif //OPENLIFE_SERVER_FEATURE_PEACETREATY_H
