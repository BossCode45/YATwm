#include "config.h"

#include <X11/Xlib.h>

#include <string>
#include <map>

//Just for testing
#include <iostream>

#include <toml++/toml.hpp>

using std::map, std::string;

// For testing
using std::cout, std::endl, std::cerr;

void exit(const KeyArg arg)
{
	cout << "exit called" << endl;
}
void spawn(const KeyArg arg)
{
	cout << "spawn called " << arg.str << endl;
}
void changeWS(const KeyArg arg)
{
	cout << "changeWS called" << endl;
}
void wToWS(const KeyArg arg)
{
	cout << "wToWS called" << endl;
}
void focChange(const KeyArg arg)
{
	cout << "focChange called" << endl;
}
void reload(const KeyArg arg)
{
	cout << "reload called" << endl;
}

map<string, void(*) (const KeyArg arg)> funcNameMap = {
	{"exit", exit},
	{"spawn", spawn},
	{"changeWS", changeWS},
	{"wToWS", wToWS},
	{"focChange", focChange},
	{"reload", reload}
};


Config::Config()
{
}

std::vector<string> split (const string &s, char delim) {
	std::vector<string> result;
	std::stringstream ss (s);
    string item;

    while (getline (ss, item, delim)) {
        result.push_back (item);
    }

    return result;
}

void Config::loadFromFile(string path)
{
	//free();
	toml::table tbl;
	try
	{
		tbl = toml::parse_file(path);
	}
	catch (const toml::parse_error& err)
	{
		throw err;
	}

	//Startup
	startupBashc = tbl["Startup"]["startupBash"].as_array()->size();
	startupBash = new string[startupBashc];
	for(int i = 0; i < startupBashc; i++)
	{
		auto element = tbl["Startup"]["startupBash"][i].value<string>();
		if(element)
		{
			startupBash[i] = *element;
		}
		else
		{
			cerr << "Element " << i << " in `startupBash` is not a string" << endl;
			startupBash[i] = "";
		}
	}

	//Main
	gaps = tbl["Main"]["gaps"].value_or<int>(3);
	outerGaps = tbl["Main"]["gaps"].value_or<int>(3);
	logFile = tbl["Main"]["gaps"].value_or<string>("/tmp/yatlog.txt");

	//Workspaces
	numWS = tbl["Workspaces"]["numWS"].value_or<int>(10);
	workspaceNamesc = tbl["Workspaces"]["workspaceNames"].as_array()->size();
	workspaceNames = new string[workspaceNamesc];
	for(int i = 0; i < workspaceNamesc; i++)
	{
		auto element = tbl["Workspaces"]["workspaceNames"][i].value<string>();
		if(element)
		{
			workspaceNames[i] = *element;
		}
		else
		{
			cerr << "Element " << i << " in `workspaceNames` is not a string" << endl;
			workspaceNames[i] = "";
		}
	}
	maxMonitors = tbl["Workspaces"]["maxMonitors"].value_or<int>(2);
	screenPreferencesc = tbl["Workspaces"]["screenPreferences"].as_array()->size();
	screenPreferences = new int*[screenPreferencesc];
	for(int i = 0; i < screenPreferencesc; i++)
	{
		int* wsScreens = new int[maxMonitors];
		for(int j = 0; j < maxMonitors; j++)
		{
			if(tbl["Workspaces"]["screenPreferences"][i].as_array()->size() <= j)
			{
				wsScreens[j] = 0;
				continue;
			}
			auto element = tbl["Workspaces"]["screenPreferences"][i][j].value<int>();
			if(element)
				wsScreens[j] = *element;
			else
			{
				cerr << "Element " << i << " " << j << " int `screenPreferences` is not an int" << endl;
				wsScreens[j] = 0;
			}
		}
		screenPreferences[i] = wsScreens;
	}

	//Keybinds
	bindsc = tbl["Keybinds"]["key"].as_array()->size();
	binds = new KeyBind[bindsc];
	for(int i = 0; i < bindsc; i++)
	{
		KeyBind bind;
		const string bindString = *tbl["Keybinds"]["key"][i]["bind"].value<string>();
		std::vector<string> keys = split(bindString, '+');
		for(string key : keys)
		{
			if(key == "mod")
			{
				bind.modifiers |= Mod4Mask;
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
				bind.keysym = XStringToKeysym(key.c_str());
			}
		}
		string funcString = *tbl["Keybinds"]["key"][i]["func"].value<string>();
		void(* func) (const KeyArg arg) = funcNameMap.find(funcString)->second;
		bind.func = func;

		auto args = tbl["Keybinds"]["key"][i]["args"];
		if(args.is<int64_t>())
		{
			int num = *args.value<int>();
			bind.args = {.num = num};
		}
		else if(args.is<string>())
		{
			string str = (string)*args.value<string>();
			if(str == "Up")
				bind.args = {.dir = Up};
			else if (str == "Down")
				bind.args = {.dir = Down};
			else if (str == "Left")
				bind.args = {.dir = Left};
			else if (str == "Right")
				bind.args = {.dir = Right};
			else
			{
				bind.args = {.str = strdup(str.c_str())};
			}
		}
		else
		{
			bind.args = {NULL};
		}
		binds[i] = bind;
	}
}

Config::~Config()
{
	free();
}
void Config::free()
{
	delete[] startupBash;
	delete[] workspaceNames;
	for(int i = 0; i < screenPreferencesc; i++)
	{
		delete[] screenPreferences[i];
	}
	delete[] screenPreferences;
}
