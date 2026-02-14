#pragma once

// 8-directional movement offset helpers.
// Directions: 1=N, 2=NE, 3=E, 4=SE, 5=S, 6=SW, 7=W, 8=NW (0=none)

namespace hb::shared::direction {

	// Lookup tables indexed by direction (0 unused, 1-8 valid)
	constexpr int OffsetX[] = { 0,  0, 1, 1, 1, 0, -1, -1, -1 };
	constexpr int OffsetY[] = { 0, -1, -1, 0, 1, 1,  1,  0, -1 };

	// Apply directional offset to coordinates.
	inline void ApplyOffset(char dir, int& x, int& y)
	{
		if (dir >= 1 && dir <= 8)
		{
			x += OffsetX[dir];
			y += OffsetY[dir];
		}
	}

	inline void ApplyOffset(char dir, short& x, short& y)
	{
		if (dir >= 1 && dir <= 8)
		{
			x += static_cast<short>(OffsetX[dir]);
			y += static_cast<short>(OffsetY[dir]);
		}
	}

} // namespace hb::shared::direction
