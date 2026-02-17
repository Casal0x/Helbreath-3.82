# Armor durability underflow fix

Fixed uint16_t underflow in armor durability reduction that could wrap remaining durability to ~65521 instead of breaking the item, making armor effectively indestructible.
