//
// Created by olivier on 17/10/2021.
//

#ifndef OPENLIFE_SERVER_CHANNEL_IPC_H
#define OPENLIFE_SERVER_CHANNEL_IPC_H

#include <sys/ipc.h>

namespace openLife::server::channel
{
	class Ipc
	{
		public:
			Ipc();
			~Ipc();

			void send(void* message);
			void read();

		private:
			int shmId;
			key_t shmKey;
			void* shmPtr;
			char shm[255];
	};
}

#endif //OPENLIFE_SERVER_CHANNEL_IPC_H
