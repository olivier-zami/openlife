//
// Created by olivier on 18/10/2021.
//

#include "player.h"
#include "OneLife/gameSource/LivingLifePage.h"
#include "minorGems/util/stringUtils.h"

extern char isAutoClick;//from LivingPage.cpp


static int pathFindingD = 32;// should match limits on server

void LivingLifePage::computePathToDest( LiveObject *inObject )
{
	GridPos start = inObject->closestPathPos;


	int startInd = getMapIndex( start.x, start.y );

	char startBiomeBad = false;
	char startPointBad = false;

	int startPointBadBiome = -1;

	if( startInd != -1 )
	{
		// count as bad if we're not already standing on edge of bad biome
		// or in it
		startPointBad = isBadBiome( startInd );

		if( startPointBad )
		{
			startPointBadBiome = mMapBiomes[ startInd ];
		}

		if( startPointBad ||
			isBadBiome( startInd - 1 ) ||
			isBadBiome( startInd + 1 ) ||
			isBadBiome( startInd - mMapD ) ||
			isBadBiome( startInd + mMapD ) )
		{
			startBiomeBad = true;
		}

		if( isAutoClick && ! startPointBad )
		{
			// don't allow auto clicking into bad biome from good
			startBiomeBad = false;
		}
	}

	GridPos end = { inObject->xd, inObject->yd };


	char destBiomeBad = isBadBiome( getMapIndex( end.x, end.y ) );

	char ignoreBad = false;

	if( inObject->holdingID > 0 &&
		getObject( inObject->holdingID )->rideable ) {
		// ride through bad biomes without stopping at edges
		ignoreBad = true;
	}



	if( inObject->pathToDest != NULL ) {
		delete [] inObject->pathToDest;
		inObject->pathToDest = NULL;
	}



	// window around player's start position
	int numPathMapCells = pathFindingD * pathFindingD;
	char *blockedMap = new char[ numPathMapCells ];

	// assume all blocked
	memset( blockedMap, true, numPathMapCells );

	int pathOffsetX = pathFindingD/2 - start.x;
	int pathOffsetY = pathFindingD/2 - start.y;


	for( int y=0; y<pathFindingD; y++ )
	{
		int mapY = ( y - pathOffsetY ) + mMapD / 2 - mMapOffsetY;

		for( int x=0; x<pathFindingD; x++ )
		{
			int mapX = ( x - pathOffsetX ) + mMapD / 2 - mMapOffsetX;

			if( mapY >= 0 && mapY < mMapD && mapX >= 0 && mapX < mMapD )//movement must be within the map
			{
				int mapI = mapY * mMapD + mapX;

				// note that unknowns (-1) count as blocked too

				if( mMap[ mapI ] == 0
					|| ( mMap[ mapI ] != -1 && ! getObject( mMap[ mapI ] )->blocksWalking) )
				{
					blockedMap[ y * pathFindingD + x ] = false;
				}

				if( ! ignoreBad
					&& ( ! startBiomeBad || ! destBiomeBad )
					&& ! startPointBad
					&& mMapFloors[ mapI ] == 0
					&& mBadBiomeIndices.getElementIndex( mMapBiomes[ mapI ] ) != -1)
				{
					// route around bad biomes on long paths

					blockedMap[ y * pathFindingD + x ] = true;
				}
				else if( ! ignoreBad
						 && startPointBad
						 && startPointBadBiome != -1
						 && mMapFloors[ mapI ] == 0
						 && mBadBiomeIndices.getElementIndex( mMapBiomes[ mapI ] ) != -1
						 && mMapBiomes[ mapI ] != startPointBadBiome )
				{
					// crossing from one bad biome to another
					blockedMap[ y * pathFindingD + x ] = true;
				}
			}
		}
	}

	// now add extra blocked spots for wide objects
	for( int y=0; y<pathFindingD; y++ )
	{
		int mapY = ( y - pathOffsetY ) + mMapD / 2 - mMapOffsetY;

		for( int x=0; x<pathFindingD; x++ )
		{
			int mapX = ( x - pathOffsetX ) + mMapD / 2 - mMapOffsetX;

			if( mapY >= 0 && mapY < mMapD && mapX >= 0 && mapX < mMapD )
			{

				int mapI = mapY * mMapD + mapX;

				if( mMap[ mapI ] > 0 )
				{
					ObjectRecord *o = getObject( mMap[ mapI ] );

					if( o->wide ) {

						for( int dx = - o->leftBlockingRadius;
							 dx <= o->rightBlockingRadius; dx++ ) {

							int newX = x + dx;

							if( newX >=0 && newX < pathFindingD ) {
								blockedMap[ y * pathFindingD + newX ] = true;
							}
						}
					}
				}

				if(mMapBiomes[ mapI ] == 7)
				{
					//printf("\n=====>water tile should be blocked : %i", mMapBiomes[ mapI ]);
					blockedMap[ y * pathFindingD + x ] = true;
				}
			}
		}
	}


	start.x += pathOffsetX;
	start.y += pathOffsetY;

	end.x += pathOffsetX;
	end.y += pathOffsetY;

	double startTime = game_getCurrentTime();

	GridPos closestFound;

	char pathFound = false;

	if( inObject->useWaypoint )
	{
		GridPos waypoint = { inObject->waypointX, inObject->waypointY };
		waypoint.x += pathOffsetX;
		waypoint.y += pathOffsetY;

		pathFound = pathFind( pathFindingD, pathFindingD,
							  blockedMap,
							  start, waypoint, end,
							  &( inObject->pathLength ),
							  &( inObject->pathToDest ),
							  &closestFound );
		if( pathFound && inObject->pathToDest != NULL &&
			inObject->pathLength > inObject->maxWaypointPathLength ) {

			// path through waypoint too long, use waypoint as dest
			// instead
			delete [] inObject->pathToDest;
			pathFound = pathFind( pathFindingD, pathFindingD,
								  blockedMap,
								  start, waypoint,
								  &( inObject->pathLength ),
								  &( inObject->pathToDest ),
								  &closestFound );
			inObject->xd = inObject->waypointX;
			inObject->yd = inObject->waypointY;
			inObject->destTruncated = false;
		}
	}
	else
	{
		pathFound = pathFind( pathFindingD, pathFindingD,
							  blockedMap,
							  start, end,
							  &( inObject->pathLength ),
							  &( inObject->pathToDest ),
							  &closestFound );
	}


	if( pathFound && inObject->pathToDest != NULL )
	{
		printf( "Path found in %f ms\n",
				1000 * ( game_getCurrentTime() - startTime ) );

		// move into world coordinates
		for( int i=0; i<inObject->pathLength; i++ ) {
			inObject->pathToDest[i].x -= pathOffsetX;
			inObject->pathToDest[i].y -= pathOffsetY;
		}

		inObject->shouldDrawPathMarks = false;

		// up, down, left, right
		int dirsInPath[4] = { 0, 0, 0, 0 };

		for( int i=1; i<inObject->pathLength; i++ ) {
			if( inObject->pathToDest[i].x > inObject->pathToDest[i-1].x ) {
				dirsInPath[3]++;
			}
			if( inObject->pathToDest[i].x < inObject->pathToDest[i-1].x ) {
				dirsInPath[2]++;
			}
			if( inObject->pathToDest[i].y > inObject->pathToDest[i-1].y ) {
				dirsInPath[1]++;
			}
			if( inObject->pathToDest[i].y < inObject->pathToDest[i-1].y ) {
				dirsInPath[0]++;
			}
		}

		if( ( dirsInPath[0] > 1 && dirsInPath[1] > 1 )
			||
			( dirsInPath[2] > 1 && dirsInPath[3] > 1 ) ) {

			// path contains switchbacks, making in confusing without
			// path marks
			inObject->shouldDrawPathMarks = true;
		}

		GridPos aGridPos = inObject->pathToDest[0];
		GridPos bGridPos = inObject->pathToDest[1];

		doublePair aPos = { (double)aGridPos.x, (double)aGridPos.y };
		doublePair bPos = { (double)bGridPos.x, (double)bGridPos.y };

		inObject->currentMoveDirection =
				normalize( sub( bPos, aPos ) );
	}
	else
	{
		printf( "Path not found in %f ms\n",
				1000 * ( game_getCurrentTime() - startTime ) );

		if( !pathFound ) {

			inObject->closestDestIfPathFailedX =
					closestFound.x - pathOffsetX;

			inObject->closestDestIfPathFailedY =
					closestFound.y - pathOffsetY;
		}
		else {
			// degen case where start == end?
			inObject->closestDestIfPathFailedX = inObject->xd;
			inObject->closestDestIfPathFailedY = inObject->yd;
		}


	}

	inObject->currentPathStep = 0;
	inObject->numFramesOnCurrentStep = 0;
	inObject->onFinalPathStep = false;

	delete [] blockedMap;
}

