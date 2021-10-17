//
// Created by olivier on 17/10/2021.
//

#include "ipc.h"

#include <sys/shm.h>
#include <cstdio>
#include <cstring>
#include "src/system/_base/object/entity/exception.h"

#define SHM_KEY 0x1234

openLife::server::channel::Ipc::Ipc()
{
	this->shmKey = ftok("/home/olivier/Projets/OpenLife/app/version.txt",65);
	if(this->shmKey == -1)throw new openLife::system::object::entity::Exception("ftok() shmId generation failed");

	this->shmId = shmget(this->shmKey, sizeof(this->shm), 0644|IPC_CREAT);
	if(this->shmId == -1)throw new openLife::system::object::entity::Exception("shmget failed");

	this->shmPtr = shmat(this->shmId, NULL, 0);
	if(this->shmPtr == (void*)-1)throw new openLife::system::object::entity::Exception("shmat memory attach failed");

	memset(this->shm, 0, sizeof(this->shm));
}

openLife::server::channel::Ipc::~Ipc()
{
	if (shmdt(this->shmPtr) == -1) perror("shmdt");
	if (shmctl(this->shmId, IPC_RMID, 0) == -1) perror("shmctl(RMID) failed");
}

void openLife::server::channel::Ipc::send(void *message)
{
	memcpy((void*)this->shmPtr, message, strlen((char*)message));
}

void openLife::server::channel::Ipc::read()
{
	printf("\ntrying to read message ...:\n=====>%s\n\n", (char*)this->shmPtr);
}