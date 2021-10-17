//
// Created by olivier on 17/10/2021.
//

#include <cstdio>
#include "src/server/channel/ipc.h"

int main(int argc, char**argv)
{
	openLife::server::channel::Ipc* ipcMessageManager = new openLife::server::channel::Ipc();
	char message[] = "new message send";
	printf("\nTry to execute commande : %s", message);

	ipcMessageManager->send(message);

	printf("\n\n");
	return 0;
}

