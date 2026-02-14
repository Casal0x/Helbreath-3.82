# Changelog

### Bug Fixes
- Server broadcast messages (type 10) were not appearing in the client's global chat area — the event list gate only allowed whispers (type 20) to bypass the floating text slot check, now broadcasts get the same treatment
- ShowChat server command terminal was immediately closing — removed cmd.exe wrapper and launch PowerShell directly to avoid pipe interpretation issues
- Exchange dialog did not release grabbed equipment items on close — the `item_id` field was being overwritten by the server response with the item config ID, so the cleanup read the wrong index. Added a dedicated `inv_slot` field to track the actual inventory slot for disable/enable
- Stackable items (potions, arrows) in exchange were not disabled after confirming quantity — could be added to multiple exchange slots simultaneously

### Code Cleanup
- Changed three diagnostic chat log messages from `log` to `debug` level so they no longer pollute the chat log with packet validation noise
- ShowChat terminal now filters out `[DEBUG]` lines so only actual chat content is displayed
