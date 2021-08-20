//
// Created by olivier on 09/08/2021.
//

#include "server.h"
#include "src/server/component/channel/speech.h"
#include "src/server/component/database/gameFeatures.h"

openLife::server::component::channel::SpeechService* speechService;
openLife::server::component::database::GameFeatures* gameFeatures;

openLife::Server::Server() {}
openLife::Server::~Server() {}

void openLife::Server::init()
{
	speechService = new openLife::server::component::channel::SpeechService();
	gameFeatures = new openLife::server::component::database::GameFeatures();
}

void openLife::Server::start()
{}

int openLife::Server::initMap()
{
	return 0;
}

void openLife::Server::initSpeechService()
{

}