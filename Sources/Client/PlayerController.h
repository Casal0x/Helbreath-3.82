#pragma once
#include <cstdint>

class CMapData;

class CPlayerController
{
public:
	CPlayerController();
	void Reset();

	// Direction offset lookup tables
	static const char DIR_OFFSET_X[9];
	static const char DIR_OFFSET_Y[9];

	// Destination
	void SetDestination(short x, short y) { m_sDestX = x; m_sDestY = y; }
	void MoveDestination(short dx, short dy) { m_sDestX += dx; m_sDestY += dy; }
	short GetDestinationX() const { return m_sDestX; }
	short GetDestinationY() const { return m_sDestY; }
	void ClearDestination() { m_sDestX = 0; m_sDestY = 0; }

	// Command State
	char GetCommand() const { return m_cCommand; }
	void SetCommand(char cmd) { m_cCommand = cmd; }

	bool IsCommandAvailable() const { return m_bCommandAvailable; }
	void SetCommandAvailable(bool available) { m_bCommandAvailable = available; }

	uint32_t GetCommandTime() const { return m_dwCommandTime; }
	void SetCommandTime(uint32_t time) { m_dwCommandTime = time; }

	// Command Throttling
	char GetCommandCount() const { return m_cCommandCount; }
	void SetCommandCount(char count) { m_cCommandCount = count; }
	void IncrementCommandCount() { m_cCommandCount++; }
	void DecrementCommandCount() { if (m_cCommandCount > 0) m_cCommandCount--; }
	void ResetCommandCount() { m_cCommandCount = 0; }

	// Previous Move Tracking (for blocked move detection)
	int GetPrevMoveX() const { return m_iPrevMoveX; }
	int GetPrevMoveY() const { return m_iPrevMoveY; }
	void SetPrevMove(int x, int y) { m_iPrevMoveX = x; m_iPrevMoveY = y; }

	bool IsPrevMoveBlocked() const { return m_bIsPrevMoveBlocked; }
	void SetPrevMoveBlocked(bool blocked) { m_bIsPrevMoveBlocked = blocked; }

	// Turn Direction (0 = clockwise, 1 = counter-clockwise for obstacle avoidance)
	char GetPlayerTurn() const { return m_cPlayerTurn; }
	void SetPlayerTurn(char turn) { m_cPlayerTurn = turn; }

	// Pending stop direction (for right-click stop while moving)
	char GetPendingStopDir() const { return m_cPendingStopDir; }
	void SetPendingStopDir(char dir) { m_cPendingStopDir = dir; }
	void ClearPendingStopDir() { m_cPendingStopDir = 0; }

	// Attack animation cooldown (time-based minimum to match server validation)
	uint32_t GetAttackEndTime() const { return m_dwAttackEndTime; }
	void SetAttackEndTime(uint32_t time) { m_dwAttackEndTime = time; }

	// Direction Calculation
	// Returns direction 1-8 to move from (sX,sY) toward (dstX,dstY), or 0 if no valid move
	// bMoveCheck: if true, considers previously blocked moves
	// bMIM: if true, reverses source/destination for direction calculation (illusion movement)
	char GetNextMoveDir(short sX, short sY, short dstX, short dstY,
	                    CMapData* pMapData, bool bMoveCheck = false, bool bMIM = false);

	// Calculates optimal turn direction (clockwise vs counter-clockwise) to reach destination
	// Sets m_cPlayerTurn based on which direction reaches destination in fewer steps
	void CalculatePlayerTurn(short playerX, short playerY, CMapData* pMapData);

private:
	// Destination coordinates
	short m_sDestX;
	short m_sDestY;

	// Current command
	char m_cCommand;

	// Command availability and timing
	bool m_bCommandAvailable;
	uint32_t m_dwCommandTime;
	char m_cCommandCount;

	// Previous move tracking
	int m_iPrevMoveX;
	int m_iPrevMoveY;
	bool m_bIsPrevMoveBlocked;

	// Turn direction for obstacle avoidance
	char m_cPlayerTurn;

	// Pending stop direction (applied when movement ends)
	char m_cPendingStopDir;

	// Earliest time player can act after an attack (prevents false positive server disconnects)
	uint32_t m_dwAttackEndTime;
};
