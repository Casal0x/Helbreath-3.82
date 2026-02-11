#include "ChatManager.h"
#include <cstring>

ChatManager& ChatManager::Get()
{
	static ChatManager instance;
	return instance;
}

void ChatManager::Initialize()
{
	m_whisper_index = game_limits::max_whisper_msgs;
	m_whisper_enabled = true;
	m_shout_enabled = true;
	ClearMessages();
	ClearWhispers();
}

void ChatManager::Shutdown()
{
	ClearMessages();
	ClearWhispers();
}

void ChatManager::AddMessage(char* msg, char type)
{
	if (m_messages[game_limits::max_chat_scroll_msgs - 1] != nullptr)
	{
		m_messages[game_limits::max_chat_scroll_msgs - 1].reset();
	}
	for (int i = game_limits::max_chat_scroll_msgs - 2; i >= 0; i--)
	{
		m_messages[i + 1] = std::move(m_messages[i]);
	}
	m_messages[0] = std::make_unique<CMsg>(1, msg, type);
}

void ChatManager::ClearMessages()
{
	for (auto& msg : m_messages) msg.reset();
}

CMsg* ChatManager::GetMessage(int index) const
{
	if (index < 0 || index >= game_limits::max_chat_scroll_msgs) return nullptr;
	return m_messages[index].get();
}

void ChatManager::AddWhisperTarget(const char* name)
{
	if (m_whisper_targets[game_limits::max_whisper_msgs - 1] != nullptr)
	{
		m_whisper_targets[game_limits::max_whisper_msgs - 1].reset();
	}
	for (int i = game_limits::max_whisper_msgs - 2; i >= 0; i--)
	{
		m_whisper_targets[i + 1] = std::move(m_whisper_targets[i]);
		m_whisper_targets[i].reset();
	}
	m_whisper_targets[0] = std::make_unique<CMsg>(0, name, 0);
	m_whisper_index = 0;
}

void ChatManager::ClearWhispers()
{
	for (auto& msg : m_whisper_targets) msg.reset();
}

const char* ChatManager::GetWhisperTargetName(int index) const
{
	if (index < 0 || index >= game_limits::max_whisper_msgs) return nullptr;
	if (!m_whisper_targets[index]) return nullptr;
	return m_whisper_targets[index]->m_pMsg;
}

bool ChatManager::HasWhisperTarget(int index) const
{
	if (index < 0 || index >= game_limits::max_whisper_msgs) return false;
	return m_whisper_targets[index] != nullptr;
}

void ChatManager::CycleWhisperUp()
{
	int max_index = 0;
	for (int i = game_limits::max_whisper_msgs - 1; i >= 0; i--)
	{
		if (m_whisper_targets[i] != nullptr)
		{
			max_index = i;
			break;
		}
	}
	m_whisper_index++;
	if (m_whisper_index > max_index) m_whisper_index = 0;
	if (m_whisper_index < 0) m_whisper_index = max_index;
}

void ChatManager::CycleWhisperDown()
{
	int max_index = 0;
	for (int i = game_limits::max_whisper_msgs - 1; i >= 0; i--)
	{
		if (m_whisper_targets[i] != nullptr)
		{
			max_index = i;
			break;
		}
	}
	m_whisper_index--;
	if (m_whisper_index < 0) m_whisper_index = max_index;
	if (m_whisper_index > max_index) m_whisper_index = 0;
}
