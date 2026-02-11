#pragma once

#include <cstdint>
#include <array>
#include <memory>
#include "GameConstants.h"
#include "ChatMsg.h"

class ChatManager
{
public:
	static ChatManager& Get();

	// Lifecycle
	void Initialize();
	void Shutdown();

	// Message buffer management
	void AddMessage(char* msg, char type);
	void ClearMessages();
	CMsg* GetMessage(int index) const;

	// Whisper target management
	void AddWhisperTarget(const char* name);
	void ClearWhispers();
	const char* GetWhisperTargetName(int index) const;
	bool HasWhisperTarget(int index) const;
	int GetWhisperIndex() const { return m_whisper_index; }
	void SetWhisperIndex(int index) { m_whisper_index = static_cast<char>(index); }
	void CycleWhisperUp();
	void CycleWhisperDown();

	// Toggle accessors
	bool IsWhisperEnabled() const { return m_whisper_enabled; }
	void SetWhisperEnabled(bool enabled) { m_whisper_enabled = enabled; }
	bool IsShoutEnabled() const { return m_shout_enabled; }
	void SetShoutEnabled(bool enabled) { m_shout_enabled = enabled; }

private:
	ChatManager() = default;
	~ChatManager() = default;
	ChatManager(const ChatManager&) = delete;
	ChatManager& operator=(const ChatManager&) = delete;

	// Message scroll list (index 0 = most recent)
	std::array<std::unique_ptr<CMsg>, game_limits::max_chat_scroll_msgs> m_messages;

	// Whisper target names (index 0 = most recent)
	std::array<std::unique_ptr<CMsg>, game_limits::max_whisper_msgs> m_whisper_targets;
	char m_whisper_index = 0;

	// Toggle state
	bool m_whisper_enabled = true;
	bool m_shout_enabled = true;
};
