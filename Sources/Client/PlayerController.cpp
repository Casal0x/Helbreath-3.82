#include "Game.h"
#include "PlayerController.h"
#include "MapData.h"
#include "Misc.h"
#include "DirectionHelpers.h"

CPlayerController::CPlayerController()
{
	reset();
}

void CPlayerController::reset()
{
	m_dest_x = 0;
	m_dest_y = 0;
	m_command = 0;
	m_command_available = true;
	m_command_time = 0;
	m_command_count = 0;
	m_prev_move_x = 0;
	m_prev_move_y = 0;
	m_is_prev_move_blocked = false;
	m_player_turn = 0;
	m_pending_stop_dir = 0;
	m_attack_end_time = 0;
}

char CPlayerController::get_next_move_dir(short sX, short sY, short dstX, short dstY,
                                        CMapData* map_data, bool move_check, bool mim)
{
	char dir, tmp_dir;
	int aX, aY, dX, dY;
	int i;

	if ((sX == dstX) && (sY == dstY)) return 0;

	dX = sX;
	dY = sY;

	if (mim == false)
		dir = CMisc::get_next_move_dir(dX, dY, dstX, dstY);
	else
		dir = CMisc::get_next_move_dir(dstX, dstY, dX, dY);

	if (dir < 1 || dir > 8) return 0;

	if (m_player_turn == 0)
	{
		for (i = dir; i <= dir + 2; i++)
		{
			tmp_dir = i;
			if (tmp_dir > 8) tmp_dir -= 8;
			aX = hb::shared::direction::OffsetX[tmp_dir];
			aY = hb::shared::direction::OffsetY[tmp_dir];
			if (((dX + aX) == m_prev_move_x) && ((dY + aY) == m_prev_move_y) && (m_is_prev_move_blocked == true) && (move_check == true))
			{
				m_is_prev_move_blocked = false;
			}
			else if (map_data->get_is_locatable(dX + aX, dY + aY) == true)
			{
				if (map_data->is_teleport_loc(dX + aX, dY + aY) == true)
				{
					return tmp_dir;
				}
				else return tmp_dir;
			}
		}
	}

	if (m_player_turn == 1)
	{
		for (i = dir; i >= dir - 2; i--)
		{
			tmp_dir = i;
			if (tmp_dir < 1) tmp_dir += 8;
			aX = hb::shared::direction::OffsetX[tmp_dir];
			aY = hb::shared::direction::OffsetY[tmp_dir];
			if (((dX + aX) == m_prev_move_x) && ((dY + aY) == m_prev_move_y) && (m_is_prev_move_blocked == true) && (move_check == true))
			{
				m_is_prev_move_blocked = false;
			}
			else if (map_data->get_is_locatable(dX + aX, dY + aY) == true)
			{
				if (map_data->is_teleport_loc(dX + aX, dY + aY) == true)
				{
					return tmp_dir;
				}
				else return tmp_dir;
			}
		}
	}

	return 0;
}

// Determines optimal turn direction (left/right bias) for pathfinding.
// Tries both directions with a 30-step cap to prevent infinite loops on unreachable targets.
void CPlayerController::calculate_player_turn(short playerX, short playerY, CMapData* map_data)
{
	char dir;
	short sX, sY, cnt1, cnt2;

	sX = playerX;
	sY = playerY;
	cnt1 = 0;
	m_player_turn = 0;

	while (1)
	{
		dir = get_next_move_dir(sX, sY, m_dest_x, m_dest_y, map_data);
		if (dir == 0) break;
		hb::shared::direction::ApplyOffset(dir, sX, sY);
		cnt1++;
		if (cnt1 > 30) break;
	}

	sX = playerX;
	sY = playerY;
	cnt2 = 0;
	m_player_turn = 1;

	while (1)
	{
		dir = get_next_move_dir(sX, sY, m_dest_x, m_dest_y, map_data);
		if (dir == 0) break;
		hb::shared::direction::ApplyOffset(dir, sX, sY);
		cnt2++;
		if (cnt2 > 30) break;
	}

	if (cnt1 > cnt2)
		m_player_turn = 0;
	else
		m_player_turn = 1;
}
