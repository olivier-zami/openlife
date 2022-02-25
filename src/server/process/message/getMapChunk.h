//
// Created by olivier on 21/09/2021.
//

#ifndef OPENLIFE_SERVER_PROCESS_MESSAGE_GETMAPCHUNK_H
#define OPENLIFE_SERVER_PROCESS_MESSAGE_GETMAPCHUNK_H

#include <cstdio>
#include <cstring>
#include "OneLife/gameSource/GridPos.h"
#include "minorGems/system/Time.h"
#include "minorGems/util/SimpleVector.h"
#include "OneLife/server/map.h"
#include "OneLife/gameSource/components/banks/objectBank.h"
#include "minorGems/formats/encodingUtils.h"

void dbLookTimePut( int inX, int inY, timeSec_t inTime );

// returns properly formatted chunk message for chunk in rectangle shape
// with bottom-left corner at x,y
// coordinates in message will be relative to inRelativeToPos
// note that inStartX,Y are absolute world coordinates
unsigned char *getChunkMessage( int inStartX, int inStartY,
								int inWidth, int inHeight,
								GridPos inRelativeToPos,
								int *outMessageLength );

#endif //OPENLIFE_SERVER_PROCESS_MESSAGE_GETMAPCHUNK_H
