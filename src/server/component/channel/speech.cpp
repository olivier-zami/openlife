//
// Created by olivier on 09/08/2021.
//

#include "speech.h"
#include "minorGems/util/SimpleVector.h"
#include "OneLife/gameSource/GridPos.h"

int numSpeechPipes = 0;
int maxSpeechPipeIndex = 0;
SimpleVector<GridPos> *speechPipesIn = NULL;
SimpleVector<GridPos> *speechPipesOut = NULL;

openLife::server::component::channel::SpeechService::SpeechService() {}
openLife::server::component::channel::SpeechService::~SpeechService() {}

void openLife::server::component::channel::SpeechService::init()
{
	numSpeechPipes = this->getMaxSpeechPipeIndex() + 1;

	speechPipesIn = new SimpleVector<GridPos>[ numSpeechPipes ];
	speechPipesOut = new SimpleVector<GridPos>[ numSpeechPipes ];
}

int openLife::server::component::channel::SpeechService::getMaxSpeechPipeIndex()
{
	return maxSpeechPipeIndex;
}