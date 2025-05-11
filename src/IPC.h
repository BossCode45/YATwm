#pragma once

#include <sys/socket.h>
#include <sys/un.h>

#include "commands.h"
#include "config.h"
#include "ewmh.h"
#include "util.h"

class IPCServerModule
{
public:
	IPCServerModule(Globals& globals, Config& cfg, CommandsModule& commandsModule, EWMHModule& ewmh);
	void init();
	void doListen();
	void quitIPC();
	int getFD();
private:
	Globals& globals;
	Config& cfg;
	CommandsModule& commandsModule;
	EWMHModule& ewmh;
	int sockfd;
	int len;
	bool first = true;
	bool ready = false;
	sockaddr_un address;
};

class IPCClientModule
{
public:
	IPCClientModule();

	// Returns 0 for success, 1 for X error, -1 for socket error
	int init();
	
	// Returns 0 for success, 1 for not ready, -1 for socket error
	int sendMessage(const char* message, int length);

	// Returns 0 for success, 1 for not ready, -1 for socket error
	int getMessage(char* buff, int buffsize);

	
	void quit();
private:
	bool ready;
	int sockfd;
};
