#include "scratch.h"
#include "commands.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <iostream>

ScratchModule::ScratchModule(CommandsModule& commandsModule, Config& cfg, Globals& globals)
	:commandsModule(commandsModule),
	 cfg(cfg),
	 globals(globals)
{
	commandsModule.addCommand("addScratch", &ScratchModule::addScratch, 1, {STR_REST}, this);
	commandsModule.addCommand("toggleScratch", &ScratchModule::toggleScratch, 1, {STR_REST}, this);
}

bool ScratchModule::checkScratch(Window w)
{
	XTextProperty name;
	bool gotName = XGetWMName(globals.dpy, w, &name);
	if(gotName)
	{
		std::string wName(reinterpret_cast<char*>(name.value));
		std::cout << wName << std::endl;
		if(scratches.count(wName) > 0)
		{
			ScratchWindow& scratch = scratches.find(wName)->second;
			if(scratch.state != NONE)
				return false;
			scratch.w = w;
			scratch.state = OFF;
			XUnmapWindow(globals.dpy, w);
			return true;
		}
	}
	return false;
}

const void ScratchModule::addScratch(const CommandArg* argv)
{
	ScratchWindow s = {argv[0].str, 0, NONE};
	scratches.insert({(std::string)argv[0].str, s});
}

const void ScratchModule::toggleScratch(const CommandArg* argv)
{
	if(scratches.count((std::string) argv[0].str) > 0)
	{
		ScratchWindow& scratch = scratches.find((std::string)argv[0].str)->second;
		if(scratch.state == OFF)
		{
			XMapWindow(globals.dpy, scratch.w);
			XMoveWindow(globals.dpy, scratch.w, 400, 225);
			XResizeWindow(globals.dpy, scratch.w, 800, 450);
			XSetInputFocus(globals.dpy, scratch.w, RevertToNone, CurrentTime);
			scratch.state = ON;
		}
		else if(scratch.state == ON)
		{
			XUnmapWindow(globals.dpy, scratch.w);
			XSetInputFocus(globals.dpy, globals.root, RevertToNone, CurrentTime);
			scratch.state = OFF;
		}
	}
}
