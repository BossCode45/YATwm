#include "config.h"
#include "commands.h"
#include "error.h"

#include <X11/Xlib.h>

#include <fstream>
#include <ostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <fcntl.h>

//Just for testing
#include <iostream>

using std::string;

// For testing
using std::cout, std::endl;

const void Config::gapsCmd(const CommandArg* argv)
{
	gaps = argv[0].num;
}

const void Config::outerGapsCmd(const CommandArg* argv)
{
	outerGaps = argv[0].num;
}

const void Config::logFileCmd(const CommandArg* argv)
{
	logFile = argv[0].str;
}

const void Config::addWorkspaceCmd(const CommandArg* argv)
{
	int* prefs = new int[argv[1].numArr.size];
	for(int i = 0; i < argv[1].numArr.size; i++)
	{
		prefs[i] = argv[1].numArr.arr[i] - 1;
	}
	workspaces.push_back({argv[0].str, prefs, argv[1].numArr.size});
	numWS++;
}

const void Config::swapSuperAltCmd(const CommandArg* argv)
{
	swapSuperAlt ^= true;
}

Config::Config(CommandsModule& commandsModule)
	: commandsModule(commandsModule)
{
	//Register commands for config
	commandsModule.addCommand("gaps", &Config::gapsCmd, 1, {NUM}, this);
	commandsModule.addCommand("outergaps", &Config::outerGapsCmd, 1, {NUM}, this);
	commandsModule.addCommand("logfile", &Config::logFileCmd, 1, {STR_REST}, this);
	commandsModule.addCommand("addworkspace", &Config::addWorkspaceCmd, 2, {STR, NUM_ARR_REST}, this);
	commandsModule.addCommand("swapmods", &Config::swapSuperAltCmd, 0, {}, this);
}

std::vector<Err> Config::reloadFile()
{
	if(!loaded)
		return {{CFG_ERR_NON_FATAL, "Not loaded config yet"}};
	return loadFromFile(file);
}

std::vector<Err> Config::loadFromFile(std::string path)
{
	std::vector<Err> errs;

	file = path;
	
	std::ifstream config(path);
	if(!config.good())
	{
		config = std::ifstream("/etc/YATwm/config");
		errs.push_back({CFG_ERR_FATAL, "Using default config: /etc/YATwm/config"});
	}
	
	//Set defaults
	gaps = 10;
	outerGaps = 10;
	logFile = "/tmp/yatlog.txt";
	numWS = 0;
	swapSuperAlt = false;
	workspaces = std::vector<Workspace>();

	//Probably need something for workspaces and binds too...

	int line = 0;
	for(string cmd; std::getline(config, cmd);)
	{
		line++;
		if(cmd.size() == 0)
			continue;
		if(cmd.at(0) == '#')
			continue;
		try
		{
			commandsModule.runCommand(cmd);
		}
		catch (Err e)
		{
			errs.push_back({e.code, "Error in config (line " + std::to_string(line) + "): " + std::to_string(e.code) + "\n\tMessage: " + e.message});
		
		}
	}
	loaded = true;
	return errs;
}

Config::~Config()
{
	free();
}
void Config::free()
{
	if(!loaded)
		return;
	for(Workspace w : workspaces)
	{
		delete [] w.screenPreferences;
	}
}
