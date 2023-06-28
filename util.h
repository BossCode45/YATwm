#pragma once

#include <vector>
#include <string>
#include <X11/Xlib.h>


std::string getEventName(int e);


std::vector<std::string> split (const std::string &s, char delim);


struct Globals {
	Display*& dpy;
	Window& root;
};
