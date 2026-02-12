#include "HotkeyManager.h"
#include "IInput.h"
#include <utility>

HotkeyManager& HotkeyManager::Get()
{
	static HotkeyManager s_instance;
	return s_instance;
}

void HotkeyManager::Clear()
{
	m_entries.clear();
}

void HotkeyManager::Register(const KeyCombo& combo, Trigger trigger, std::function<void()> callback)
{
	m_entries.push_back(Entry{ combo, trigger, std::move(callback) });
}

bool HotkeyManager::HandleKeyDown(KeyCode vk)
{
	return HandleKey(vk, Trigger::KeyDown);
}

bool HotkeyManager::HandleKeyUp(KeyCode vk)
{
	return HandleKey(vk, Trigger::KeyUp);
}

bool HotkeyManager::HandleKey(KeyCode vk, Trigger trigger)
{
	if (m_entries.empty()) {
		return false;
	}

	const bool ctrlDown = hb::shared::input::IsCtrlDown();
	const bool shiftDown = hb::shared::input::IsShiftDown();
	const bool altDown = hb::shared::input::IsAltDown();

	bool handled = false;
	for (const auto& entry : m_entries)
	{
		if (entry.trigger != trigger || entry.combo.vk != vk) {
			continue;
		}
		if (entry.combo.ctrl && !ctrlDown) {
			continue;
		}
		if (!entry.combo.ctrl && ctrlDown) {
			continue;
		}
		if (entry.combo.shift && !shiftDown) {
			continue;
		}
		if (!entry.combo.shift && shiftDown) {
			continue;
		}
		if (entry.combo.alt && !altDown) {
			continue;
		}
		if (!entry.combo.alt && altDown) {
			continue;
		}
		if (entry.callback) {
			entry.callback();
			handled = true;
		}
	}

	return handled;
}
