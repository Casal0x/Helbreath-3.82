# Hair invisibility fix, traveler nameplate color, and Alt+F4 logout fix

Fixed player hair not becoming transparent when the invisibility spell is active. Hair was the only equipment piece bypassing the transparency check, now renders at 25% opacity like all other equipment.

Fixed Traveler nameplate showing green (friendly) instead of blue (neutral). The local player path unconditionally forced green for non-PK players, ignoring citizen status. Travelers now always display the neutral blue color, matching NPC behavior.

Fixed Alt+F4 recasting the last spell instead of starting the logout sequence. F4 key-up now checks for Alt held, preventing the spell recast from firing and cancelling the logout countdown.
