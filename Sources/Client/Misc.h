// Misc.h: Header-only implementation for CMisc utility functions.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "GlobalDef.h"

enum {CODE_ENG,CODE_HAN1,CODE_HAN2};

namespace CMisc
{
	static inline char cGetNextMoveDir(short sX, short sY, short dX, short dY)
	{
		short absX, absY;
		char  cRet = 0;

		absX = sX - dX;
		absY = sY - dY;

		if ((absX == 0) && (absY == 0)) cRet = 0;

		if (absX == 0) {
			if (absY > 0) cRet = 1;
			if (absY < 0) cRet = 5;
		}
		if (absY == 0) {
			if (absX > 0) cRet = 7;
			if (absX < 0) cRet = 3;
		}
		if ( (absX > 0)	&& (absY > 0) ) cRet = 8;
		if ( (absX < 0)	&& (absY > 0) ) cRet = 2;
		if ( (absX > 0)	&& (absY < 0) ) cRet = 6;
		if ( (absX < 0)	&& (absY < 0) ) cRet = 4;

		return cRet;
	}

	static inline void GetPoint(int x0, int y0, int x1, int y1, int * pX, int * pY, int * pError, int iCount)
	{
		int dx, dy, x_inc, y_inc, error, index;
		int iResultX, iResultY, iCnt = 0;

		if ((x0 == x1) && (y0 == y1))
		{	*pX = x0;
			*pY = y0;
			return;
		}
		error = *pError;
		iResultX = x0;
		iResultY = y0;
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
					iResultY += y_inc;
				}
				iResultX += x_inc;
				iCnt++;
				if (iCnt >= iCount) break;
			}
		}else
		{	for(index = 0; index <= dy; index++)
			{	error += dx;
				if(error > dy)
				{	error -= dy;
					iResultX += x_inc;
				}
				iResultY += y_inc;
				iCnt++;
				if (iCnt >= iCount) break;
		}	}
		*pX = iResultX;
		*pY = iResultY;
		*pError = error;
	}

	static inline bool bCheckValidString(char * str)
	{
		size_t len = strlen(str);
		for (size_t i = 0; i < len; i++)
		{	if(str[i] == ' ') return false;
		}
		return true;
	}

	static inline void ReplaceString(char * pStr, char cFrom, char cTo)
	{
		size_t len = strlen(pStr);
		for (size_t i = 0; i < len; i++)
		{	if (pStr[i] == cFrom) pStr[i] = cTo;
		}
	}

	static inline char cCalcDirection(short sX, short sY, short dX, short dY)
	{
		double dTmp1, dTmp2, dTmp3;
		if ((sX == dX) && (sY == dY)) return 1;
		if ((sX == dX) && (sY != dY))
		{	if (sY > dY) return 1;
			else return 5;
		}
		if ((sX != dX) && (sY == dY))
		{	if (sX > dX) return 7;
			else return 3;
		}
		dTmp1 = (double)(dX - sX);
		dTmp2 = (double)(dY - sY);
		dTmp3 = dTmp1 / dTmp2;
		if (dTmp3 < -3)
		{	if (sX > dX) return 7;
			else return 3;
		}
		if (dTmp3 > 3)
		{	if (sX > dX) return 7;
			else return 3;
		}
		if ((dTmp3 > -0.3333f) && (dTmp3 <= 0.3333f))
		{	if (sY > dY) return 1;
			else return 5;
		}
		if ((dTmp3 > 0.3333f) && (dTmp3 <= 3.0f))
		{	if (sX > dX) return 8;
			else return 4;
		}
		if ((dTmp3 >= -0.3333f) && (dTmp3 < 3.0f))
		{	if (sX > dX) return 7;
			else return 3;
		}
		if ((dTmp3 >= -3.0f) && (dTmp3 < -0.3333f))
		{	if (sX > dX) return 6;
			else return 2;
		}
		return 1;
	}

	static inline bool bCheckValidName(char *pStr)
	{
		size_t i, iLen;
		iLen = strlen(pStr);
		for (i = 0; i < iLen; i++)
		{	if ( pStr[i] < 0 )	return false;
			if ( (pStr[i] == ',')  || (pStr[i] == '=') || (pStr[i] == ' ')  || (pStr[i] == '\n') ||
				 (pStr[i] == '\t') || (pStr[i] == '.') || (pStr[i] == '\\') || (pStr[i] == '/')  ||
				 (pStr[i] == ':')  || (pStr[i] == '*') || (pStr[i] == '?')  || (pStr[i] == '<')  ||
				 (pStr[i] == '>')  || (pStr[i] == '|') || (pStr[i] == '"')  || (pStr[i] == '`')  ||
				 (pStr[i] == ';')  || (pStr[i] == '=') || (pStr[i] == '@')  || (pStr[i] == '[')  ||
				 (pStr[i] == ']')  || (pStr[i] == '^') || (pStr[i] == '_')  || (pStr[i] == '\'') ) return false;
			if( (pStr[i] < '0') || (pStr[i] > 'z')) return false;
		}
		return true;
	}

	static inline bool bIsValidEmail(char *pStr)
	{
		size_t len = strlen( pStr );
		if( len < 7 ) return false;
		char cEmail[52];
		ZeroMemory( cEmail, sizeof(cEmail) );
		memcpy( cEmail, pStr, len );
		bool bFlag = false;
		for( size_t i=0 ; i<len ; i++ )
		{
			if( cEmail[i] == '@' ) bFlag = true;
		}
		if( bFlag == false ) return false;
		bFlag = false;
		for( int i=0 ; i<len ; i++ )
		{
			if( cEmail[i] == '.' ) bFlag = true;
		}
		if( bFlag == false ) return false;
		return true;
	}
}
