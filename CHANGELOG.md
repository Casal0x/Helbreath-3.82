# Stamina bar sync and HUD display fixes

Fixed stamina bar desync caused by running — server now sends authoritative stamina updates during movement, preventing client/server drift. Fixed HUD gauge bars silently clamping HP/MP/SP game state during rendering, which could overwrite correct server values. Removed short truncation from stat number display to prevent negative numbers at high values.
