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
	bool operator<(const Keybind &o) const;
	bool operator==(const Keybind &o) const;
};

struct KeyFunction
{
	std::string command;
    int mapID;
};

#define getKeymap(X) \
	keyMaps.find(X)->second

class KeybindsModule
{
public:
	KeybindsModule(CommandsModule& commandsModule, Config& cfg, Globals& globals, void (*updateMousePos)());
	~KeybindsModule() = default;
	const void bind(const CommandArg* argv);
	const void quitKey(const CommandArg* argv);
	const void handleKeypress(XKeyEvent e);
	const void clearKeybinds();
private:
	Keybind getKeybind(std::string bindString);
	void changeMap(int newMapID);
	std::map<int, std::map<Keybind, KeyFunction>> keyMaps;
	// Modifier keys to ignore when canceling a keymap
	KeyCode ignoredKeys[8] = {50, 37, 133, 64, 62, 105, 134, 108};
	int currentMapID = 0;
	int nextKeymapID = 1;
	Keybind exitBind = {42, 0x40};
	CommandsModule& commandsModule;
	Config& cfg;
	Globals& globals;
	void (*updateMousePos)();
};
