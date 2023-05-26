#include <X11/X.h>
#include <X11/Xlib.h>
#include <iostream>
#include <sstream>
#include <vector>

#include "error.h"
#include "keybinds.h"
#include "util.h"

using std::string, std::cout, std::endl;

KeybindsModule::KeybindsModule(CommandsModule& commandsModule, Display* dpy, Window root)
	:commandsModule(commandsModule)
{
	this->dpy = dpy;
	this->root = root;
	CommandArgType* bindArgs = new CommandArgType[2];
	bindArgs[0] = STR;
	bindArgs[1] = STR_REST;
	commandsModule.addCommand("bind", &KeybindsModule::bind, 2, bindArgs, this);
	commandsModule.addCommand("exit", &KeybindsModule::exit, 0, {}, this);
	exitNow = false;
}

const void KeybindsModule::handleKeypress(XKeyEvent e)
{
	if(e.same_screen!=1) return;
	//updateMousePos();
	for(Keybind bind : binds)
	{
		if(bind.modifiers == e.state && bind.key == e.keycode)
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
	for(string key : keys)
	{
		if(key == "mod")
		{
			bind.modifiers |= Mod1Mask;
		}
		else if(key == "alt")
		{
			bind.modifiers |= Mod1Mask;
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
			bind.key = XKeysymToKeycode(dpy, XStringToKeysym(key.c_str()));
			if(bind.key == NoSymbol)
			{
				throw Err(CFG_ERR_KEYBIND, "Keybind '" + string(argv[0].str) + "' is invalid!");
				continue;
			}
		}
	}
	XGrabKey(dpy, bind.key, bind.modifiers, root, false, GrabModeAsync, GrabModeAsync);
	binds.push_back(bind);
}

const void KeybindsModule::exit(const CommandArg* argv)
{
	exitNow = true;
	cout << "Exiting..." << endl;
}
