#pragma once
// -------------------------------------------------------------------------------

#include "user.h"
// -------------------------------------------------------------------------------

class SummonTheDemons  
{
public:
			SummonTheDemons();
	virtual	~SummonTheDemons();
	// ----
	bool	DropScroll(LPOBJ lpUser, WORD ItemType);
	void	DropReward(LPOBJ lpUser, LPOBJ lpMonster);
	void	Mix(LPOBJ lpUser);
	// ----
	bool	IsSummon(int MonsterType) { return MonsterType >= 271 && MonsterType <= 275; };
	bool	IsScroll(WORD ItemType) { return ItemType >= ITEMGET(14, 217) && ItemType <= ITEMGET(14, 221); };
	// ----
private:
	BYTE	m_Grade1MixRate;
	int		m_Grade1MixMoney;
	BYTE	m_Grade2MixRate;
	int		m_Grade2MixMoney;
	BYTE	m_Grade3MixRate;
	int		m_Grade3MixMoney;
	BYTE	m_Grade4MixRate;
	int		m_Grade4MixMoney;
	// ----
}; extern SummonTheDemons g_SummonTheDemons;
// -------------------------------------------------------------------------------
