#pragma once

#include <X11/X.h>
#include <X11/Xlib.h>
#include <map>
#include <string>
#include <X11/keysym.h>
#include <vector>

#include "commands.h"
#include "util.h"

struct Keybind {
	KeySym key;
	unsigned int modifiers;
	std::string command;
};

class KeybindsModule {
public:
	KeybindsModule(CommandsModule& commandsModule, Globals& globals);
	~KeybindsModule() = default;
	const void bind(const CommandArg* argv);
	const void handleKeypress(XKeyEvent e);
private:
	std::vector<Keybind> binds;
	CommandsModule& commandsModule;
	Globals& globals;
};
