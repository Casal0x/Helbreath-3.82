#pragma once

#include "IInput.h"  // For KeyCode
#include <functional>
#include <vector>

class HotkeyManager
{
public:
	static HotkeyManager& Get();

	enum class Trigger
	{
		KeyDown,
		KeyUp
	};

	struct KeyCombo
	{
		KeyCode vk;
		bool ctrl;
		bool shift;
		bool alt;
	};

	void Clear();
	void Register(const KeyCombo& combo, Trigger trigger, std::function<void()> callback);
	bool HandleKeyDown(KeyCode vk);
	bool HandleKeyUp(KeyCode vk);

private:
	HotkeyManager() = default;
	~HotkeyManager() = default;
	HotkeyManager(const HotkeyManager&) = delete;
	HotkeyManager& operator=(const HotkeyManager&) = delete;

	struct Entry
	{
		KeyCombo combo;
		Trigger trigger;
		std::function<void()> callback;
	};

	bool HandleKey(KeyCode vk, Trigger trigger);

	std::vector<Entry> m_entries;
};
