//
// Created by olivier on 09/08/2021.
//

#ifndef OPENLIFE_SERVER_H
#define OPENLIFE_SERVER_H

#include "src/system/_base/object/process/service.h"

#include <string>
#include <array>

namespace openLife
{
	class Server
	{
		public:
			Server();
			~Server();

			void init();
			void useService(openLife::system::object::process::Service* service = nullptr);
			void start();

			//private:
			int initMap();
			void initSpeechService();

			std::array<openLife::system::object::process::Service*, 5> service;

	};
}

#endif //OPENLIFE_SERVER_H
