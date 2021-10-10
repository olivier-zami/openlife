//
// Created by olivier on 04/08/2021.
//

#include "src/client/game.h"

#include "src/system/_base/init.h"
openLife::system::type::Value2D_U32 mapGenSeed;
int maxSpeechPipeIndex = 0;

client::Game* game;

int main( int inArgCount, char **inArgs )
{
	openLife::system::init();


	game = new client::Game();

	return mainFunction( inArgCount, inArgs );
}
