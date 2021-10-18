//
// Created by olivier on 18/10/2021.
//

#include "communication.h"

#include "minorGems/util/SimpleVector.h"

SimpleVector<LocationSpeech> locationSpeech;

void clearLocationSpeech()
{
	for( int i=0; i<locationSpeech.size(); i++ ) {
		delete [] locationSpeech.getElementDirect( i ).speech;
	}
	locationSpeech.deleteAll();
}
