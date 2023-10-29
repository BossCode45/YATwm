#include "IPC.h"
#include "ewmh.h"

#include <cstring>
#include <string>
#include <sys/socket.h>
#include <iostream>
#include <unistd.h>

using std::cout, std::endl;

static const char* path = "/tmp/YATwm.sock";

IPCModule::IPCModule(CommandsModule& commandsModule, Config& cfg, Globals& globals)
	:commandsModule(commandsModule),
	 cfg(cfg),
	 globals(globals)
{
	sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
	address.sun_family = AF_UNIX;
	strcpy(address.sun_path, path);
	unlink(address.sun_path);
	len = strlen(address.sun_path) + sizeof(address.sun_family);

	if(bind(sockfd, (sockaddr*)&address, len) == -1)
	{
		cout << "ERROR " << errno << endl;
	}
	cout << "SOCKETED" << endl;
}

void IPCModule::init()
{
	setIPCPath((unsigned char*)path, strlen(path));
}

void IPCModule::doListen()
{
	if(listen(sockfd, 1) != 0)
	{
		cout << "ERROR 2" << endl;
		return;
	}
	if(first)
	{
		first = false;
		return;
	}
	cout << "DOLISTEN" << endl;
	unsigned int socklen = 0;
	sockaddr_un remote;
	int newsock = accept(sockfd, (sockaddr*)&remote, &socklen);
	cout << "LISTENING" << endl;
	char buffer[256];
	memset(buffer, 0, 256);
	read(newsock, buffer, 256);
	std::string command(buffer);
	while(command[command.size() - 1] == 0 || command[command.size() - 1] == '\n')
		command = command.substr(0, command.size() - 1);
	//cout << '"' << command << '"' << endl;
	try
	{
		commandsModule.runCommand(command);
	}
	catch(Err e)
	{
		cout << e.code << " " << e.message << endl;
	}
	char* message = "RAN COMMAND";
	send(newsock, message, strlen(message), 0);
	cout << "RAN COMMAND" << endl;
	shutdown(newsock, SHUT_RDWR);
	close(newsock);
}

void IPCModule::quitIPC()
{
	close(sockfd);
}

int IPCModule::getFD()
{
	if(sockfd > 0)
		return sockfd;
	return -1;
}