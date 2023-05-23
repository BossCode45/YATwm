#pragma once

#include "commands.h"
#include <X11/X.h>
#include <X11/keysym.h>

#include <string>

struct Workspace
{
	std::string name;
	int* screenPreferences;
	int screenPreferencesc;
};

#define COMMAND(X) \
	const void X (const CommandArg* argv)

class Config
{  
	public:
		Config(CommandsModule& commandsModule);
		~Config();
		void free();
	
		void loadFromFile(std::string path);
		void reloadFile();
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
		std::string* workspaceNames;
		int workspaceNamesc;
		int maxMonitors;
		int** screenPreferences;
		int screenPreferencesc;

		// Config Commands
		COMMAND(gapsCmd);
		COMMAND(outerGapsCmd);
		COMMAND(logFileCmd);
		COMMAND(addWorkspaceCmd);

		// Keybind Commands
		COMMAND(exit);
		COMMAND(spawn);
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
