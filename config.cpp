#include "config.h"

#include "error.h"
#include "toml++/toml.hpp"
#include "util.h"

#include <X11/X.h>
#include <X11/Xlib.h>

#include <string>
#include <map>

//Just for testing
#include <iostream>
#include <vector>

using std::map, std::string, std::to_string;

// For testing
using std::cout, std::endl, std::cerr;

map<string, void(*) (const KeyArg arg)> funcNameMap = {
	{"exit", exit},
	{"spawn", spawn},
	{"toggle", toggle},
	{"kill", kill},
	{"changeWS", changeWS},
	{"wToWS", wToWS},
	{"focChange", focChange},
	{"wMove", wMove},
	{"bashSpawn", bashSpawn},
	{"reload", reload},
	{"wsDump", wsDump},
	{"nextMonitor", nextMonitor},
};


Config::Config()
{
}

string to_string(string s)
{
	return s;
}
string to_string(bool b)
{
	if(b)
		return "true";
	else
		return "false";
}

template <typename T>
T Config::getValue(string path, Err* err)
{
	std::optional<T> tblVal = tbl.at_path(path).value<T>();
	if(tblVal)
		return *tblVal;
	else
	{
		err->code = ERR_CFG_NON_FATAL;
		T val = *defaults.at_path(path).value<T>();
		err->errorMessage += "\n\tValue for " + path + " is invalid, using default (" + to_string(val) + ")";
		return val;
	}
}

void Config::loadWorkspaceArrays(toml::table tbl, toml::table defaults, Err* err)
{
	if(!tbl["Workspaces"]["workspaceNames"].as_array())
	{
		err->code = ERR_CFG_NON_FATAL;
		err->errorMessage += "\n\tworkspaceNames invalid array, using defaults";
		return loadWorkspaceArrays(defaults, defaults, err);
	}
	workspaceNamesc = tbl["Workspaces"]["workspaceNames"].as_array()->size();
	workspaceNames = new string[workspaceNamesc];
	for(int i = 0; i < workspaceNamesc; i++)
	{
		auto element = tbl["Workspaces"]["workspaceNames"][i].value<string>();
		if(element)
			workspaceNames[i] = *element;
		else
		{
			err->code = ERR_CFG_NON_FATAL;
			err->errorMessage += "\nelement " + to_string(i) + " in workspaceNames invalid, using defaults";
			delete[] workspaceNames;
			return loadWorkspaceArrays(defaults, defaults, err);
		}
	}
	if(!tbl["Workspaces"]["screenPreferences"].as_array())
	{
		err->code = ERR_CFG_NON_FATAL;
		err->errorMessage += "\nscreenPreferences invalid array, using default";
		delete[] workspaceNames;
		return loadWorkspaceArrays(defaults, defaults, err);
	}
	screenPreferencesc = tbl["Workspaces"]["screenPreferences"].as_array()->size();
	if(screenPreferencesc != workspaceNamesc)
	{
		err->code = ERR_CFG_NON_FATAL;
		err->errorMessage += "\nworkspaceNames and screenPreferences aren't the same length, using defaults";
		delete[] workspaceNames;
		return loadWorkspaceArrays(defaults, defaults, err);
	}
	screenPreferences = new int*[screenPreferencesc];
	for(int i = 0; i < screenPreferencesc; i++)
	{
		if(!tbl["Workspaces"]["screenPreferences"][i].as_array())
		{
			err->code = ERR_CFG_NON_FATAL;
			err->errorMessage += "\telement " + to_string(i) + " in screenPreferences in invalid, using defaults";
			delete[] workspaceNames;
			for(int k = 0; k < i; k++)
			{
				delete[] screenPreferences[k];
			}
			delete[] screenPreferences;
			return loadWorkspaceArrays(defaults, defaults, err);
		}
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
				err->code = ERR_CFG_NON_FATAL;
				err->errorMessage += "\telement " + to_string(i) + " " + to_string(j) + " in screenPreferences in invalid, using defaults";
				delete[] workspaceNames;
				for(int k = 0; k <= i; k++)
				{
					delete[] screenPreferences[k];
				}
				delete[] screenPreferences;
				return loadWorkspaceArrays(defaults, defaults, err);
			}
		}
		screenPreferences[i] = wsScreens;
	}
}

void Config::loadStartupBash(toml::table tbl, toml::table defaults, Err* err)
{
	if(!tbl["Startup"]["startupBash"].as_array())
	{
		err->code = ERR_CFG_NON_FATAL;
		err->errorMessage += "\n\tstartupBash array invalid, using default";
		return loadStartupBash(defaults, defaults, err);
	}
	startupBashc = tbl["Startup"]["startupBash"].as_array()->size();
	std::vector<string> startupBash;
	for(int i = 0; i < startupBashc; i++)
	{
		auto element = tbl["Startup"]["startupBash"][i].value<string>();
		if(element)
			startupBash.push_back(*element);
		else
		{
			err->code = ERR_CFG_NON_FATAL;
			err->errorMessage += "\n\tstartupBash element " + to_string(i) + " invalid, skipping";
		}
	}
	startupBashc = startupBash.size();
	this->startupBash = new string[startupBashc];
	for(int i = 0; i < startupBash.size(); i++)
	{
		this->startupBash[i] = startupBash[i];
	}
}

