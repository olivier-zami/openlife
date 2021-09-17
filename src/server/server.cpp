//
// Created by olivier on 09/08/2021.
//

#include "server.h"

#include <iostream>
#include "src/server/component/channel/speech.h"
#include "src/server/component/database/gameFeatures.h"

openLife::server::component::channel::SpeechService* speechService;
openLife::server::component::database::GameFeatures* gameFeatures;

openLife::Server::Server(
		openLife::server::Settings serverSettings,
		openLife::server::settings::WorldMap worldMapSettings,
		LINEARDB3* biomeDB,
		char* anyBiomesInDB,
		openLife::system::object::store::memory::random::Biome* cachedBiome)
{
	worldMapSettings.mapGenerator.type = serverSettings.mapGenerator.type;
	worldMapSettings.mapGenerator.sketch.filename = serverSettings.mapGenerator.sketch.filename;
	this->worldMap = new openLife::server::service::database::WorldMap(worldMapSettings);
	this->worldMap->legacy(biomeDB, anyBiomesInDB, cachedBiome);
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

openLife::server::service::database::WorldMap* openLife::Server::getWorldMap()
{
	return this->worldMap;
}

int openLife::Server::initMap()
{
	return 0;
}

void openLife::Server::initSpeechService()
{

}