#pragma once

#include <vector>
#include <string>

#define evNames {0, 0, "KeyPress", "KeyRelease", "ButtonPress", "ButtonRelease", "MotionNotify", "EnterNotify", "LeaveNotify", "FocusIn", "FocusOut", "KeymapNotify", "Expose", "GraphicsExpose", "NoExpose", "VisibilityNotify", "CreateNotify", "DestroyNotify", "UnmapNotify", "MapNotify", "MapRequest", "ReparentNotify", "ConfigureNotify", "ConfigureRequest", "GravityNotify", "ResizeRequest", "CirculateNotify", "CirculateRequest", "PropertyNotify", "SelectionClear", "SelectionRequest", "SelectionNotify", "ColormapNotify", "ClientMessage", "MappingNotify", "GenericEvent", "LASTEvent"};


std::vector<std::string> split (const std::string &s, char delim);
