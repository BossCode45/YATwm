#pragma once

#include "commands.h"
#include <X11/X.h>
#include <X11/keysym.h>

#include <string>
#include <vector>

struct Workspace
{
	std::string name;
	int* screenPreferences;
	int screenPreferencesc;
};

#define COMMAND(X)								\
	const void X (const CommandArg* argv)

class Config
{  
public:
	Config(CommandsModule& commandsModule);
	~Config();
	void free();
	
	std::vector<Err> loadFromFile(std::string path);
	std::vector<Err> reloadFile();
	// Startup
	std::string* startupBash;
	int startupBashc;

	// Main
	int gaps;
	int outerGaps;
	std::string logFile;

	// Workspaces
	std::vector<Workspace> workspaces;
	int numWS;

	// Config Commands
	COMMAND(gapsCmd);
	COMMAND(outerGapsCmd);
	COMMAND(logFileCmd);
	COMMAND(addWorkspaceCmd);

	// Keybind Commands
	COMMAND(exit);
	COMMAND(spawn_once);
	COMMAND(changeWS);
	COMMAND(wToWS);
	COMMAND(focChange);
	COMMAND(reload);
private:
	CommandsModule& commandsModule;
	bool loaded = false;
	std::string file;
};
