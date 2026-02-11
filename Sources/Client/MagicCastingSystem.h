#pragma once

class CGame;

class MagicCastingSystem
{
public:
	static MagicCastingSystem& Get();

	void SetGame(CGame* pGame);

	int GetManaCost(int iMagicNo);
	void BeginCast(int iMagicNo);

private:
	MagicCastingSystem() = default;
	CGame* m_game = nullptr;
};
