#include <iostream>

#include "keybinds.h"

using std::string, std::cin, std::cout, std::endl;

KeybindsModule::KeybindsModule(CommandsModule& commandsModule)
	:commandsModule(commandsModule)
{
	CommandArgType* bindArgs = new CommandArgType[2];
	bindArgs[0] = STR;
	bindArgs[1] = STR_REST;
	commandsModule.addCommand("bind", &KeybindsModule::bind, 2, bindArgs, this);
	commandsModule.addCommand("exit", &KeybindsModule::exit, 0, {}, this);
	commandsModule.addCommand("readBinds", &KeybindsModule::readBinds, 0, {}, this);
	exitNow = false;
}

const void KeybindsModule::bind(const CommandArg* argv)
{
	Err e = commandsModule.checkCommand(argv[1].str);
	if(e.code != NOERR)
	{
		e.message = "Binding fail - " + e.message;
		throw e;
	}
	binds.insert({argv[0].str, argv[1].str});
}
const void KeybindsModule::readBinds(const CommandArg* argv)
{
	cout << "Reading binds" << endl;
	while(exitNow == false)
	{
		string key;
		cout << "> ";
		std::getline(std::cin, key);
		commandsModule.runCommand(binds.find(key)->second);
	}
}

const void KeybindsModule::exit(const CommandArg* argv)
{
	exitNow = true;
	cout << "Exiting..." << endl;
}
