#include <X11/X.h>
#include <X11/Xlib.h>

#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

using std::cout, std::endl;

int main()
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

	if(XGetWindowProperty(dpy, root, propName, 0L, 32L, False, propType, &type, &format, &length, &after, &sockPath) != Success)
	{
		cout << "Failed to get path" << endl;
		XFree(sockPath);
		return 1;
	}

	int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
	if(sockfd == -1)
	{
		cout << "Failed to create socket" << endl;
		XFree(sockPath);
		return 1;
	}
	sockaddr_un address;
	address.sun_family = AF_UNIX;
	strcpy(address.sun_path, (const char*)sockPath);
	if(connect(sockfd, (sockaddr*) &address, sizeof(address)) == -1)
	{
		cout << "Failed connect" << endl;
		XFree(sockPath);
		return 1;
	}
	const char* command = "echo Hello from YATmsg!!!";
	if(write(sockfd, command, 22) == -1)
	{
		cout << "Failed write" << endl;
		XFree(sockPath);
		return 1;
	}

	char recv[128];
	read(sockfd, recv, 128);
	cout << recv << endl;
	
	close(sockfd);
	XFree(sockPath);
	return 0;
}
