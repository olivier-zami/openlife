//
// Created by olivier on 19/10/2021.
//

#ifndef OPENLIFE_CLIENT_PROCEDURE_FAMILY_H
#define OPENLIFE_CLIENT_PROCEDURE_FAMILY_H

#include "minorGems/util/SimpleVector.h"
#include "src/client/agent/player.h"

char *getRelationName(
	SimpleVector<int> *ourLin,
	SimpleVector<int> *theirLin,
	int ourID,
	int theirID,
	int ourDisplayID,
	int theirDisplayID,
	double ourAge,
	double theirAge,
	int ourEveID,
	int theirEveID );
char *getRelationName( LiveObject *inOurObject, LiveObject *inTheirObject );

#endif //OPENLIFE_CLIENT_PROCEDURE_FAMILY_H
