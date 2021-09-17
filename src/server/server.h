//
// Created by olivier on 09/08/2021.
//

#ifndef OPENLIFE_SERVER_H
#define OPENLIFE_SERVER_H


#include <string>
#include <array>
#include "src/system/_base/object/process/service.h"
#include "src/server/settings.h"
#include "src/server/service/database/worldMap.h"

namespace openLife
{
	class Server
	{
		public:
			Server(
					openLife::server::Settings serverSettings,
					openLife::server::settings::WorldMap worldMapSettings,
					LINEARDB3* biomeDB,
					char* anyBiomesInDB,
					openLife::system::object::store::memory::random::Biome* cachedBiome);
			~Server();

			void init();
			void useService(openLife::system::object::process::Service* service = nullptr);
			void start();
			openLife::server::service::database::WorldMap* getWorldMap();

			//private:
			int initMap();
			void initSpeechService();

			openLife::server::service::database::WorldMap* worldMap;
			std::array<openLife::system::object::process::Service*, 5> service;
	};
}

#endif //OPENLIFE_SERVER_H
