#pragma once

#include <X11/Xlib.h>

#include <X11/extensions/Xrandr.h>
#include <string>
#include <vector>

#define noID -1

struct Client
{
	int ID;
	Window w;
	bool floating;
};

enum TileDir
{
	horizontal,
	vertical,
	noDir
};

struct Frame
{
	int ID;
	int pID;

	bool isClient;

	//If its a client (window)
	int cID;

	//If it isn't a client
	TileDir dir;
	std::vector<int> subFrameIDs;
	bool isRoot;
	std::vector<int> floatingFrameIDs;
	//int whichChildFocused = 0;
};

struct ScreenInfo
{
	std::string name;
	int x, y, w, h;
};
