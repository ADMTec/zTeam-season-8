#include "stdafx.h"
#include "SummonTheDemons.h"
#include "logproc.h"
#include "GameMain.h"
#include "ChaosBox.h"
#include "..\common\winutil.h"
#include "DSProtocol.h"
// -------------------------------------------------------------------------------
SummonTheDemons g_SummonTheDemons;
// -------------------------------------------------------------------------------

SummonTheDemons::SummonTheDemons()
{
	this->m_Grade1MixRate	= 70;
	this->m_Grade2MixRate	= 70;
	this->m_Grade3MixRate	= 70;
	this->m_Grade4MixRate	= 70;
	this->m_Grade1MixMoney	= 100000;
	this->m_Grade2MixMoney	= 200000;
	this->m_Grade3MixMoney	= 300000;
	this->m_Grade4MixMoney	= 400000;
}
// -------------------------------------------------------------------------------

SummonTheDemons::~SummonTheDemons()
{

}
// -------------------------------------------------------------------------------

bool SummonTheDemons::DropScroll(LPOBJ lpUser, WORD ItemType)
{
	WORD MonsterType	= ItemType - 6731;
	BYTE MapAttr		= MapC[lpUser->MapNumber].GetAttr(lpUser->X, lpUser->Y);
	// ----
	if( MapAttr & 1 != 0 )
	{
		LogAddTD("[SummonTheDemons] [%s] [%s] Drop fail, used in safe zone",
			lpUser->AccountID, lpUser->Name);
		return false;
	}
	// ----
	int MonsterIndex = gObjAddMonster(lpUser->MapNumber);
	// ----
	if( MonsterIndex < 0 )
	{
		LogAddTD("[SummonTheDemons] [%s] [%s] Drop fail, monster index error",
			lpUser->AccountID, lpUser->Name);
		return false;
	}
	// ----
	gObj[MonsterIndex].m_PosNum		= -1;
    gObj[MonsterIndex].X			= lpUser->X + rand() % 2;
    gObj[MonsterIndex].Y			= lpUser->Y + rand() % 2;
    gObj[MonsterIndex].MapNumber	= lpUser->MapNumber;
    gObj[MonsterIndex].TX			= gObj[MonsterIndex].X;
    gObj[MonsterIndex].TY			= gObj[MonsterIndex].Y;
    gObj[MonsterIndex].m_OldX		= gObj[MonsterIndex].X;
    gObj[MonsterIndex].m_OldY		= gObj[MonsterIndex].Y;
    gObj[MonsterIndex].StartX		= gObj[MonsterIndex].X;
    gObj[MonsterIndex].StartY		= gObj[MonsterIndex].Y;
	gObj[MonsterIndex].m_iRegenType	= 0;
	// ----
	LPMONSTER_ATTRIBUTE lpMonAttr = gMAttr.GetAttr(MonsterType);
	// ----
	if( lpMonAttr == NULL )
	{
		gObjDel(MonsterIndex);
		LogAddTD("[SummonTheDemons] [%s] [%s] Drop fail, monster attr error",
			lpUser->AccountID, lpUser->Name);
		return false;
	}
	// ----
    gObjSetMonster(MonsterIndex, MonsterType);
    gObj[MonsterIndex].Dir = rand() % 8;
	// ----
	return true;
}
// -------------------------------------------------------------------------------

