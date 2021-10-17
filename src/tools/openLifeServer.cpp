//
// Created by olivier on 17/10/2021.
//

#include <cstdio>
#include <cstring>
#include "src/server/channel/ipc.h"
#include "src/server/type/entities.h"

int main(int argc, char**argv)
{
	openLife::server::type::entity::Command command;
	command.id = 0;
	memset((void*)command.name, 0, sizeof(command.name));
	memset((void*)command.parameters, 0, sizeof(command.parameters));

	openLife::server::channel::Ipc* ipcMessageManager = new openLife::server::channel::Ipc("/home/olivier/Projets/OpenLife/app/version.txt");
	if(argc > 1)
	{
		if(!strcmp("ADD", argv[1]))
		{
			command.id = 1;
			strcpy(command.name, "ADD OBJECT");
			printf("\nsend message : add object (size %li)", sizeof(command));
			ipcMessageManager->send(command);
		}
		else if(!strcmp("DEL", argv[1]))
		{
			command.id = 2;
			strcpy(command.name, "DEL OBJECT");
			printf("\nsend message : delete object (size %li)", sizeof(command));
			ipcMessageManager->send(command);
		}
		else
		{
			command.id = 0;
			printf("\nno message send ...");
		}
	}
	else
	{
		ipcMessageManager->send(command);//!for test msg must be clean by receiver
	}



	printf("\n\n");
	return 0;
}

