#pragma once

#include <unordered_map>
#include <string>

#include "commands.h"

class KeybindsModule {
	public:
		KeybindsModule(CommandsModule& commandsModule);
		~KeybindsModule() = default;
		const void bind(const CommandArg* argv);
		const void readBinds(const CommandArg* argv);
		const void exit(const CommandArg* argv);
	private:
		std::unordered_map<std::string, std::string> binds;
		bool exitNow;
		CommandsModule& commandsModule;
};