void SummonTheDemons::Mix(LPOBJ lpUser)
{
	PMSG_CHAOSMIXRESULT pMsg;
	PHeadSetB((LPBYTE)&pMsg.h, 0x86, sizeof(PMSG_CHAOSMIXRESULT));
	pMsg.Result = CB_ERROR;
	// ----
	lpUser->ChaosLock = TRUE;
	// ----
	BYTE SummonLv1Count = 0;
	BYTE SummonLv2Count = 0;
	BYTE SummonLv3Count = 0;
	BYTE SummonLv4Count = 0;
	
	BYTE OtherItemCount	= 0;
	// ----
	for( int n = 0; n < CHAOS_BOX_SIZE; n++ )
	{
		if( !lpUser->pChaosBox[n].IsItem() )
		{
			continue;
		}
		// ----
		if( lpUser->pChaosBox[n].m_Type == ITEMGET(14, 217) )
		{
			SummonLv1Count++;
		}
		else if( lpUser->pChaosBox[n].m_Type == ITEMGET(14, 218) )
		{
			SummonLv2Count++;
		}
		else if( lpUser->pChaosBox[n].m_Type == ITEMGET(14, 219) )
		{
			SummonLv3Count++;
		}
		else if( lpUser->pChaosBox[n].m_Type == ITEMGET(14, 220) )
		{
			SummonLv4Count++;
		}
		else
		{
			OtherItemCount++;
		}
	}
	// ----
	if( OtherItemCount != 0 )
	{
		DataSend(lpUser->m_Index, (LPBYTE)&pMsg, pMsg.h.size);
		lpUser->ChaosLock = FALSE;
		return;
	}
	// ----
	if(		SummonLv1Count == 3 
		&&	SummonLv2Count == 0
		&&	SummonLv3Count == 0 
		&&	SummonLv4Count == 0 )
		
	{
		if( lpUser->Money < this->m_Grade1MixMoney )
		{
			pMsg.Result = CB_NOT_ENOUGH_ZEN;
			DataSend(lpUser->m_Index, (LPBYTE)&pMsg, pMsg.h.size);
			lpUser->ChaosLock = FALSE;
			return;
		}
		// ----
		lpUser->Money -= this->m_Grade1MixMoney;
		GCMoneySend(lpUser->m_Index, lpUser->Money);
		// ----
		lpUser->ChaosSuccessRate = this->m_Grade1MixRate;
		// ----
		if( rand() % 100 < lpUser->ChaosSuccessRate )
		{
			ItemSerialCreateSend(lpUser->m_Index, 255, 0, 0, ITEMGET(14, 217), 0, 1, 0, 0, 0, lpUser->m_Index, 0, 0);
			gObjInventoryCommit(lpUser->m_Index);
			LogAddTD("[SummonTheDemons][ChaosMix] Grade 1 Success [%s][%s], Rate: %d, Money: %d / %d",
				lpUser->AccountID, lpUser->Name, lpUser->ChaosSuccessRate, lpUser->Money, this->m_Grade1MixMoney);
		}
		else
		{
			g_ChaosBox.ChaosBoxInit(lpUser);
			GCUserChaosBoxSend(lpUser, 0);
			DataSend(lpUser->m_Index, (LPBYTE)&pMsg, pMsg.h.size);
			LogAddTD("[SummonTheDemons][ChaosMix] Grade 1 Fail [%s][%s], Rate: %d, Money: %d / %d",
				lpUser->AccountID, lpUser->Name, lpUser->ChaosSuccessRate, lpUser->Money, this->m_Grade1MixMoney);
		}
	}
	else if(	SummonLv1Count == 1
			&&	SummonLv2Count == 1
			&&	SummonLv3Count == 0 
		    &&	SummonLv4Count == 0 )
	{
		if( lpUser->Money < this->m_Grade2MixMoney )
		{
			pMsg.Result = CB_NOT_ENOUGH_ZEN;
			DataSend(lpUser->m_Index, (LPBYTE)&pMsg, pMsg.h.size);
			lpUser->ChaosLock = FALSE;
			return;
		}
		// ----
		lpUser->Money -= this->m_Grade2MixMoney;
		GCMoneySend(lpUser->m_Index, lpUser->Money);
		// ----
		lpUser->ChaosSuccessRate = this->m_Grade2MixRate;
		// ----
		if( rand() % 100 < lpUser->ChaosSuccessRate )
		{
			ItemSerialCreateSend(lpUser->m_Index, 255, 0, 0, ITEMGET(14, 218), 0, 1, 0, 0, 0, lpUser->m_Index, 0, 0);
			gObjInventoryCommit(lpUser->m_Index);
			LogAddTD("[SummonTheDemons][ChaosMix] Grade 2 Success [%s][%s], Rate: %d, Money: %d / %d",
				lpUser->AccountID, lpUser->Name, lpUser->ChaosSuccessRate, lpUser->Money, this->m_Grade2MixMoney);
		}
		else
		{
			g_ChaosBox.ChaosBoxInit(lpUser);
			GCUserChaosBoxSend(lpUser, 0);
			DataSend(lpUser->m_Index, (LPBYTE)&pMsg, pMsg.h.size);
			LogAddTD("[SummonTheDemons][ChaosMix] Grade 2 Fail [%s][%s], Rate: %d, Money: %d / %d",
				lpUser->AccountID, lpUser->Name, lpUser->ChaosSuccessRate, lpUser->Money, this->m_Grade2MixMoney);
		}
	}
	else if(	SummonLv1Count == 1
			&&	SummonLv2Count == 0
			&&	SummonLv3Count == 1 
			&&	SummonLv4Count == 0 )
	{
		if( lpUser->Money < this->m_Grade3MixMoney )
		{
			pMsg.Result = CB_NOT_ENOUGH_ZEN;
			DataSend(lpUser->m_Index, (LPBYTE)&pMsg, pMsg.h.size);
			lpUser->ChaosLock = FALSE;
			return;
		}
		// ----
		lpUser->Money -= this->m_Grade3MixMoney;
		GCMoneySend(lpUser->m_Index, lpUser->Money);
		// ----
		lpUser->ChaosSuccessRate = this->m_Grade3MixRate;
		// ----
		if( rand() % 100 < lpUser->ChaosSuccessRate )
		{
			ItemSerialCreateSend(lpUser->m_Index, 255, 0, 0, ITEMGET(14, 219), 0, 1, 0, 0, 0, lpUser->m_Index, 0, 0);
			gObjInventoryCommit(lpUser->m_Index);
			LogAddTD("[SummonTheDemons][ChaosMix] Grade 3 Success [%s][%s], Rate: %d, Money: %d / %d",
				lpUser->AccountID, lpUser->Name, lpUser->ChaosSuccessRate, lpUser->Money, this->m_Grade3MixMoney);
		}
		else
		{
			g_ChaosBox.ChaosBoxInit(lpUser);
			GCUserChaosBoxSend(lpUser, 0);
			DataSend(lpUser->m_Index, (LPBYTE)&pMsg, pMsg.h.size);
			LogAddTD("[SummonTheDemons][ChaosMix] Grade 3 Fail [%s][%s], Rate: %d, Money: %d / %d",
				lpUser->AccountID, lpUser->Name, lpUser->ChaosSuccessRate, lpUser->Money, this->m_Grade3MixMoney);
		}
	}
	else if(	SummonLv1Count == 1
			&&	SummonLv2Count == 0
			&&	SummonLv3Count == 0
			&&	SummonLv4Count == 1 )
	{
		if( lpUser->Money < this->m_Grade4MixMoney )
		{
			pMsg.Result = CB_NOT_ENOUGH_ZEN;
			DataSend(lpUser->m_Index, (LPBYTE)&pMsg, pMsg.h.size);
			lpUser->ChaosLock = FALSE;
			return;
		}
		
	// ----
	
		if( rand() % 100 < lpUser->ChaosSuccessRate )
		{
			ItemSerialCreateSend(lpUser->m_Index, 255, 0, 0, ITEMGET(14, 220), 0, 1, 0, 0, 0, lpUser->m_Index, 0, 0);
			gObjInventoryCommit(lpUser->m_Index);
			LogAddTD("[SummonTheDemons][ChaosMix] Grade 4 Success [%s][%s], Rate: %d, Money: %d / %d",
				lpUser->AccountID, lpUser->Name, lpUser->ChaosSuccessRate, lpUser->Money, this->m_Grade4MixMoney);
		}
		else
		{
			g_ChaosBox.ChaosBoxInit(lpUser);
			GCUserChaosBoxSend(lpUser, 0);
			DataSend(lpUser->m_Index, (LPBYTE)&pMsg, pMsg.h.size);
			LogAddTD("[SummonTheDemons][ChaosMix] Grade 4 Fail [%s][%s], Rate: %d, Money: %d / %d",
				lpUser->AccountID, lpUser->Name, lpUser->ChaosSuccessRate, lpUser->Money, this->m_Grade4MixMoney);
		}
	}
	
	//---------------------
	
	lpUser->ChaosLock = FALSE;
}
// -------------------------------------------------------------------------------