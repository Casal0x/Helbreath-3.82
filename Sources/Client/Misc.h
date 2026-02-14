// Misc.h: Header-only implementation for CMisc utility functions.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include "GlobalDef.h"

enum {CODE_ENG,CODE_HAN1,CODE_HAN2};

namespace CMisc
{
	// Movement direction calculation using asymmetric zones (N/S 3:1, E/W 4:1 ratio)
	// Returns direction 1-8 (N, NE, E, SE, S, SW, W, NW) or 0 if same position
	static inline char get_next_move_dir(short sX, short sY, short dX, short dY)
	{
		short diffX = dX - sX;
		short diffY = dY - sY;

		if (diffX == 0 && diffY == 0) return 0;

		short absX = (diffX < 0) ? -diffX : diffX;
		short absY = (diffY < 0) ? -diffY : diffY;

		if (absY == 0) {
			return (diffX > 0) ? 3 : 7;  // East or West
		}
		if (absX == 0) {
			return (diffY < 0) ? 1 : 5;  // North or South
		}

		// Asymmetric ratios: N/S uses 3:1, E/W uses 4:1
		if (absY >= absX * 3) {
			return (diffY < 0) ? 1 : 5;  // North or South
		}
		if (absX >= absY * 4) {
			return (diffX > 0) ? 3 : 7;  // East or West
		}

		// Diagonal
		if (diffX > 0 && diffY < 0) return 2;  // NE
		if (diffX > 0 && diffY > 0) return 4;  // SE
		if (diffX < 0 && diffY > 0) return 6;  // SW
		return 8;  // NW
	}

	static inline void get_point(int x0, int y0, int x1, int y1, int * pX, int * pY, int * error_acc, int count)
	{
		int dx, dy, x_inc, y_inc, error, index;
		int result_x, result_y, cnt = 0;

		if ((x0 == x1) && (y0 == y1))
		{	*pX = x0;
			*pY = y0;
			return;
		}
		error = *error_acc;
		result_x = x0;
		result_y = y0;
		dx = x1-x0;
		dy = y1-y0;
		if(dx>=0) x_inc = 1;
		else
		{	x_inc = -1;
			dx = -dx;
		}
		if(dy>=0) y_inc = 1;
		else
		{	y_inc = -1;
			dy = -dy;
		}
		if(dx>dy)
		{	for(index = 0; index <= dx; index++)
			{	error += dy;
				if(error > dx)
				{	error -= dx;
					result_y += y_inc;
				}
				result_x += x_inc;
				cnt++;
				if (cnt >= count) break;
			}
		}else
		{	for(index = 0; index <= dy; index++)
			{	error += dx;
				if(error > dy)
				{	error -= dy;
					result_x += x_inc;
				}
				result_y += y_inc;
				cnt++;
				if (cnt >= count) break;
		}	}
		*pX = result_x;
		*pY = result_y;
		*error_acc = error;
	}

	static inline bool check_valid_string(const char * str)
	{
		size_t len = strlen(str);
		for (size_t i = 0; i < len; i++)
		{	if(str[i] == ' ') return false;
		}
		return true;
	}

	static inline void replace_string(char * str, char cFrom, char to)
	{
		size_t len = strlen(str);
		for (size_t i = 0; i < len; i++)
		{	if (str[i] == cFrom) str[i] = to;
		}
	}

	static inline char calc_direction(short sX, short sY, short dX, short dY)
	{
		double tmp1, tmp2, tmp3;
		if ((sX == dX) && (sY == dY)) return 1;
		if ((sX == dX) && (sY != dY))
		{	if (sY > dY) return 1;
			else return 5;
		}
		if ((sX != dX) && (sY == dY))
		{	if (sX > dX) return 7;
			else return 3;
		}
		tmp1 = static_cast<double>(dX - sX);
		tmp2 = static_cast<double>(dY - sY);
		tmp3 = tmp1 / tmp2;
		if (tmp3 < -3)
		{	if (sX > dX) return 7;
			else return 3;
		}
		if (tmp3 > 3)
		{	if (sX > dX) return 7;
			else return 3;
		}
		if ((tmp3 > -0.3333f) && (tmp3 <= 0.3333f))
		{	if (sY > dY) return 1;
			else return 5;
		}
		if ((tmp3 > 0.3333f) && (tmp3 <= 3.0f))
		{	if (sX > dX) return 8;
			else return 4;
		}
		if ((tmp3 >= -0.3333f) && (tmp3 < 3.0f))
		{	if (sX > dX) return 7;
			else return 3;
		}
		if ((tmp3 >= -3.0f) && (tmp3 < -0.3333f))
		{	if (sX > dX) return 6;
			else return 2;
		}
		return 1;
	}

	static inline bool check_valid_name(const char *str)
	{
		size_t i, len;
		len = strlen(str);
		for (i = 0; i < len; i++)
		{	if ( static_cast<unsigned char>(str[i]) >= 128 )	return false;
			if ( (str[i] == ',')  || (str[i] == '=') || (str[i] == ' ')  || (str[i] == '\n') ||
				 (str[i] == '\t') || (str[i] == '.') || (str[i] == '\\') || (str[i] == '/')  ||
				 (str[i] == ':')  || (str[i] == '*') || (str[i] == '?')  || (str[i] == '<')  ||
				 (str[i] == '>')  || (str[i] == '|') || (str[i] == '"')  || (str[i] == '`')  ||
				 (str[i] == ';')  || (str[i] == '=') || (str[i] == '@')  || (str[i] == '[')  ||
				 (str[i] == ']')  || (str[i] == '^') || (str[i] == '_')  || (str[i] == '\'') ) return false;
			if( (str[i] < '0') || (str[i] > 'z')) return false;
		}
		return true;
	}

	static inline bool is_valid_email(const char *str)
	{
		size_t len = strlen( str );
		if( len < 7 ) return false;
		bool hasAt = false;
		bool hasDot = false;
		for( size_t i = 0; i < len; i++ )
		{
			if( str[i] == '@' ) hasAt = true;
			if( str[i] == '.' ) hasDot = true;
		}
		return hasAt && hasDot;
	}
}
