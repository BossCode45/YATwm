#include "commands.h"
#include "error.h"

#include <cctype>
#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <regex>
#include <cstring>

using std::cout, std::endl, std::string, std::vector, std::tolower;

CommandsModule::CommandsModule()
{
}
CommandsModule::~CommandsModule()
{
	for(Command c : commandList)
	{
		if(c.argc > 0)
			delete[] c.argTypes;
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
void CommandsModule::addCommand(std::string name, const void (*func)(const CommandArg *), const int argc, CommandArgType *argTypes)
{
	Command c = {name, nullptr, func, argc, argTypes, nullptr};
	addCommand(c);
}
void CommandsModule::addCommand(std::string name, const void(*func)(const CommandArg*), const int argc, std::vector<CommandArgType> argTypes)
{
	CommandArgType* argTypesArr = new CommandArgType[argc];
	for(int i = 0; i < argc; i++)
	{
		argTypesArr[i] = argTypes[i];
	}
	addCommand(name, func, argc, argTypesArr);
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
	string arg = "";
	bool inQuotes = false;
	bool escapeNext = true;
	char quoteType;
	for(int i = 0; i < command.size(); i++)
	{
		if(escapeNext)
		{
			arg += command[i];
			escapeNext = false;
		}
		else if(command[i] == '\\')
		{
			escapeNext = true;
		}
		else if(inQuotes)
		{
			if(command[i] == quoteType)
			{
				if(arg != "")
				{
					v.push_back(arg);
					arg = "";
				}
				inQuotes = false;
			}
			else
			{
				arg += command[i];
			}
		}
		else
		{
			if(command[i] == ' ')
			{
				if(arg != "")
				{
					v.push_back(arg);
					arg = "";
				}
			}
			else if(command[i] == '"' || command[i] == '\'')
			{
				inQuotes = true;
				quoteType = command[i];
			}
			else
			{
				arg += command[i];
			}
		}
	}
	if(arg != "")
		v.push_back(arg);
	return v;
}

string lowercase(string s)
{
    string s2 = s;
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
			case NUM:
				{
					try
					{
						args[i-1].num = std::stoi(split[i]);
						break;
					}
					catch(std::invalid_argument e)
					{
						delete[] args;
						throw Err(CMD_ERR_WRONG_ARGS, split[i] + " is not a number!");
					}
				}
			case MOVDIR:
				{
					if(lowercase(split[i]) == "up")
						args[i-1].dir = UP;
					else if(lowercase(split[i]) == "down")
						args[i-1].dir = DOWN;
					else if(lowercase(split[i]) == "left")
						args[i-1].dir = LEFT;
					else if(lowercase(split[i]) == "right")
						args[i-1].dir = RIGHT;
					else
					{
						delete[] args;
						throw Err(CMD_ERR_WRONG_ARGS, split[i] + " is not a direction!");
					}
					break;
				}
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
						try
						{
							rest[j] = std::stoi(split[j + i]);
						}
						catch(std::invalid_argument e)
						{
							delete[] rest;
							delete[] args;
							throw Err(CMD_ERR_WRONG_ARGS, split[i] + " is not a number!");
						}
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
	CommandArg* args;
	try
	{
		args = getCommandArgs(split, cmd->argTypes, cmd->argc);
	}
	catch(Err e)
	{
		throw e;
	}
	try
	{
		if(cmd->module == nullptr)
			cmd->staticFunc(args);
		else
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
	CommandArg* args;
	try
	{
		args = getCommandArgs(split, cmd->argTypes, cmd->argc);
	}
	catch(Err e)
	{
		return e;
	}
	for(int i = 0; i < cmd->argc; i++)
	{
		if(cmd->argTypes[i] == STR_REST || cmd->argTypes[i] == NUM_ARR_REST)
			delete[] args[i].str;
	}
	delete[] args;
	return Err(NOERR, "");
}
