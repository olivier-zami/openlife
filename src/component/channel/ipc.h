//
// Created by olivier on 17/10/2021.
//

#ifndef OPENLIFE_SERVER_CHANNEL_IPC_H
#define OPENLIFE_SERVER_CHANNEL_IPC_H

#include <sys/ipc.h>
#include <cstddef>
#include "src/server/type/entities.h"

namespace openLife::server::channel::ipc
{
	typedef struct sharedMemory
	{
		char status;
		openLife::server::type::entity::Command command;
	}SharedMemory;
}

namespace openLife::server::channel
{
	class Ipc
	{
		public:
			Ipc(const char* ipcFile);
			~Ipc();

			void send(openLife::server::type::entity::Command command);
			void read();

			bool isEmpty();

			openLife::server::type::entity::Command getOut();

		private:
			int shmId;
			key_t shmKey;
			void* shmPtr;
			size_t shmSize;
			::openLife::server::channel::ipc::SharedMemory shm;
	};
}

#endif //OPENLIFE_SERVER_CHANNEL_IPC_H