Err Config::reload()
{
	if(!loaded)
		return {ERR_CFG_FATAL, "Path not set yet, call loadFromFile before reload"};
	return loadFromFile(path);
}

Err Config::loadFromFile(string path)
{
	if(loaded)
	{
		free();
	}
	loaded = true;
	this->path = path;
	Err err;
	err.code = NOERR;
	err.errorMessage = "";
	defaults = toml::parse_file("/etc/YATwm/config.toml");
	try
	{
		tbl = toml::parse_file(path);
	}
	catch (const toml::parse_error& parseErr)
	{
		err.code = ERR_CFG_FATAL;
		string description = (string) parseErr.description();
		string startCol = std::to_string(parseErr.source().begin.column);
		string startLine = std::to_string(parseErr.source().begin.line);
		string endCol = std::to_string(parseErr.source().end.column);
		string endLine = std::to_string(parseErr.source().end.line);
		string pos =  "Line " + startLine;
		string what = parseErr.what();
		err.errorMessage += "\n\t" + description + "\t(" + pos + ")" + "\n\tUsing /etc/YATwm/config.toml instead";
		tbl = defaults;
	}

	//Startup
	loadStartupBash(tbl, defaults, &err);

	//Main
	gaps = getValue<int>("Main.gaps", &err);
	outerGaps = getValue<int>("Main.outerGaps", &err);
	logFile = getValue<string>("Main.logFile", &err);

	//Workspaces
	numWS = getValue<int>("Workspaces.numWS", &err);
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
	maxMonitors = getValue<int>("Workspaces.maxMonitors", &err);
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
	bool swapSuperAlt = getValue<bool>("Keybinds.swapSuperAlt", &err);

	toml::node_view<toml::node> bindsArr = tbl["Keybinds"]["key"];
	if(!bindsArr.is_array())
	{
		err.code = ERR_CFG_NON_FATAL;
		err.errorMessage += "\n\tBinds array not valid, using default";
		bindsArr = defaults["Keybinds"]["key"];
	}
	std::vector<KeyBind> keyBinds;
	bindsc = bindsArr.as_array()->size();
	for(int i = 0; i < bindsc; i++)
	{
		KeyBind bind;
		bind.modifiers = 0;
		const std::optional<string> potentialBindString = bindsArr[i]["bind"].value<string>();
		string bindString;
		if(potentialBindString)
			bindString = *potentialBindString;
		else
		{
			err.code = ERR_CFG_NON_FATAL;
			err.errorMessage += "\n\tSkipping element " + to_string(i) + " of binds as the bind string is invalid";
			continue;
		}
		std::vector<string> keys = split(bindString, '+');
		for(string key : keys)
		{
			if(key == "mod")
			{
				if(!swapSuperAlt)
					bind.modifiers |= Mod4Mask;
				else
					bind.modifiers |= Mod1Mask;
			}
			else if(key == "alt")
			{
				if(swapSuperAlt)
					bind.modifiers |= Mod4Mask;
				else
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
				if(bind.keysym == NoSymbol)
				{
					err.code = ERR_CFG_NON_FATAL;
					err.errorMessage += "\n\tSkipping element " + to_string(i) + " of binds as the bind string is invalid";
					continue;
				}
			}
		}
		std::optional<string> potentialFuncString = bindsArr[i]["func"].value<string>();
		string funcString;
		if(potentialFuncString)
			funcString = *potentialFuncString;
		else
		{
			err.code = ERR_CFG_NON_FATAL;
			err.errorMessage += "\n\tSkipping element " + to_string(i) + " of binds as the func string is invalid";
			continue;
		}
		if(funcNameMap.count(funcString) == 0)
		{
			err.code = ERR_CFG_NON_FATAL;
			err.errorMessage += "\n\tSkipping element " + to_string(i) + " of binds as the func string is invalid";
			continue;
		}
		void(* func) (const KeyArg arg) = funcNameMap.find(funcString)->second;
		bind.func = func;

		auto args = bindsArr[i]["args"];
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
		keyBinds.push_back(bind);
	}
	bindsc = keyBinds.size();
	binds = new KeyBind[bindsc];
	for(int i = 0; i < bindsc; i++)
	{
		binds[i] = keyBinds[i];
	}
	if(err.code != NOERR)
		err.errorMessage = err.errorMessage.substr(1);
	return err;
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
	delete[] binds;
	loaded = false;
}
