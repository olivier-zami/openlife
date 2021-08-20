//
// Created by olivier on 09/08/2021.
//

#ifndef OPENLIFE_SERVER_H
#define OPENLIFE_SERVER_H

namespace openLife
{
	class Server
	{
		public:
			Server();
			~Server();

			void init();
			void start();

			//private:
			int initMap();
			void initSpeechService();
	};
}

#endif //OPENLIFE_SERVER_H
