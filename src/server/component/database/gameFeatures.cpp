//
// Created by olivier on 09/08/2021.
//

#include "gameFeatures.h"
#include "minorGems/util/SimpleVector.h"
#include "OneLife/gameSource/GridPos.h"

SimpleVector<int> eveSecondaryLocObjectIDs;
SimpleVector<double> recentlyUsedPrimaryEvePositionTimes;
SimpleVector<GridPos> recentlyUsedPrimaryEvePositions;
SimpleVector<int> recentlyUsedPrimaryEvePositionPlayerIDs;

openLife::server::component::database::GameFeatures::GameFeatures() {}
openLife::server::component::database::GameFeatures::~GameFeatures() {}

void openLife::server::component::database::GameFeatures::deleteEveFeatures1()
{
	eveSecondaryLocObjectIDs.deleteAll();
	recentlyUsedPrimaryEvePositionTimes.deleteAll();
	recentlyUsedPrimaryEvePositions.deleteAll();
	recentlyUsedPrimaryEvePositionPlayerIDs.deleteAll();
}