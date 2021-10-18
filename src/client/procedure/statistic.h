//
// Created by olivier on 18/10/2021.
//

#ifndef OPENLIFE_CLIENT_PROCEDURE_STATISTIC_H
#define OPENLIFE_CLIENT_PROCEDURE_STATISTIC_H

#include "minorGems/util/SimpleVector.h"
#include "minorGems/game/doublePair.h"
#include "minorGems/game/gameGraphics.h"

#define MEASURE_TIME_NUM_CATEGORIES 3

typedef struct TimeMeasureRecord
{
	double timeMeasureAverage[ MEASURE_TIME_NUM_CATEGORIES ];
	double total;
} TimeMeasureRecord;

void addToGraph( SimpleVector<TimeMeasureRecord> *inHistory, double inValue[ MEASURE_TIME_NUM_CATEGORIES ] );

void drawGraph(
	SimpleVector<TimeMeasureRecord> *inHistory,
	doublePair inPos,
	FloatColor inColor[MEASURE_TIME_NUM_CATEGORIES] );

void drawGraph(
	SimpleVector<double> *inHistory,
	doublePair inPos,
	FloatColor inColor );

void addToGraph( SimpleVector<double> *inHistory, double inValue );

#endif //OPENLIFE_CLIENT_PROCEDURE_STATISTIC_H
