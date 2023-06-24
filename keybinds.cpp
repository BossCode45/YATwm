#include <X11/X.h>
#include <X11/Xlib.h>
#include <iostream>
#include <sstream>
#include <vector>

#include "error.h"
#include "keybinds.h"
#include "util.h"

using std::string, std::cout, std::endl;

KeybindsModule::KeybindsModule(CommandsModule& commandsModule, Config& cfg, Globals& globals, void (*updateMousePos)())
	:commandsModule(commandsModule),
	 globals(globals),
	 cfg(cfg)
{
	commandsModule.addCommand("bind", &KeybindsModule::bind, 2, {STR, STR_REST}, this);
	this->updateMousePos = updateMousePos;
}

const void KeybindsModule::handleKeypress(XKeyEvent e)
{
	if(e.same_screen!=1) return;
	//cout << "Key Pressed" << endl;
	//cout << "\tState: " << e.state << endl;
	//cout << "\tCode: " <<  XKeysymToString(XKeycodeToKeysym(globals.dpy, e.keycode, 0)) << endl;
	updateMousePos();

	const unsigned int masks = ShiftMask | ControlMask | Mod1Mask | Mod4Mask;
	for(Keybind bind : binds)
	{
		if(bind.modifiers == (e.state & masks) && bind.key == XLookupKeysym(&e, 0))
		{
			commandsModule.runCommand(bind.command);
		}
	}
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
	std::vector<string> keys = split(argv[0].str, '+');
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
				throw Err(CFG_ERR_KEYBIND, "Keybind '" + string(argv[0].str) + "' is invalid!");
				continue;
			}
		}
	}
	bind.command = argv[1].str;
	KeyCode c = XKeysymToKeycode(globals.dpy, bind.key);
	XGrabKey(globals.dpy, c, bind.modifiers, globals.root, False, GrabModeAsync, GrabModeAsync);
	binds.push_back(bind);
}

const void KeybindsModule::clearKeybinds()
{
	XUngrabButton(globals.dpy, AnyKey, AnyModifier, globals.root);
	binds = std::vector<Keybind>();
}
