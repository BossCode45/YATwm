#pragma once

#include <sys/socket.h>
#include <sys/un.h>

#include "commands.h"
#include "config.h"
#include "util.h"

class IPCModule
{
public:
	IPCModule(CommandsModule& commandsModule, Config& cfg, Globals& globals);
	void init();
	void doListen();
	void quitIPC();
	int getFD();
private:
	CommandsModule& commandsModule;
	Config& cfg;
	Globals& globals;
	int sockfd;
	int len;
	bool first = true;
	sockaddr_un address;
};
