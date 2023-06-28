#include "util.h"

#include <sstream>

using std::string;

std::vector<string> split (const string &s, char delim) {
	std::vector<string> result;
	std::stringstream ss (s);
    string item;

    while (getline (ss, item, delim)) {
        result.push_back (item);
    }

    return result;
}

const string evNames[] = {"", "", "KeyPress", "KeyRelease", "ButtonPress", "ButtonRelease", "MotionNotify", "EnterNotify", "LeaveNotify", "FocusIn", "FocusOut", "KeymapNotify", "Expose", "GraphicsExpose", "NoExpose", "VisibilityNotify", "CreateNotify", "DestroyNotify", "UnmapNotify", "MapNotify", "MapRequest", "ReparentNotify", "ConfigureNotify", "ConfigureRequest", "GravityNotify", "ResizeRequest", "CirculateNotify", "CirculateRequest", "PropertyNotify", "SelectionClear", "SelectionRequest", "SelectionNotify", "ColormapNotify", "ClientMessage", "MappingNotify", "GenericEvent", "LASTEvent"};

string getEventName(int e)
{
	return evNames[e];
}
