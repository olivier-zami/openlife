//
// Created by olivier on 18/10/2021.
//

#include "statistic.h"

#include "minorGems/game/drawUtils.h"//TODO: remove this include because graphical lib

int historyGraphLength = 100;
FloatColor timeMeasureGraphColors[ MEASURE_TIME_NUM_CATEGORIES ] =
	{ { 1, 1, 0, 1 },
	  { 0, 1, 0, 1 },
	  { 1, 0, 0, 1 } };

void addToGraph( SimpleVector<TimeMeasureRecord> *inHistory,
						double inValue[ MEASURE_TIME_NUM_CATEGORIES ] ) {
	TimeMeasureRecord r;
	r.total = 0;
	for( int i=0; i<MEASURE_TIME_NUM_CATEGORIES; i++ ) {
		r.timeMeasureAverage[i] = inValue[i];
		r.total += inValue[i];
	}

	inHistory->push_back( r );

	while( inHistory->size() > historyGraphLength ) {
		inHistory->deleteElement( 0 );
	}
}

void drawGraph( SimpleVector<TimeMeasureRecord> *inHistory,
					   doublePair inPos,
					   FloatColor inColor[MEASURE_TIME_NUM_CATEGORIES] )
{
	double max = 0;
	for( int i=0; i<inHistory->size(); i++ ) {
		double val = inHistory->getElementDirect( i ).total;
		if( val > max ) {
			max = val;
		}
	}

	setDrawColor( 0, 0, 0, 0.5 );

	double graphHeight = 40;

	drawRect( inPos.x - 2,
			  inPos.y - 2,
			  inPos.x + historyGraphLength + 2,
			  inPos.y + graphHeight + 2 );



	for( int i=0; i<inHistory->size(); i++ ) {

		for( int m=MEASURE_TIME_NUM_CATEGORIES - 1; m >= 0; m-- ) {

			double sum = 0;

			for( int n=m; n>=0; n-- ) {

				sum += inHistory->getElementDirect( i ).timeMeasureAverage[n];
			}

			FloatColor c = timeMeasureGraphColors[m];

			setDrawColor( c.r, c.g, c.b, 0.75 );

			double scaledVal = sum / max;

			drawRect( inPos.x + i,
					  inPos.y,
					  inPos.x + i + 1,
					  inPos.y + scaledVal * graphHeight );
		}
	}
}

void drawGraph( SimpleVector<double> *inHistory, doublePair inPos,
					   FloatColor inColor ) {
	double max = 0;
	for( int i=0; i<inHistory->size(); i++ ) {
		double val = inHistory->getElementDirect( i );
		if( val > max ) {
			max = val;
		}
	}

	setDrawColor( 0, 0, 0, 0.5 );

	double graphHeight = 40;

	drawRect( inPos.x - 2,
			  inPos.y - 2,
			  inPos.x + historyGraphLength + 2,
			  inPos.y + graphHeight + 2 );



	setDrawColor( inColor.r, inColor.g, inColor.b, 0.75 );
	for( int i=0; i<inHistory->size(); i++ ) {
		double val = inHistory->getElementDirect( i );

		double scaledVal = val / max;

		drawRect( inPos.x + i,
				  inPos.y,
				  inPos.x + i + 1,
				  inPos.y + scaledVal * graphHeight );
	}
}

void addToGraph( SimpleVector<double> *inHistory, double inValue )
{
	inHistory->push_back( inValue );

	while( inHistory->size() > historyGraphLength ) {
		inHistory->deleteElement( 0 );
	}
}
