#include "CmdHelp.h"
#include <cstdio>
#include <cstring>
#include "Log.h"
#include "StringCompat.h"

void CmdHelp::execute(CGame* game, const char* args)
{
	// help <command> — show detailed help for a specific command
	if (args != nullptr && args[0] != '\0')
	{
		for (const auto& cmd : m_commands)
		{
			if (hb_stricmp(args, cmd->get_name()) == 0)
			{
				hb::logger::log("{} - {}", cmd->get_name(), cmd->GetDescription());

				// Print help text line by line (split on \n)
				const char* help = cmd->GetHelp();
				const char* p = help;
				while (*p != '\0')
				{
					const char* lineEnd = p;
					while (*lineEnd != '\0' && *lineEnd != '\n')
						lineEnd++;

					char line[256];
					size_t len = lineEnd - p;
					if (len >= sizeof(line)) len = sizeof(line) - 1;
					std::memcpy(line, p, len);
					line[len] = '\0';
					hb::logger::log("{}", line);

					p = (*lineEnd == '\n') ? lineEnd + 1 : lineEnd;
				}
				return;
			}
		}

		hb::logger::log("Unknown command: '{}'. Type 'help' for a list.", args);
		return;
	}

	// help — list all commands
	hb::logger::log("Available commands:");
	for (const auto& cmd : m_commands)
	{
		hb::logger::log("{} {}", cmd->get_name(), cmd->GetDescription());
	}
	hb::logger::log("Type 'help <command>' for detailed usage.");
}
