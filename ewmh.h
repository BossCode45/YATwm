#pragma once

#include <X11/X.h>
#include <X11/Xatom.h>

#include <map>
#include <string>
#include <vector>

#include "structs.h"
#include "config.h"

void initEWMH(Display** dpy, Window* root, int numWS, std::vector<Workspace> workspaces);

void updateClientList(std::map<int, Client> clients);

void setWindowDesktop(Window w, int desktop);

void setCurrentDesktop(int desktop);

void setFullscreen(Window w, bool fullscreen);

int getProp(Window w, char* propName, Atom* type, unsigned char** data);
