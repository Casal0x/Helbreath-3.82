// Tile.cpp: implementation of the CTile class.
//
//////////////////////////////////////////////////////////////////////

#include "CommonTypes.h"
#include "Tile.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CTile::CTile()
{ 
 

	m_bIsMoveAllowed = true;
	m_bIsTeleport    = false;
	m_bIsWater       = false;
	m_bIsFarm        = false;
												   
	m_sOwner      = 0;
	m_cOwnerClass = 0;
	
	m_sDeadOwner      = 0;
	m_cDeadOwnerClass = 0;
	
	for(int i = 0; i < hb::server::map::TilePerItems; i++) 
		m_pItem[i] = 0;
	m_cTotalItem = 0;

	m_wDynamicObjectID   = 0;
	m_sDynamicObjectType = 0;

	m_bIsTempMoveAllowed = true;

	m_iOccupyStatus    = 0;
	m_iOccupyFlagIndex = 0;
	
	m_iAttribute  = 0;
}

CTile::~CTile()
{
 
	for(int i = 0; i < hb::server::map::TilePerItems; i++) 
	if (m_pItem[i] != 0) delete m_pItem[i];
}
