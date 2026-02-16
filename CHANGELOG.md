# Linux build fixes and gitignore for GCC/CMake

Added missing `SFML/Graphics/Image.hpp` include in SFMLWindow.cpp so the `set_icon` method compiles on Linux where GCC doesn't transitively pull in the header.

Fixed GCC format-truncation warnings in Screen_OnGame.cpp by using the actual destination buffer size instead of NpcNameLen for snprintf calls. Fixed GCC stringop-overflow warning in ASIOSocket.cpp by adding an explicit uint16_t upper bound check on the send size. Added GCC/CMake/Linux patterns to .gitignore.
