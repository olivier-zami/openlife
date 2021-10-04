//
// Created by olivier on 21/09/2021.
//

#include "getMapChunk.h"

#include "src/server/server.h"

extern int lastCheckedBiome;
extern int lastCheckedBiomeX;
extern int lastCheckedBiomeY;
extern openLife::Server* server;

/**
 * @note returns properly formatted chunk message for chunk centered around x,y
 * @param inStartX
 * @param inStartY
 * @param inWidth
 * @param inHeight
 * @param inRelativeToPos
 * @param outMessageLength
 * @return
 */
unsigned char *getChunkMessage( int inStartX, int inStartY,
								int inWidth, int inHeight,
								GridPos inRelativeToPos,
								int *outMessageLength )
{
	int chunkCells = inWidth * inHeight;
	//printf("\n=====>Prepare message (%i cells)", chunkCells);

	int *chunk = new int[chunkCells];

	int *chunkBiomes = new int[chunkCells];
	int *chunkFloors = new int[chunkCells];

	int *containedStackSizes = new int[ chunkCells ];
	int **containedStacks = new int*[ chunkCells ];

	int **subContainedStackSizes = new int*[chunkCells];
	int ***subContainedStacks = new int**[chunkCells];


	int endY = inStartY + inHeight;
	int endX = inStartX + inWidth;

	if( endY < inStartY ) {
		// wrapped around in integer space
		// pull inStartY back from edge
		inStartY -= inHeight;
		endY = inStartY + inHeight;
	}
	if( endX < inStartX ) {
		// wrapped around in integer space
		// pull inStartY back from edge
		inStartX -= inWidth;
		endX = inStartX + inWidth;
	}



	timeSec_t curTime = Time::timeSec();

	// look at four corners of chunk whenever we fetch one
	dbLookTimePut( inStartX, inStartY, curTime );
	dbLookTimePut( inStartX, endY, curTime );
	dbLookTimePut( endX, inStartY, curTime );
	dbLookTimePut( endX, endY, curTime );

	for( int y=inStartY; y<endY; y++ )
	{
		int chunkY = y - inStartY;
		for( int x=inStartX; x<endX; x++ )
		{
			int chunkX = x - inStartX;

			int cI = chunkY * inWidth + chunkX;

			lastCheckedBiome = -1;

			chunk[cI] = getMapObject( x, y );

			if( lastCheckedBiome == -1 ||
				lastCheckedBiomeX != x ||
				lastCheckedBiomeY != y )
			{
				// biome wasn't checked in order to compute
				// getMapObject

				// get it ourselves

				//lastCheckedBiome = biomes[server->getWorldMap()->select( x, y )->getBiomeRecord().value];
				lastCheckedBiome = server->getWorldMap()->select(x, y)->getBiome();
				//printf("\n=====>Send last checked biome : (%i, %i) = %i\n", x, y, lastCheckedBiome);

			}
			chunkBiomes[ cI ] = lastCheckedBiome;

			chunkFloors[cI] = getMapFloor( x, y );


			int numContained;
			int *contained = NULL;

			if( chunk[cI] > 0 && getObject( chunk[cI] )->numSlots > 0 ) {
				contained = getContained( x, y, &numContained );
			}

			if( contained != NULL ) {
				containedStackSizes[cI] = numContained;
				containedStacks[cI] = contained;

				subContainedStackSizes[cI] = new int[numContained];
				subContainedStacks[cI] = new int*[numContained];

				for( int i=0; i<numContained; i++ ) {
					subContainedStackSizes[cI][i] = 0;
					subContainedStacks[cI][i] = NULL;

					if( containedStacks[cI][i] < 0 ) {
						// a sub container
						containedStacks[cI][i] *= -1;

						int numSubContained;
						int *subContained = getContained( x, y,
														  &numSubContained,
														  i + 1 );
						if( subContained != NULL ) {
							subContainedStackSizes[cI][i] = numSubContained;
							subContainedStacks[cI][i] = subContained;
						}
					}
				}
			}
			else {
				containedStackSizes[cI] = 0;
				containedStacks[cI] = NULL;
				subContainedStackSizes[cI] = NULL;
				subContainedStacks[cI] = NULL;
			}
		}

	}



	SimpleVector<unsigned char> chunkDataBuffer;

	for( int i=0; i<chunkCells; i++ )
	{

		if( i > 0 ) {
			chunkDataBuffer.appendArray( (unsigned char*)" ", 1 );
		}


		char *cell = autoSprintf( "%d:%d:%d", chunkBiomes[i],
								  hideIDForClient( chunkFloors[i] ),
								  hideIDForClient( chunk[i] ) );

		chunkDataBuffer.appendArray( (unsigned char*)cell, strlen(cell) );
		delete [] cell;

		if( containedStacks[i] != NULL ) {
			for( int c=0; c<containedStackSizes[i]; c++ ) {
				char *containedString =
						autoSprintf( ",%d",
									 hideIDForClient( containedStacks[i][c] ) );

				chunkDataBuffer.appendArray( (unsigned char*)containedString,
											 strlen( containedString ) );
				delete [] containedString;

				if( subContainedStacks[i][c] != NULL ) {

					for( int s=0; s<subContainedStackSizes[i][c]; s++ ) {

						char *subContainedString =
								autoSprintf( ":%d",
											 hideIDForClient(
													 subContainedStacks[i][c][s] ) );

						chunkDataBuffer.appendArray(
								(unsigned char*)subContainedString,
								strlen( subContainedString ) );
						delete [] subContainedString;
					}
					delete [] subContainedStacks[i][c];
				}
			}

			delete [] subContainedStackSizes[i];
			delete [] subContainedStacks[i];

			delete [] containedStacks[i];
		}
	}

	delete [] chunk;
	delete [] chunkBiomes;
	delete [] chunkFloors;

	delete [] containedStackSizes;
	delete [] containedStacks;

	delete [] subContainedStackSizes;
	delete [] subContainedStacks;



	unsigned char *chunkData = chunkDataBuffer.getElementArray();

	int compressedSize;
	unsigned char *compressedChunkData =
			zipCompress( chunkData, chunkDataBuffer.size(),
						 &compressedSize );

	char *header = autoSprintf( "MC\n%d %d %d %d\n%d %d\n#",
								inWidth, inHeight,
								inStartX - inRelativeToPos.x,
								inStartY - inRelativeToPos.y,
								chunkDataBuffer.size(),
								compressedSize );

	SimpleVector<unsigned char> buffer;
	buffer.appendArray( (unsigned char*)header, strlen( header ) );
	delete [] header;

	buffer.appendArray( compressedChunkData, compressedSize );

	delete [] chunkData;
	delete [] compressedChunkData;

	*outMessageLength = buffer.size();
	return buffer.getElementArray();
}