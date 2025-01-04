#include <X11/X.h>
#include <X11/Xlib.h>
#include <cctype>
#include <iostream>
#include <sstream>
#include <utility>
#include <vector>
#include <regex>

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
	commandsModule.addCommand("bindmode", &KeybindsModule::bindMode, 1, {STR}, this);

	bindModes = {
		{"normal", &KeybindsModule::normalBindMode},
		{"emacs", &KeybindsModule::emacsBindMode}
	};
	bindFunc = &KeybindsModule::normalBindMode;
	
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
			XGrabKey(globals.dpy, bind.key, bind.modifiers, globals.root, false, GrabModeAsync, GrabModeAsync);
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
	Keybind k = {(KeyCode)e.keycode,  e.state & masks};
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

bool isUpper(const std::string& s) {
    return std::all_of(s.begin(), s.end(), [](unsigned char c){ return std::isupper(c); });
}

const void KeybindsModule::bindMode(const CommandArg* argv)
{
	if(bindModes.count(argv[0].str) < 1)
	{
		throw Err(CFG_ERR_KEYBIND, "Bind mode: " + string(argv[0].str) + " does not exist");
	}
	else
	{
		bindFunc = bindModes.find(argv[0].str)->second;
	}
}

Keybind KeybindsModule::getKeybind(std::string bindString)
{
	return (this->*bindFunc)(bindString);
}

const Keybind KeybindsModule::normalBindMode(string bindString)
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
			if(isUpper(key))
			{
				bind.modifiers |= ShiftMask;
			}
			KeySym s = XStringToKeysym(key.c_str());
			if(s == NoSymbol)
			{
				throw Err(CFG_ERR_KEYBIND, "Keybind '" + bindString + "' is invalid!");
			}
			bind.key = XKeysymToKeycode(globals.dpy, s);
		}
	}
	if(!bind.key)
		throw Err(CFG_ERR_KEYBIND, "Keybind '" + bindString + "' is invalid!");
	return bind;
}

const Keybind KeybindsModule::emacsBindMode(string bindString)
{
	Keybind bind;
	bind.modifiers = 0;

	const std::regex keyRegex("^(?:([CMs])-)?(?:([CMs])-)?(?:([CMs])-)?([^\\s]|(SPC|ESC|RET|))$");
	std::smatch keyMatch;
	if(std::regex_match(bindString, keyMatch, keyRegex))
	{
		for (int i = 1; i < 3; i++)
		{
			std::ssub_match modifierMatch = keyMatch[i];
			if(modifierMatch.matched)
			{
				std::string modifier = modifierMatch.str();
				if(modifier == "s")
				{
					bind.modifiers |= Mod4Mask >> 3 * cfg.swapSuperAlt;
				}
				else if(modifier == "M")
				{
					bind.modifiers |= Mod1Mask << 3 * cfg.swapSuperAlt;
				}
				else if(modifier == "C")
				{
					bind.modifiers |= ControlMask;
				}
			}
		}
	}
	KeySym keySym = XStringToKeysym(keyMatch[4].str().c_str());
	
	if(isUpper(keyMatch[4].str().c_str()) && keySym != NoSymbol)
	{
		bind.modifiers |= ShiftMask;
	}
	if(keySym == NoSymbol)
	{
		if(keyMatch[4].str() == "RET")
			keySym = XK_Return;
		else if(keyMatch[4].str() == "ESC")
			keySym = XK_Escape;
		else if(keyMatch[4].str() == "SPC")
			keySym = XK_space;
		else if(keyMatch[4].str() == "-")
			keySym = XK_minus;
		else if(keyMatch[4].str() == "+")
			keySym = XK_plus;
		else
			throw Err(CFG_ERR_KEYBIND, "Keybind '" + bindString + "' is invalid");
	}
	bind.key = XKeysymToKeycode(globals.dpy, keySym);
    
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
