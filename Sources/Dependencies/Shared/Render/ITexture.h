// ITexture.h: Abstract interface for texture/surface objects
//
// This interface wraps platform-specific texture implementations (SFML, etc.)
//////////////////////////////////////////////////////////////////////

#pragma once

#include <cstdint>

namespace hb::shared::render {

class ITexture
{
public:
	virtual ~ITexture() = default;

	virtual uint16_t get_width() const = 0;
	virtual uint16_t get_height() const = 0;
};

} // namespace hb::shared::render
