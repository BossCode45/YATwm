#include "config.h"
#include "commands.h"
#include "error.h"

#include <X11/Xlib.h>

#include <cstdio>
#include <cstring>
#include <fstream>
#include <ios>
#include <string>
#include <vector>
#include <sstream>
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
	memcpy(prefs, argv[1].numArr.arr, argv[1].numArr.size * sizeof(int));
	workspaces.push_back({argv[0].str, prefs, argv[1].numArr.size});
	numWS++;
}

Config::Config(CommandsModule& commandsModule)
	: commandsModule(commandsModule)
{
	//Register commands for config
	commandsModule.addCommand("gaps", &Config::gapsCmd, 1, {NUM}, this);
	commandsModule.addCommand("outergaps", &Config::outerGapsCmd, 1, {NUM}, this);
	commandsModule.addCommand("logfile", &Config::logFileCmd, 1, {STR_REST}, this);
	commandsModule.addCommand("addworkspace", &Config::addWorkspaceCmd, 2, {STR, NUM_ARR_REST}, this);
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
	//Set defaults
	gaps = 3;
	outerGaps = 3;
	logFile = "/tmp/yatlog.txt";
	numWS = 0;

	//Probably need something for workspaces and binds too...

	string cmd;
	int line = 0;
	std::ifstream config(path);
	while(getline(config, cmd))
	{
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
		line++;
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
