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

	// Main
	int gaps;
	int outerGaps;
	std::string logFile;

	// Workspaces
	std::vector<Workspace> workspaces;
	int numWS;
	bool loaded = false;

	// Binds
	bool swapSuperAlt;

	// Config Commands
	COMMAND(gapsCmd);
	COMMAND(outerGapsCmd);
	COMMAND(logFileCmd);
	COMMAND(addWorkspaceCmd);
	COMMAND(swapSuperAltCmd);

private:
	CommandsModule& commandsModule;
	std::string file;
};
