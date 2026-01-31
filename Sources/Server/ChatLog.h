#pragma once

class ChatLog
{
public:
	static ChatLog& Get();
	void Initialize();
	void Write(int chatType, const char* playerName, const char* mapName, const char* message, const char* whisperTarget = nullptr);
	void Shutdown();
private:
	ChatLog() = default;
	bool m_bInitialized = false;
};
