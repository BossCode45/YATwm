#pragma once

#include "commands.h"
#include "config.h"
#include "util.h"
#include <X11/X.h>
#include <unordered_map>
#include <string>

enum ScratchState
{
	NONE,
	OFF,
	ON
};

struct ScratchWindow
{
	std::string name;
	Window w;
	ScratchState state;
};

class ScratchModule
{
public:
	ScratchModule(CommandsModule& commandsModule, Config& cfg, Globals& globals);
	~ScratchModule() = default;
	const void addScratch(const CommandArg* argv);
	const void toggleScratch(const CommandArg* argv);
	bool checkScratch(Window w);
private:
	CommandsModule& commandsModule;
	Config& cfg;
	Globals& globals;
	std::unordered_map<std::string, ScratchWindow> scratches;
};
