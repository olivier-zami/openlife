//
// Created by olivier on 09/08/2021.
//

#include "server.h"
#include "src/server/component/channel/speech.h"

server::component::channel::SpeechService* speechService;

Server::Server() {}
Server::~Server() {}

void Server::init()
{
	speechService = new server::component::channel::SpeechService();
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