#include "ewmh.h"
#include <X11/Xlib.h>
#include <string>

Display** dpy_;
Window* root_;

void initEWMH(Display** dpy, Window* root, int numWS, std::string workspaceNames[])
{
	dpy_ = dpy;
	root_ = root;

	Atom supported[] = {XInternAtom(*dpy_, "_NET_NUMBER_OF_DESKTOPS", false), XInternAtom(*dpy_, "_NET_DESKTOP_NAMES", false), XInternAtom(*dpy_, "_NET_CLIENT_LIST", false), XInternAtom(*dpy_, "_NET_CURRENT_DESKTOP", false)};
	int wsNamesLen = numWS; //For  null bytes
	for(int i = 0; i < numWS; i++)
	{
		wsNamesLen += workspaceNames[i].length();
	}
	char wsNames[wsNamesLen];
	int pos = 0;
	for(int i = 0; i < numWS; i++)
	{
		for(char toAdd : workspaceNames[i])
		{
			wsNames[pos++] = toAdd;		
		}
		wsNames[pos++] = '\0';
	}
	unsigned long numDesktops = numWS;
	Atom netSupportedAtom = XInternAtom(*dpy_, "_NET_SUPPORTED", false);
	Atom netNumDesktopsAtom = XInternAtom(*dpy_, "_NET_NUMBER_OF_DESKTOPS", false);
	Atom netDesktopNamesAtom = XInternAtom(*dpy_, "_NET_DESKTOP_NAMES", false);
	Atom XA_UTF8STRING = XInternAtom(*dpy_, "UTF8_STRING", false);
	XChangeProperty(*dpy_, *root_, netSupportedAtom, XA_ATOM, 32, PropModeReplace, (unsigned char*)supported, 3);
	XChangeProperty(*dpy_, *root_, netDesktopNamesAtom, XA_UTF8STRING, 8, PropModeReplace, (unsigned char*)&wsNames, wsNamesLen);
	XChangeProperty(*dpy_, *root_, netNumDesktopsAtom, XA_CARDINAL, 32, PropModeReplace, (unsigned char*)&numDesktops, 1);


}

void updateClientList(std::map<int, Client> clients)
{
	Atom netClientList = XInternAtom(*dpy_, "_NET_CLIENT_LIST", false);
	XDeleteProperty(*dpy_, *root_, netClientList);

	std::map<int, Client>::iterator cItr;
	for(cItr = clients.begin(); cItr != clients.end(); cItr++)
	{
		XChangeProperty(*dpy_, *root_, netClientList, XA_WINDOW, 32, PropModeAppend, (unsigned char*)&cItr->second.w, 1);
	}

}

void setWindowDesktop(Window w, int desktop)
{
	unsigned long currDesktop = desktop - 1;
	Atom netWMDesktop = XInternAtom(*dpy_, "_NET_WM_DESKTOP", false);
	XChangeProperty(*dpy_, w, netWMDesktop, XA_CARDINAL, 32, PropModeReplace, (unsigned char*)&currDesktop, 1);
}

void setCurrentDesktop(int desktop)
{
	unsigned long currDesktop = desktop - 1;
	Atom netCurrentDesktop = XInternAtom(*dpy_, "_NET_CURRENT_DESKTOP", false);
	XChangeProperty(*dpy_, *root_, netCurrentDesktop, XA_CARDINAL, 32, PropModeReplace, (unsigned char*)&currDesktop, 1);
}

int getProp(Window w, char* propName, Atom* type, unsigned char** data)
{
	Atom prop_type = XInternAtom(*dpy_, propName, false);
	int format;
	unsigned long length;
	unsigned long after;
	int status = XGetWindowProperty(*dpy_, w, prop_type,
							0L, 1L, False,
							AnyPropertyType, type, &format,
							&length, &after, data);
	return(status);
}
