#pragma once

#include <string>
#include <vector>
#include <memory>

class CGame;

class GameChatCommand
{
public:
	virtual ~GameChatCommand() = default;
	virtual const char* GetName() const = 0;
	virtual bool Execute(CGame* pGame, int iClientH, const char* pArgs) = 0;
};

class GameChatCommandManager
{
public:
	static GameChatCommandManager& Get();

	void Initialize(CGame* pGame);
	void RegisterCommand(std::unique_ptr<GameChatCommand> command);

	// Process a chat message starting with '/'. Returns true if consumed.
	bool ProcessCommand(int iClientH, const char* pMessage, uint32_t dwMsgSize);

private:
	GameChatCommandManager() = default;
	~GameChatCommandManager() = default;
	GameChatCommandManager(const GameChatCommandManager&) = delete;
	GameChatCommandManager& operator=(const GameChatCommandManager&) = delete;

	void RegisterBuiltInCommands();
	void LogCommand(int iClientH, const char* pCommand);

	CGame* m_pGame = nullptr;
	std::vector<std::unique_ptr<GameChatCommand>> m_commands;
	bool m_bInitialized = false;
};
