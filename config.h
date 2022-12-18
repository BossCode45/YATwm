#pragma once

#include <X11/X.h>
#include <X11/keysym.h>

#include <string>

enum MoveDir
{
	Up,
	Right,
	Down,
	Left
};

typedef union
{
	char* str;
	int num;
	MoveDir dir;
} KeyArg;

struct KeyBind
{
	unsigned int modifiers;
	KeySym keysym;
	void(* func) (const KeyArg arg);
	KeyArg args;
};

//Keybind commands
#define KEYCOM(X) \
	void X (const KeyArg arg)
KEYCOM(exit);
KEYCOM(spawn);
KEYCOM(toggle);
KEYCOM(kill);
KEYCOM(changeWS);
KEYCOM(wToWS);
KEYCOM(focChange);
KEYCOM(wMove);
KEYCOM(bashSpawn);
KEYCOM(reload);
KEYCOM(wsDump);
KEYCOM(nextMonitor);

class Config
{   
	public:
		Config();
		~Config();
		void free();
	
		void loadFromFile(std::string path);
		// Startup
		std::string* startupBash;
		int startupBashc;

		// Main
		int gaps;
		int outerGaps;
		std::string logFile;

		// Workspaces
		int numWS;
		std::string* workspaceNames;
		int workspaceNamesc;
		int maxMonitors;
		int** screenPreferences;
		int screenPreferencesc;

		// Keybinds
		KeyBind* binds;
		int bindsc;
};
