#pragma once

#include <X11/X.h>
#include <X11/Xatom.h>

#include <map>
#include <string>

#include "structs.h"

void initEWMH(Display** dpy, Window* root, int numWS, const std::string workspaceNames[]);

void updateClientList(std::map<int, Client> clients);

void setWindowDesktop(Window w, int desktop);

void setCurrentDesktop(int desktop);

int getProp(Window w, char* propName, Atom* type, unsigned char** data);
