#include <X11/Xlib.h>

#include <vector>

int noID = -1;

struct Client
{
	int ID;
	Window w;
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
};
