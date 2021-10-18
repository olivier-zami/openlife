//
// Created by olivier on 18/10/2021.
//

#ifndef OPENLIFE_LEADER_H
#define OPENLIFE_LEADER_H

#include "src/server/type/entities.h"//TODO: handle conflict possibility with LiveObject{}LiveObject
#include "src/server/feature/genetics.h"//TODO: handle getFemale outside from that file

void leaderDied( LiveObject *inLeader );
char *getLeadershipName( LiveObject *nextPlayer, char inNoName  = false );
char isFollower( LiveObject *inLeader, LiveObject *inTestFollower );
char isExiled( LiveObject *inViewer, LiveObject *inTarget );
void removeAllOwnership( LiveObject *inPlayer, char inProcessInherit = true  );
char isLeader( LiveObject *inPlayer, int inPossibleLeaderID );
char isOwned( LiveObject *inPlayer, GridPos inPos );
char isOwned( LiveObject *inPlayer, int inX, int inY );
LiveObject *findHeir( LiveObject *inPlayer );
void endOwnership( int inX, int inY, int inObjectID );
LiveObject *findFittestOffspring( int inPlayerID, int inSkipID );

#endif //OPENLIFE_LEADER_H
