//
// Created by olivier on 17/10/2021.
//

#include "ipc.h"

#include <sys/shm.h>
#include <cstdio>
#include <cstring>
#include "src/system/_base/object/entity/exception.h"

#define SHM_KEY 0x1234

openLife::server::channel::Ipc::Ipc(const char* ipcFile)
{
	this->shmSize = sizeof(this->shm);

	this->shmKey = ftok(ipcFile,65);
	if(this->shmKey == -1)throw new openLife::system::object::entity::Exception("ftok() shmId generation failed");

	this->shmId = shmget(this->shmKey, sizeof(this->shmSize), 0644|IPC_CREAT);
	if(this->shmId == -1)throw new openLife::system::object::entity::Exception("shmget failed");

	this->shmPtr = shmat(this->shmId, NULL, 0);
	if(this->shmPtr == (void*)-1)throw new openLife::system::object::entity::Exception("shmat memory attach failed");
}

openLife::server::channel::Ipc::~Ipc()
{
	if (shmdt(this->shmPtr) == -1) perror("shmdt");
	if (shmctl(this->shmId, IPC_RMID, 0) == -1) perror("shmctl(RMID) failed");
}

void openLife::server::channel::Ipc::send(openLife::server::type::entity::Command command)
{
	if(!((openLife::server::channel::ipc::SharedMemory*)this->shmPtr)->status)
	{
		((openLife::server::channel::ipc::SharedMemory*)this->shmPtr)->status = 1;
		((openLife::server::channel::ipc::SharedMemory*)this->shmPtr)->command = command;
		((openLife::server::channel::ipc::SharedMemory*)this->shmPtr)->status = 2;
	}
	//TODO: empiler et envoyer plus tard
}

void openLife::server::channel::Ipc::read()
{
	if(((openLife::server::channel::ipc::SharedMemory*)(this->shmPtr))->status == 2)
	{
		memcpy((void*)(&this->shm), this->shmPtr, this->shmSize);
		memset(this->shmPtr, 0, this->shmSize);
		this->shm.status = 1;
		((openLife::server::channel::ipc::SharedMemory*)(this->shmPtr))->status = 0;
	}
}

bool openLife::server::channel::Ipc::isEmpty()
{
	return (bool) !this->shm.status;
}

openLife::server::type::entity::Command openLife::server::channel::Ipc::getOut()
{
	openLife::server::type::entity::Command command = this->shm.command;
	memset((void*)&(this->shm), 0, sizeof(this->shm));
	return command;
}