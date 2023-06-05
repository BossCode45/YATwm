#pragma once

#include <X11/X.h>
#include <X11/Xlib.h>
#include <map>
#include <string>
#include <X11/keysym.h>
#include <vector>

#include "commands.h"
#include "config.h"
#include "util.h"

struct Keybind {
	KeySym key;
	unsigned int modifiers;
	std::string command;
};

class KeybindsModule {
public:
	KeybindsModule(CommandsModule& commandsModule, Config& cfg, Globals& globals, void (*updateMousePos)());
	~KeybindsModule() = default;
	const void bind(const CommandArg* argv);
	const void handleKeypress(XKeyEvent e);
	const void clearKeybinds();
private:
	std::vector<Keybind> binds;
	CommandsModule& commandsModule;
	Config& cfg;
	Globals& globals;
	void (*updateMousePos)();
};
