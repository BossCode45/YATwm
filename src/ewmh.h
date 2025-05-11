#pragma once

#include <X11/X.h>
#include <X11/Xatom.h>

#include <map>
#include <string>
#include <vector>

#include "structs.h"
#include "config.h"
#include "util.h"

class EWMHModule
{
public:
	EWMHModule(Globals& globals, Config& cfg);
	
	void init();

	void updateClientList(std::map<int, Client> clients);

	void updateScreens(ScreenInfo* screens, int nscreens);

	void setWindowDesktop(Window w, int desktop);

	void setCurrentDesktop(int desktop);

	void setFullscreen(Window w, bool fullscreen);

	void setIPCPath(unsigned char* path, int len);

	int getProp(Window w, char* propName, Atom* type, unsigned char** data);

private:
	Globals& globals;
	Config& cfg;
};
