//
// Created by olivier on 04/08/2021.
//

#include "src/client/game.h"
openLife::system::type::Value2D_U32 mapGenSeed;
int maxSpeechPipeIndex = 0;

client::Game* game;

int main( int inArgCount, char **inArgs )
{
	game = new client::Game();

	return mainFunction( inArgCount, inArgs );
}