void printPath( GridPos *inPath, int inLength )
{
	for( int i=0; i<inLength; i++ ) {
		printf( "(%d,%d) ", inPath[i].x, inPath[i].y );
	}
	printf( "\n" );
}

void removeDoubleBacksFromPath( GridPos **inPath, int *inLength )
{

	SimpleVector<GridPos> filteredPath;

	int dbA = -1;
	int dbB = -1;

	int longestDB = 0;

	GridPos *path = *inPath;
	int length = *inLength;

	for( int e=0; e<length; e++ ) {

		for( int f=e; f<length; f++ ) {

			if( equal( path[e],
					   path[f] ) ) {

				int dist = f - e;

				if( dist > longestDB ) {

					dbA = e;
					dbB = f;
					longestDB = dist;
				}
			}
		}
	}

	if( longestDB > 0 ) {

		printf( "Removing loop with %d elements\n",
				longestDB );

		for( int e=0; e<=dbA; e++ ) {
			filteredPath.push_back(
					path[e] );
		}

		// skip loop between

		for( int e=dbB + 1; e<length; e++ ) {
			filteredPath.push_back(
					path[e] );
		}

		*inLength = filteredPath.size();

		delete [] path;

		*inPath =
				filteredPath.getElementArray();
	}
}

double computeCurrentAgeNoOverride( LiveObject *inObj )
{
	if( inObj->finalAgeSet ) {
		return inObj->age;
	}
	else {
		return inObj->age +
			   inObj->ageRate * ( game_getCurrentTime() - inObj->lastAgeSetTime );
	}
}

double computeCurrentAge( LiveObject *inObj )
{
	if( inObj->finalAgeSet ) {
		return inObj->age;
	}
	else {
		if( inObj->tempAgeOverrideSet ) {
			double curTime = game_getCurrentTime();

			if( curTime - inObj->tempAgeOverrideSetTime < 5 ) {
				// baby cries for 5 seconds each time they speak

				// update age using clock
				return inObj->tempAgeOverride +
					   inObj->ageRate *
					   ( curTime - inObj->tempAgeOverrideSetTime );
			}
			else {
				// temp override over
				inObj->tempAgeOverrideSet = false;
			}
		}

		return computeCurrentAgeNoOverride( inObj );
	}

}

char *getDisplayObjectDescription( int inID )//TODO: put in agent::object or agent::animal npc ...
{
	ObjectRecord *o = getObject( inID );
	if( o == NULL ) {
		return NULL;
	}
	char *upper = stringToUpperCase( o->description );
	stripDescriptionComment( upper );
	return upper;
}
