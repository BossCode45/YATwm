#include "IPC.h"
#include "ewmh.h"

#include <X11/Xlib.h>
#include <cstring>
#include <string>
#include <sys/socket.h>
#include <iostream>
#include <unistd.h>

using std::cout, std::endl;

static const char* path = "/tmp/YATwm.sock";

IPCServerModule::IPCServerModule(CommandsModule& commandsModule, Config& cfg, Globals& globals)
	:commandsModule(commandsModule),
	 cfg(cfg),
	 globals(globals)
{
}

void IPCServerModule::init()
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
	setIPCPath((unsigned char*)path, strlen(path));
	ready = true;
}

void IPCServerModule::doListen()
{
	if(!ready)
		return;
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
	unsigned int socklen = 0;
	sockaddr_un remote;
	int newsock = accept(sockfd, (sockaddr*)&remote, &socklen);
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
	const char* message = "RAN COMMAND";
	send(newsock, message, strlen(message), 0);
	shutdown(newsock, SHUT_RDWR);
	close(newsock);
}

void IPCServerModule::quitIPC()
{
	if(!ready)
		return;
	close(sockfd);
	ready = false;
}

int IPCServerModule::getFD()
{
	if(!ready)
		return -1;
	if(sockfd > 0)
		return sockfd;
	return -1;
}

IPCClientModule::IPCClientModule()
{
	ready = false;
}

int IPCClientModule::init()
{
	Display* dpy = XOpenDisplay(nullptr);
	Window root = Window(DefaultRootWindow(dpy));
	Atom propName = XInternAtom(dpy, "YATWM_SOCKET_PATH", false);
	Atom propType = XInternAtom(dpy, "STRING", false);
	int format;
	unsigned long length;
	unsigned long after;
	Atom type;
	unsigned char* sockPath;
	
	if(XGetWindowProperty(dpy, root, propName, 0L, 32L, False, propType, &type, &format, &length, &after, &sockPath) != Success || type == None)
	{
		cout << "Failed to get path" << endl;
		XFree(sockPath);
		return 1;
	}

	sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
	if(sockfd == -1)
	{
		cout << "Failed to create socket" << endl;
		XFree(sockPath);
		return -1;
	}
	sockaddr_un address;
	address.sun_family = AF_UNIX;
	strcpy(address.sun_path, (const char*)sockPath);
	if(connect(sockfd, (sockaddr*) &address, sizeof(address)) == -1)
	{
		cout << "Failed connect" << endl;
		XFree(sockPath);
		return -1;
	}
	XFree(sockPath);
	XCloseDisplay(dpy);

	ready = true;
	return 0;
}

int IPCClientModule::sendMessage(const char* message, int length)
{
	if(!ready)
		return 1;
	return write(sockfd, message, length);
}

int IPCClientModule::getMessage(char* buff, int buffsize)
{
	if(!ready)
		return 1;
	return read(sockfd, buff, buffsize);
}

void IPCClientModule::quit()
{
	if(!ready)
		return;
	close(sockfd);
}
