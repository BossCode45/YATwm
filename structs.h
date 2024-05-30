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
	bool fullscreen;
};

enum TileDir
{
	horizontal,
	vertical,
	noDir
};

struct RootData
{
	std::vector<int> floatingFrameIDs;
	Window focus;
	//int workspaceNumber;
};

struct Frame
{
	int ID;
	int pID;

	bool isClient;

	// If its a client (window)
	int cID;

	// If it isn't a client
	TileDir dir;
	std::vector<int> subFrameIDs;

	// Null if not root
	RootData* rootData;
};

struct ScreenInfo
{
	std::string name;
	int x, y, w, h;
};
