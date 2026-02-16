# Screen shake restoration, reduced motion toggle, and toggle-to-chat setting

Fixed screen shake for Earth Shock Wave, Bloody Shock Strike, and Blizzard Projectile spells. These effects used tile coordinates but calculated distance with the pixel-coordinate formula, producing wildly inflated distances that prevented the shake from triggering.

Fixed screen shake being completely invisible because the camera snap_to call in the render pass overwrote the shake offset every frame before drawing. Shake is now applied after snap_to so the offset actually affects rendering.

Added a reduced motion toggle in the system tab that disables all camera shake effects, persisted in settings.json.

Removed unused server address settings from ConfigManager.

Added a "Toggle to Chat" setting in the system tab. When disabled, any printable keypress on the game screen automatically activates chat input without needing to press Enter first. Defaults to on (current behavior preserved). Persisted in settings.json.
