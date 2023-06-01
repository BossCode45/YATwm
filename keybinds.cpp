#include <X11/X.h>
#include <X11/Xlib.h>
#include <iostream>
#include <sstream>
#include <vector>

#include "error.h"
#include "keybinds.h"
#include "util.h"

using std::string, std::cout, std::endl;

KeybindsModule::KeybindsModule(CommandsModule& commandsModule, Globals& globals)
	:commandsModule(commandsModule),
	globals(globals)
{
	CommandArgType* bindArgs = new CommandArgType[2];
	bindArgs[0] = STR;
	bindArgs[1] = STR_REST;
	commandsModule.addCommand("bind", &KeybindsModule::bind, 2, bindArgs, this);
}

const void KeybindsModule::handleKeypress(XKeyEvent e)
{
	if(e.same_screen!=1) return;
	//updateMousePos();
	for(Keybind bind : binds)
	{
		if(bind.modifiers == e.state && bind.key == XKeycodeToKeysym(globals.dpy, e.keycode, 0))
		{
			commandsModule.runCommand(bind.command);
		}
	}
}

const void KeybindsModule::bind(const CommandArg* argv)
{
	Err e = commandsModule.checkCommand(argv[1].str);
	if(e.code != NOERR)
	{
		e.message = "Binding fail - " + e.message;
		throw e;
	}
	std::vector<string> keys = split(argv[0].str, '+');
	Keybind bind;
	bind.modifiers = 0;
	for(string key : keys)
	{
		if(key == "mod")
		{
			bind.modifiers |= Mod1Mask;
		}
		else if(key == "alt")
		{
			bind.modifiers |= Mod4Mask;
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
