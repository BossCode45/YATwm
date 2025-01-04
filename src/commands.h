#pragma once

#include "error.h"

#include <vector>
#include <string>
#include <any>
#include <functional>

enum MoveDir
	{
		UP,
		RIGHT,
		DOWN,
		LEFT
	};
enum CommandArgType
	{
		STR,
		NUM,
		MOVDIR,
		STR_REST,
		NUM_ARR_REST
	};

struct NumArr
{
	int* arr;
	int size;
};
typedef union
{
	char* str;
	int num;
	NumArr numArr;
	MoveDir dir;
} CommandArg;

struct Command
{
	const std::string name;
	const std::function<void(std::any&, const CommandArg* argv)> func;
	const std::function<void(const CommandArg* argv)> staticFunc;
	const int argc;
	CommandArgType* argTypes;
	std::any* module;
};
class CommandsModule
{
private:
	std::vector<Command> commandList;
	std::vector<std::string> splitCommand(std::string command);
	CommandArg* getCommandArgs(std::vector<std::string>& args, const CommandArgType* argTypes, const int argc);
	const void echo(const CommandArg* argv);
	const void loadFile(const CommandArg* argv);
	std::string cwd;
public:   
	CommandsModule();
	~CommandsModule();
	template <class T>
	void addCommand(std::string name, const void(T::*func)(const CommandArg*), const int argc, CommandArgType* argTypes, T* module);
	void addCommand(std::string name, const void(*func)(const CommandArg*), const int argc, CommandArgType* argTypes);
	template <class T>
	void addCommand(std::string name, const void(T::*func)(const CommandArg*), const int argc, std::vector<CommandArgType> argTypes, T* module);
	void addCommand(std::string name, const void(*func)(const CommandArg*), const int argc, std::vector<CommandArgType> argTypes);
	void addCommand(Command c);
	Command* lookupCommand(std::string name);
	void runCommand(std::string command);
	void runCommand(std::vector<std::string> split);
	std::vector<Err> checkCommand(std::string command);
	Err checkCommand(std::vector<std::string> split);
};

// YES I KNOW THIS IS BAD
// but it needs to be done this way
template <class T>
void CommandsModule::addCommand(std::string name, const void(T::*func)(const CommandArg*), const int argc, CommandArgType* argTypes, T* module)
{
	Command c = {name, (const void*(std::any::*)(const CommandArg* argv)) func, nullptr, argc, argTypes, (std::any*)module};
	addCommand(c);
}
template <class T>
void CommandsModule::addCommand(std::string name, const void(T::*func)(const CommandArg*), const int argc, std::vector<CommandArgType> argTypes, T* module)
{
	CommandArgType* argTypesArr = new CommandArgType[argc];
	for(int i = 0; i < argc; i++)
	{
		argTypesArr[i] = argTypes[i];
	}
	addCommand(name, func, argc, argTypesArr, module);
}
