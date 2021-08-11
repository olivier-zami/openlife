//
// Created by olivier on 09/08/2021.
//

#include "server.h"
#include "src/server/component/channel/speech.h"
#include "src/server/component/database/gameFeatures.h"

server::component::channel::SpeechService* speechService;
server::component::database::GameFeatures* gameFeatures;

Server::Server() {}
Server::~Server() {}

void Server::init()
{
	speechService = new server::component::channel::SpeechService();
	gameFeatures = new server::component::database::GameFeatures();
}

void Server::start()
{}

int Server::initMap()
{
	return 0;
}

void Server::initSpeechService()
{

}