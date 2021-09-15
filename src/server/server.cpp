//
// Created by olivier on 09/08/2021.
//

#include "server.h"

#include <iostream>
#include "src/server/component/channel/speech.h"
#include "src/server/component/database/gameFeatures.h"

openLife::server::component::channel::SpeechService* speechService;
openLife::server::component::database::GameFeatures* gameFeatures;

openLife::Server::Server()
{

}

openLife::Server::~Server() {}

void openLife::Server::init()
{
	//speechService = new openLife::server::component::channel::SpeechService();
	//gameFeatures = new openLife::server::component::database::GameFeatures();
}

void openLife::Server::useService(openLife::system::object::process::Service* service)
{
	std::cout << "\nSet Service  id : " << service->getId();
	std::cout << "\nInsertion of service type : " << service->getTypeId();
	this->service[service->getId()] = service;
}

void openLife::Server::start()
{
	std::cout << "\nStart Server ...";
}

int openLife::Server::initMap()
{
	return 0;
}

void openLife::Server::initSpeechService()
{

}