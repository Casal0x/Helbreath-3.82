# Mana saving item bugfixes

Fixed Magic dialog not showing reduced mana costs when mana saving items are equipped — client was reading effect data from inventory items (which lack it) instead of item configs. Removed erroneous +20 flat mana penalty for weapon_type 34 wands (Berserk/Kloness/Resurrection) so all MS20 wands now provide the same mana saving.
