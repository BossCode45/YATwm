#include "ewmh.h"
#include "util.h"
#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <cstdint>
#include <cstring>
#include <string>

#include <iostream>

EWMHModule::EWMHModule(Globals& globals, Config& cfg)
	:globals(globals)
	,cfg(cfg)
{
}

void EWMHModule::init()
{
	Atom supported[] = {XInternAtom(globals.dpy, "_NET_NUMBER_OF_DESKTOPS", false), XInternAtom(globals.dpy, "_NET_DESKTOP_NAMES", false), XInternAtom(globals.dpy, "_NET_CLIENT_LIST", false), XInternAtom(globals.dpy, "_NET_CURRENT_DESKTOP", false), XInternAtom(globals.dpy, "_NET_DESKTOP_VIEWPORT", false)};
	int wsNamesLen = cfg.numWS; //For  null bytes
	for(int i = 0; i < cfg.numWS; i++)
	{
		wsNamesLen += cfg.workspaces[i].name.length();
	}
	char *wsNames = new char[wsNamesLen];
	int pos = 0;
	for(int i = 0; i < cfg.numWS; i++)
	{
		for(char toAdd : cfg.workspaces[i].name)
		{
			wsNames[pos++] = toAdd;		
		}
		wsNames[pos++] = '\0';
	}
	unsigned long numDesktops = cfg.numWS;
	Atom netSupportedAtom = XInternAtom(globals.dpy, "_NET_SUPPORTED", false);
	Atom netNumDesktopsAtom = XInternAtom(globals.dpy, "_NET_NUMBER_OF_DESKTOPS", false);
	Atom netDesktopNamesAtom = XInternAtom(globals.dpy, "_NET_DESKTOP_NAMES", false);
	Atom XA_UTF8STRING = XInternAtom(globals.dpy, "UTF8_STRING", false);
	XChangeProperty(globals.dpy, globals.root, netSupportedAtom, XA_ATOM, 32, PropModeReplace, (unsigned char*)supported, 5);
	XChangeProperty(globals.dpy, globals.root, netDesktopNamesAtom, XA_UTF8STRING, 8, PropModeReplace, (unsigned char*)wsNames, wsNamesLen);
	XChangeProperty(globals.dpy, globals.root, netNumDesktopsAtom, XA_CARDINAL, 32, PropModeReplace, (unsigned char*)&numDesktops, 1);

	delete[] wsNames;
}

void EWMHModule::updateClientList(std::map<int, Client> clients)
{
	Atom netClientList = XInternAtom(globals.dpy, "_NET_CLIENT_LIST", false);
	XDeleteProperty(globals.dpy, globals.root, netClientList);

	std::map<int, Client>::iterator cItr;
	for(cItr = clients.begin(); cItr != clients.end(); cItr++)
	{
		XChangeProperty(globals.dpy, globals.root, netClientList, XA_WINDOW, 32, PropModeAppend, (unsigned char*)&cItr->second.w, 1);
	}

}

void EWMHModule::updateScreens(ScreenInfo* screens, int nscreens)
{
	unsigned long *desktopViewports = new unsigned long[cfg.numWS*2];
	memset(desktopViewports, 0, sizeof(int)*cfg.numWS*2);

	for(int i = 0; i < cfg.numWS; i++)
	{
		for(int j = 0; j < cfg.workspaces[i].screenPreferencesc; j++)
		{
			if(cfg.workspaces[i].screenPreferences[j] < nscreens)
			{
				desktopViewports[i*2] = screens[cfg.workspaces[i].screenPreferences[j]].x;
				desktopViewports[i*2 + 1] = screens[cfg.workspaces[i].screenPreferences[j]].y;
				break;
			}
		}
	}

	Atom netDesktopViewport = XInternAtom(globals.dpy, "_NET_DESKTOP_VIEWPORT", false);
	int status = XChangeProperty(globals.dpy, globals.root, netDesktopViewport, XA_CARDINAL, 32, PropModeReplace, (unsigned char*)desktopViewports, cfg.numWS*2);

	delete[] desktopViewports;
}

void EWMHModule::setWindowDesktop(Window w, int desktop)
{
	unsigned long currDesktop = desktop - 1;
	Atom netWMDesktop = XInternAtom(globals.dpy, "_NET_WM_DESKTOP", false);
	XChangeProperty(globals.dpy, w, netWMDesktop, XA_CARDINAL, 32, PropModeReplace, (unsigned char*)&currDesktop, 1);
}

void EWMHModule::setCurrentDesktop(int desktop)
{
	unsigned long currDesktop = desktop - 1;
	Atom netCurrentDesktop = XInternAtom(globals.dpy, "_NET_CURRENT_DESKTOP", false);
	XChangeProperty(globals.dpy, globals.root, netCurrentDesktop, XA_CARDINAL, 32, PropModeReplace, (unsigned char*)&currDesktop, 1);
}

void EWMHModule::setFullscreen(Window w, bool fullscreen)
{
	Atom netWMState = XInternAtom(globals.dpy, "_NET_WM_STATE", false);
	Atom netWMStateVal;
	if(fullscreen)
		netWMStateVal = XInternAtom(globals.dpy, "_NET_WM_STATE_FULLSCREEN", false);
	else
		netWMStateVal = XInternAtom(globals.dpy, "", false);
	XChangeProperty(globals.dpy, w, netWMState, XA_ATOM, 32, PropModeReplace, (unsigned char*)&netWMStateVal, 1);

}

void EWMHModule::setIPCPath(unsigned char* path, int len)
{
	Atom socketPathAtom = XInternAtom(globals.dpy, "YATWM_SOCKET_PATH", false);
	XChangeProperty(globals.dpy, globals.root, socketPathAtom, XA_STRING, 8, PropModeReplace, path, len);
}

int EWMHModule::getProp(Window w, char* propName, Atom* type, unsigned char** data)
{
	Atom prop_type = XInternAtom(globals.dpy, propName, false);
	int format;
	unsigned long length;
	unsigned long after;
	int status = XGetWindowProperty(globals.dpy, w, prop_type,
							0L, 1L, False,
							AnyPropertyType, type, &format,
							&length, &after, data);
	return(status);
}
