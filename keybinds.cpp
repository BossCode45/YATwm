#include <X11/X.h>
#include <X11/Xlib.h>
#include <iostream>
#include <sstream>
#include <utility>
#include <vector>

#include "commands.h"
#include "error.h"
#include "keybinds.h"
#include "util.h"

using std::string, std::cout, std::endl;

bool Keybind::operator<(const Keybind &o) const {
	if(key != o.key)
	{
		return key < o.key;
	}
	else return modifiers < o.modifiers;
}
bool Keybind::operator==(const Keybind &o) const {
	return (key == o.key && modifiers == o.modifiers);
}

KeybindsModule::KeybindsModule(CommandsModule& commandsModule, Config& cfg, Globals& globals, void (*updateMousePos)())
	:commandsModule(commandsModule),
	 globals(globals),
	 cfg(cfg)
{
	commandsModule.addCommand("bind", &KeybindsModule::bind, 2, {STR, STR_REST}, this);
	commandsModule.addCommand("quitkey", &KeybindsModule::quitKey, 1, {STR}, this);
	this->updateMousePos = updateMousePos;
	keyMaps.insert({0, std::map<Keybind, KeyFunction>()});
}

void KeybindsModule::changeMap(int newMapID)
{
	if(currentMapID == newMapID)
		return;
	if(currentMapID != 0)
		XUngrabKeyboard(globals.dpy, CurrentTime);
	XUngrabButton(globals.dpy, AnyKey, AnyModifier, globals.root);
	currentMapID = newMapID;
	if(newMapID == 0)
	{
		for(std::pair<Keybind, KeyFunction> pair : getKeymap(currentMapID))
		{
			Keybind bind = pair.first;
			KeyCode c = XKeysymToKeycode(globals.dpy, bind.key);
			XGrabKey(globals.dpy, c, bind.modifiers, globals.root, false, GrabModeAsync, GrabModeAsync);
		}
	}
	else
	{
		XGrabKeyboard(globals.dpy, globals.root, false, GrabModeAsync, GrabModeAsync, CurrentTime);
	}
}

const void KeybindsModule::handleKeypress(XKeyEvent e)
{
	if(e.same_screen!=1) return;
	// cout << "Key Pressed" << endl;
	// cout << "\tState: " << e.state << endl;
	// cout << "\tCode: " <<  XKeysymToString(XKeycodeToKeysym(globals.dpy, e.keycode, 0)) << endl;
	updateMousePos();

	const unsigned int masks = ShiftMask | ControlMask | Mod1Mask | Mod4Mask;
	Keybind k = {XLookupKeysym(&e, 0), e.state & masks};
	if(k == exitBind)
	{
		changeMap(0);
	}
	else if(getKeymap(currentMapID).count(k) > 0)
	{
		KeyFunction& c = getKeymap(currentMapID).find(k)->second;
		if(getKeymap(c.mapID).size() == 0)
		{
			commandsModule.runCommand(c.command);
			changeMap(0);
		}
		else
		{
			XUngrabButton(globals.dpy, AnyKey, AnyModifier, globals.root);
			changeMap(c.mapID);
		}
	}
	else if(std::find(std::begin(ignoredKeys), std::end(ignoredKeys), e.keycode) == std::end(ignoredKeys))
	{
		changeMap(0);
	}
}

Keybind KeybindsModule::getKeybind(string bindString)
{
	std::vector<string> keys = split(bindString, '+');
	Keybind bind;
	bind.modifiers = 0;
	for(string key : keys)
	{
		if(key == "mod")
		{
			bind.modifiers |= Mod4Mask >> 3 * cfg.swapSuperAlt;
		}
		else if(key == "alt")
		{
			bind.modifiers |= Mod1Mask << 3 * cfg.swapSuperAlt;
		}
		else if(key == "shift")
		{
			bind.modifiers |= ShiftMask;
		}
		else if(key == "control")
		{
			bind.modifiers |= ControlMask;
		}
		else
		{
			KeySym s = XStringToKeysym(key.c_str());
			bind.key = s;
			if(bind.key == NoSymbol)
			{
				throw Err(CFG_ERR_KEYBIND, "Keybind '" + bindString + "' is invalid!");
				continue;
			}
		}
	}
	return bind;
}

const void KeybindsModule::bind(const CommandArg* argv)
{
	std::vector<Err> errs = commandsModule.checkCommand(argv[1].str);
	for(Err e : errs)
	{
		if(e.code != NOERR)
		{
			e.message = "Binding fail - " + e.message;
			throw e;
		}
	}
	std::vector<string> keys = split(argv[0].str, ' ');
    int currentBindingMap = 0;
	for(int i = 0; i < keys.size() - 1; i++)
	{
		Keybind bind = getKeybind(keys[i]);
		if(getKeymap(currentBindingMap).count(bind) > 0)
		{
			currentBindingMap = getKeymap(currentBindingMap).find(bind)->second.mapID;
		}
		else
		{
			KeyFunction newMap = {"", nextKeymapID};
			keyMaps.insert({nextKeymapID, std::map<Keybind, KeyFunction>()});
			nextKeymapID++;
			getKeymap(currentBindingMap).insert({bind, newMap});
			currentBindingMap = getKeymap(currentBindingMap).find(bind)->second.mapID;
		}
	}
	Keybind bind = getKeybind(keys[keys.size() - 1]);
	if(getKeymap(currentBindingMap).count(bind) <= 0)
	{
		KeyFunction function = {argv[1].str, nextKeymapID};
		keyMaps.insert({nextKeymapID, std::map<Keybind, KeyFunction>()});
		nextKeymapID++;
		getKeymap(currentBindingMap).insert({bind, function});
		if(currentBindingMap == currentMapID)
		{
			KeyCode c = XKeysymToKeycode(globals.dpy, bind.key);
			XGrabKey(globals.dpy, c, bind.modifiers, globals.root, false, GrabModeAsync, GrabModeAsync);
		}
	}
	else
	{
		throw Err(CFG_ERR_KEYBIND, "Bind is a keymap already!");
	}
	// cout << "Added bind" << endl;
	// cout << "\t" << argv[0].str << endl;
}

const void KeybindsModule::quitKey(const CommandArg* argv)
{
	exitBind = getKeybind(argv[0].str);
}

const void KeybindsModule::clearKeybinds()
{
	XUngrabButton(globals.dpy, AnyKey, AnyModifier, globals.root);
    keyMaps = std::map<int, std::map<Keybind, KeyFunction>>();
	keyMaps.insert({0, std::map<Keybind, KeyFunction>()});
}
