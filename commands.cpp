#include "commands.h"
#include "error.h"

#include <cctype>
#include <iostream>
#include <algorithm>
#include <string>
#include <utility>
#include <vector>
#include <regex>
#include <cstring>

using std::cout, std::endl, std::string, std::vector, std::tolower;

const void CommandsModule::printHello(const CommandArg* argv)
{
	cout << "Hello" << endl;
}

const void CommandsModule::echo(const CommandArg* argv)
{
	cout << "Echo: '" << argv[0].str << '\'' << endl;
}

CommandsModule::CommandsModule()
{
	addCommand("printHello", &CommandsModule::printHello, 0, {}, this);
	CommandArgType* args0 = new CommandArgType[1];
	args0[0] = STR_REST;
	addCommand("echo", &CommandsModule::echo, 1, args0, this);
}
CommandsModule::~CommandsModule()
{
	for(Command c : commandList)
	{
		// This is probably needed but its not working
		//if(c.argc > 0)
			//delete[] c.argTypes;
	}
}

void CommandsModule::addCommand(Command c)
{
	if(lookupCommand(c.name) != nullptr)
	{
		cout << "Duplicate command: " << c.name << endl;
	}
	commandList.push_back(c);
}

struct NameMatches
{
	NameMatches(string s): s_{s} {}
	bool operator()(Command c) { return (c.name == s_); }
	string s_;        
};

Command* CommandsModule::lookupCommand(string name)
{
	auto elem = std::find_if(commandList.begin(), commandList.end(), NameMatches(name));
	if (elem != commandList.end())
	{
		int i = elem - commandList.begin();
		return &(commandList[i]);
	}
	else
	{
		return nullptr;
	}
}

vector<string> CommandsModule::splitCommand(string command)
{
	vector<string> v;
	string regexString = "((?:\"[^\"\n]+\")|(?:'[^'\n]+')|(?:[^ \n]+))+";
	std::regex regex(regexString, std::regex_constants::_S_icase);
	auto begin = std::sregex_iterator(command.begin(), command.end(), regex);
	auto end = std::sregex_iterator();
	for (std::sregex_iterator i = begin; i != end; i++)
	{
		std::smatch match = *i;
		std::string str = match.str();
		if(str[0] == '\'' || str[0] == '"')
			str = str.substr(1, str.size() - 2);
		v.push_back(str);
	}
	return v;
}

template <typename T>
std::basic_string<T> lowercase(const std::basic_string<T>& s)
{
    std::basic_string<T> s2 = s;
    std::transform(s2.begin(), s2.end(), s2.begin(), [](unsigned char c){ return std::tolower(c); });
    return s2;
}

CommandArg* CommandsModule::getCommandArgs(vector<string>& split, const CommandArgType* argTypes, const int argc)
{
	CommandArg* args = new CommandArg[argc];
	for(int i = 1; i < argc + 1; i++)
	{
		switch(argTypes[i-1])
		{
			case STR: args[i-1].str = (char*)split[i].c_str(); break;
			case NUM: args[i-1].num = std::stoi(split[i]); break;
			case DIR: args[i-1].dir = LEFT; break;
			case STR_REST:
			{
				string rest = "";
				for(int j = i; j < split.size(); j++)
				{
					rest += split[j];
					if(j != split.size() - 1)
						rest += " ";
				}
				args[i-1].str = new char[rest.size()];
				strcpy(args[i-1].str, rest.c_str());
				return args;
			}
			case NUM_ARR_REST:
			{
				int* rest = new int[split.size() - i];
				for(int j = 0; j < split.size() - i; j++)
				{
					rest[j] = std::stoi(split[j + i]);
				}
				args[i-1].numArr = {rest, (int) split.size() - i};
				return args;
			}
			default: cout << "UH OH SOMETHING IS VERY WRONG" << endl;
		}
	}
	return args;
}

void CommandsModule::runCommand(string command)
{
	vector<string> split = splitCommand(command);
	Command* cmd = lookupCommand(split[0]);
	if(cmd == nullptr)
		throw Err(CMD_ERR_NOT_FOUND, split[0] + " is not a valid command name");
	if(cmd->argc > split.size() - 1)
		throw Err(CMD_ERR_WRONG_ARGS, command + " is the wrong args");
	CommandArg* args = getCommandArgs(split, cmd->argTypes, cmd->argc);
	try
	{
		cmd->func(*cmd->module, args);
	}
	catch (Err e)
	{
		for(int i = 0; i < cmd->argc; i++)
		{
			if(cmd->argTypes[i] == STR_REST)
				delete[] args[i].str;
		}
		delete[] args;
		throw e;
	}
	for(int i = 0; i < cmd->argc; i++)
	{
		if(cmd->argTypes[i] == STR_REST)
			delete[] args[i].str;
	}
	delete[] args;
}

Err CommandsModule::checkCommand(string command)
{
	vector<string> split = splitCommand(command);
	Command* cmd = lookupCommand(split[0]);
	if(cmd == nullptr)
		return Err(CMD_ERR_NOT_FOUND, split[0] + " is not a valid command name");
	if(cmd->argc > split.size())
		return Err(CMD_ERR_WRONG_ARGS, command + " is the wrong args");
	CommandArg* args = getCommandArgs(split, cmd->argTypes, cmd->argc);
	for(int i = 0; i < cmd->argc; i++)
	{
		if(cmd->argTypes[i] == STR_REST || cmd->argTypes[i] == NUM_ARR_REST)
			delete[] args[i].str;
	}
	delete[] args;
	return Err(NOERR, "");
}
