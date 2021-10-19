//
// Created by olivier on 19/10/2021.
//

#ifndef OPENLIFE_CLIENT_SOCKET_H
#define OPENLIFE_CLIENT_SOCKET_H

namespace openLife::client
{
	class Socket
	{
		public:
			Socket();
			~Socket();
	};
}

char readServerSocketFull( int inServerSocket );

#endif //OPENLIFE_CLIENT_SOCKET_H
