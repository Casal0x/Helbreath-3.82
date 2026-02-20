## Fix: Overlay instant-dismiss on Enter key
- Added `discard_pending_input()` to `Overlay_LogResMsg`, `Overlay_VersionNotMatch`, and `Overlay_ChangePassword` — prevents Enter held from the previous screen from immediately firing the default OK button when the overlay appears
