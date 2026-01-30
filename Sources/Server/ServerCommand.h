#pragma once

#include <string>
#include <vector>
#include <memory>

class CGame;

class ServerCommand
{
public:
	virtual ~ServerCommand() = default;
	virtual const char* GetName() const = 0;
	virtual const char* GetDescription() const = 0;
	virtual void Execute(CGame* pGame, const char* pArgs) = 0;
};

class ServerCommandManager
{
public:
	static ServerCommandManager& Get();

	void Initialize(CGame* pGame);
	void RegisterCommand(std::unique_ptr<ServerCommand> command);
	bool ProcessCommand(const char* pInput);

private:
	ServerCommandManager() = default;
	~ServerCommandManager() = default;
	ServerCommandManager(const ServerCommandManager&) = delete;
	ServerCommandManager& operator=(const ServerCommandManager&) = delete;

	void RegisterBuiltInCommands();

	CGame* m_pGame = nullptr;
	std::vector<std::unique_ptr<ServerCommand>> m_commands;
	bool m_bInitialized = false;
};
