# Teleport Pre-Auth, Fade Transitions, and Map Cache Fix

Fixed map cache hash validation to prevent stale cache hits.

Added teleport pre-authorization: client sends RequestTeleportAuth before committing to a teleport. Server validates player state, teleport tile existence, destination map availability, and level limits (both min and max), responding with TeleportApproved or a rejection notice. Rejection notifications (NoticeMsg, LimitedLevel, TravelerLimitedLevel, NoRecall) gracefully cancel the pending auth on the client side.

On approval, the client fades to black (150ms), sends the real RequestTeleport at full black, loads map data invisibly, then hands off to GameModeManager's fade-in. GameModeManager's redundant FadeOut phase is skipped during teleport transitions (config set to 0s out / 0.15s in), then restored to defaults afterward. This eliminates both the visible map "pop" and the unnecessary delay at peak black.

Fixed rejected teleport causing player to get stuck on the tile. After auth rejection, the rejected tile coordinates are remembered so the client won't re-request auth while the player remains on the same tile. Coords clear on successful auth or map change, allowing normal movement off the tile.
