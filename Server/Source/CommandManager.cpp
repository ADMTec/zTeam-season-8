#include "stdafx.h"
#include "CommandManager.h"
#include "..\pugixml\pugixml.hpp"
#include "GameMain.h"
#include "DSProtocol.h"
#include "BuffEffectSlot.h"
#include "..\common\winutil.h"
#include "CastleSiege.h"
#include "Crywolf.h"
#include "MoveCommand.h"
#include "Gate.h"
#include "MultiWareHouseSystem.h"
#include "SProtocol.h"
#if (ENABLE_CUSTOM_OFFLINETRADE == 1)
#include "OfflineTrade.h"
#endif
// -------------------------------------------------------------------------------

CLogToFile g_GMChatLog("GMSystem", ".\\LOG\\GMLog", TRUE);
CLogToFile g_USystemLog("USystem", ".\\LOG\\ULog", TRUE);
CLogToFile g_PostLog("Post", ".\\LOG\\Post", TRUE);


using namespace pugi;
CommandManager g_CommandManager;
// -------------------------------------------------------------------------------

CommandManager::CommandManager()
{

}
// -------------------------------------------------------------------------------

CommandManager::~CommandManager()
{

}
// -------------------------------------------------------------------------------
			//int tempindex;
			//int aIndex;

void CommandManager::Init()
{
	this->m_CommandInfo.clear();
	if( this->m_CommandInfo.capacity() > 0 )
	{
		std::vector<CommandInfo>().swap(this->m_CommandInfo);
	}
}
// -------------------------------------------------------------------------------

void CommandManager::Load()
{
	this->Init();
	this->Read(gDirPath.GetNewPath(FILE_CUSTOM_COMMANDMANAGER));
}
// -------------------------------------------------------------------------------

void CommandManager::Read(LPSTR File)
{
	xml_document Document;
	xml_parse_result Result = Document.load_file(File);
	// ----
	if( Result.status != status_ok )
	{
		MsgBox("[CommandManager] File %s not found! %d", File, Result.status);
		return;
	}
	// ----
	xml_node CommandManager = Document.child("commandmanager");
	xml_node CommandList = CommandManager.child("commandlist");
	// ----
	for( xml_node Node = CommandList.child("command"); Node; Node = Node.next_sibling() )
	{
		CommandInfo lpCommand = { 0 };
		lpCommand.Index = Node.attribute("id").as_int();
		lpCommand.Access = Node.attribute("access").as_int();
		lpCommand.MinLevel = Node.attribute("minlevel").as_int();
		lpCommand.MinReset = Node.attribute("minreset").as_int();
		lpCommand.PriceType = Node.attribute("pricetype").as_int();
		lpCommand.Price = Node.attribute("price").as_int();
		lpCommand.PremiumAccess = Node.attribute("premium").as_int();
		strcpy(lpCommand.Text, Node.text().as_string());
		this->m_CommandInfo.push_back(lpCommand);
	}
}
// -------------------------------------------------------------------------------

void CommandManager::Run(LPOBJ lpUser, LPSTR Text)
{
	if (g_SelfDefenseOn)
	{
		if ((GetTickCount() - lpUser->MySelfDefenseTime) < g_SelfDefenseTime * 1000) 
		{
			GCServerMsgStringSend(lMsg.Get(1133), lpUser->m_Index, 1);
			return;
		}
	}
	// ----
	LPSTR Command = { 0 };
	char Buffer[250];
	char Separator[2] = " ";
	// ----
	strcpy(Buffer, Text);
	Command = strtok(Buffer, Separator);
	// ----
	CommandInfo* lpCommand = this->GetCommand(Command);
	BYTE CheckStatus = this->CheckCommand(lpUser, Command);	

	if (lpCommand == NULL) {
		return;
	}

	if( CheckStatus != 1)
	{
		if( CheckStatus == 0 )
		{
			MsgOutput(lpUser->m_Index, "Wrong command");
		}
		else if( CheckStatus == 2 )
		{
			MsgOutput(lpUser->m_Index, "Your access code is wrong for this command");
		}
		else if( CheckStatus == 3 
			&& lpCommand->Index != Command::PKReset) // special calculation
		{
			MsgOutput(lpUser->m_Index, "You are short money for this command");
		}
		else if( CheckStatus == 4 )
		{
			MsgOutput(lpUser->m_Index, "Your premium access code is wrong for this command");
		}
		else if( CheckStatus == 5 )
		{
			MsgOutput(lpUser->m_Index, "Your level is small for use this command");
		}
		else if( CheckStatus == 6 )
		{
			MsgOutput(lpUser->m_Index, "Your reset is small for use this command");
		}
		// ----
		return;
	}
	// ----
	
	// ----
	switch(lpCommand->Index)
	{
	case Command::BanChat:
		{
			LPSTR UserName = this->GetTokenString();
			int Time = this->GetTokenNumber();
			// ----
			if( UserName == 0 || Time == -1 )
			{
				MsgOutput(lpUser->m_Index, "Syntax error in command");
				return;
			}
			// ----
			LPOBJ lpTarget = this->GetUserInfo(lpUser, UserName);
			// ----
			if( !lpTarget )
			{
				return;
			}
			

			//LogAddTD("[GMSystem] [BanChat] GMName: [%s] -> Name: [%s]", lpUser->Name, lpTarget->Name);
			g_GMChatLog.Output("[GMSystem] [BanChat] GMName: [%s] -> Name: [%s]", lpUser->Name, lpTarget->Name);

			// ----
			lpTarget->ChatLimitTime = Time;

		}
		break;
		// --
	case Command::BanUser: //Need DB request for set new ctrlcode
		{
			MsgOutput(lpUser->m_Index, "Command temporarily not working");
			return;
			/*LPSTR UserName = this->GetTokenString();
			// ----
			if( UserName == 0 )
			{
			MsgOutput(lpUser->m_Index, "Syntax error in command");
			return;
			}
			// ----
			LPOBJ lpTarget = this->GetUserInfo(lpUser, UserName);
			// ----
			if( !lpTarget )
			{
			return;
			}
			// ----
			lpTarget->Authority = 1;*/
		}
		break;
		// --
	case Command::BanAccount:
		{
			MsgOutput(lpUser->m_Index, "Command temporarily not working");
			return;
		}
		break;
		// --
	case Command::Disconnect:
		{
			LPSTR UserName = this->GetTokenString();
			// ----
			if( UserName == 0 )
			{
				MsgOutput(lpUser->m_Index, "Syntax error in command");
				return;
			}
			// ----
			LPOBJ lpTarget = this->GetUserInfo(lpUser, UserName);
			LPOBJ lpObj;
			// ----
			if( !lpTarget )
			{
				return;
			}

			//LogAddTD("[GMSystem] [Disconnect] GMName: [%s] -> Name: [%s] ", lpUser->Name, lpTarget->Name);
			g_GMChatLog.Output("[GMSystem] [Disconnect] GMName: [%s] -> Name: [%s] ", lpUser->Name, lpTarget->Name);

			// ----
			CloseClient(lpTarget->m_Index);

#if (ENABLE_CUSTOM_OFFLINETRADE == 1)
			if( lpTarget->bOffTrade )
			{
				gObjDel(lpTarget->m_Index);
			}
#endif
		}
		break;
		// --
	case Command::PKSet:
		{
			LPSTR UserName = this->GetTokenString();
			int Level = this->GetTokenNumber();
			// ----
			if( UserName == 0 || Level == -1 )
			{
				MsgOutput(lpUser->m_Index, "Syntax error in command");
				return;
			}
			// ----
			LPOBJ lpTarget = this->GetUserInfo(lpUser, UserName);
			// ----
			if( !lpTarget )
			{
				return;
			}
			// ----
			lpTarget->m_PK_Level = Level;
			// ----
			if( lpTarget->PartyNumber >= 0 )
			{
				gParty.SetPkLevel(lpTarget->PartyNumber, lpTarget->m_Index, lpTarget->DBNumber, lpTarget->m_PK_Level);
				gParty.SetPkCount(lpTarget->PartyNumber);
			}
			// ----
			GCPkLevelSend(lpTarget->m_Index, lpTarget->m_PK_Level);
		}
		break;

		/* // Old
		// --
	case Command::PKReset:
		{
			LPSTR UserName = this->GetTokenString();
			// ----
			if( UserName == 0 )
			{
				MsgOutput(lpUser->m_Index, "Syntax error in command");
				return;
			}
			// ----
			LPOBJ lpTarget = this->GetUserInfo(lpUser, UserName);
			// ----
			if( !lpTarget )
			{
				return;
			}
			// ----
			if (lpUser->Money < lpCommand->Price * lpTarget->m_PK_Count) {
				MsgOutput(lpUser->m_Index, "You are short of money");
				return;
			}

			lpUser->Money -= lpCommand->Price * lpTarget->m_PK_Count;
			GCMoneySend(lpUser->m_Index, lpUser->Money);
			
			lpTarget->m_PK_Count = 0;
			lpTarget->m_PK_Level = 3;
			lpTarget->m_PK_Time	= 0;
			// ----
			if( lpTarget->PartyNumber >= 0 )
			{
				gParty.SetPkLevel(lpTarget->PartyNumber, lpTarget->m_Index, lpTarget->DBNumber, lpTarget->m_PK_Level);
				gParty.SetPkCount(lpTarget->PartyNumber);
			}
			// ----
			GCPkLevelSend(lpTarget->m_Index, lpTarget->m_PK_Level);
		}
		break;
		// --

		*/
		case Command::PKReset:
		{
			// ----

			if ( lpUser->m_PK_Level <= 3 )
			{
				MsgOutput(lpUser->m_Index, "You are not PK.");
				return;
			}

			if (lpUser->Money < lpCommand->Price * lpUser->m_PK_Count) {
				MsgOutput(lpUser->m_Index, "You are short of money");
				return;
			}

			lpUser->Money -= lpCommand->Price * lpUser->m_PK_Count;
			GCMoneySend(lpUser->m_Index, lpUser->Money);
			
			lpUser->m_PK_Count = 0;
			lpUser->m_PK_Level = 3;
			lpUser->m_PK_Time	= 0;
			// ----
			if( lpUser->PartyNumber >= 0 )
			{
				gParty.SetPkLevel(lpUser->PartyNumber, lpUser->m_Index, lpUser->DBNumber, lpUser->m_PK_Level);
				gParty.SetPkCount(lpUser->PartyNumber);
			}
			// ----

			GCPkLevelSend(lpUser->m_Index, lpUser->m_PK_Level);
		}
		break;
		// --

	case Command::MoneySet:
		{
			LPSTR UserName = this->GetTokenString();
			int Money = this->GetTokenNumber();
			// ----

			if( UserName == 0 || Money == -1 || Money == 1000000000)
			{
				MsgOutput(lpUser->m_Index, "Syntax error in command");
				return;
			}

			// ----
			LPOBJ lpTarget = this->GetUserInfo(lpUser, UserName);
			// ----
			if( !lpTarget )
			{
				return;
			}
			// ----
			lpTarget->Money = Money;

			//LogAddTD("[GMSystem] [MoneySet] GMName: [%s] -> Name: [%s] Money: [%d] ", lpUser->Name, lpTarget->Name, lpTarget->Money);
			g_GMChatLog.Output("[GMSystem] [MoneySet] GMName: [%s] -> Name: [%s] Money: [%d] ", lpUser->Name, lpTarget->Name, lpTarget->Money);

			GCMoneySend(lpTarget->m_Index, lpTarget->Money);
		}
		break;
		// --
	case Command::MakeItem:
		{
			int EBP798;
			int EBP79C = 0;
			int EBP7A0 = 0;
			int EBP7A4 = 0;
			BYTE EBP7A8 = 0;
			int iDur = (BYTE)-1;//7AC
			int EBP7B0;

			int ItemCount = this->GetTokenNumber();
			int ItemType  = this->GetTokenNumber();
			int ItemIndex  = this->GetTokenNumber();
			EBP798 = this->GetTokenNumber();
			iDur = this->GetTokenNumber();
			EBP79C = this->GetTokenNumber();
			EBP7A0 = this->GetTokenNumber();
			EBP7B0 = this->GetTokenNumber();

			if( ItemCount <= 0 )
			{
				ItemCount = 1;
			}

			if( ItemCount > 10 )
			{
				ItemCount = 10;
			}

			if( ItemType < 0 || ItemType > 15 )
			{
				MsgOutput(lpUser->m_Index, "Syntax error in command");
				return;
			}

			if( ItemIndex < 0 || ItemIndex > 512 )
			{
				MsgOutput(lpUser->m_Index, "Syntax error in command");
				return;
			}
			
			int ItemCode = ITEMGET(ItemType, ItemIndex);
			
			if( ItemCode < ITEMGET(0, 0) || ItemCode > ITEMGET(15, 512) )
			{
				MsgOutput(lpUser->m_Index, "Syntax error in command");
				return;
			}

			if( !ItemAttribute[ItemCode].HaveItemInfo )
			{
				MsgOutput(lpUser->m_Index, "Syntax error in command");
				return;
			}

			if(ItemType == 13 && ItemIndex == 3)
			{
				if(	EBP7B0 < 8)
				{
					EBP7A4 = EBP7B0;
				}
			}
			else
			{
				if( EBP7B0 >= 4 )
				{
					EBP7A4 = EBP7B0 / 4;
				}
				else
				{
					EBP7B0 = 0;
				}
			}

			int EBP7B4 = this->GetTokenNumber();
			int EBP7B8 = this->GetTokenNumber();

			if(EBP7B4 != 0 )
			{
				if( EBP7B8 == 0)
				{
					if((rand()%100) < 80)
					{
						EBP7B4 |= 4; 
					}
					else
					{
						EBP7B4 |= 8; 
					}
				}
				else if (EBP7B8 == 1) 
				{
					EBP7B4 |= 4; 
				}
				else
				{
					EBP7B4 |= 8; 
				}
			}

			int EBP7BC = this->GetTokenNumber();

			if(EBP7BC > 0)
			{
				EBP7A8 |= 0x20;
			}

			int EBP7C0 = this->GetTokenNumber();

			if(EBP7C0 > 0)
			{
				EBP7A8 |= 0x10;
			}

			int EBP7C4 = this->GetTokenNumber();

			if(EBP7C4 > 0)
			{
				EBP7A8 |= 0x08;
			}

			int EBP7C8 = this->GetTokenNumber();

			if(EBP7C8 > 0)
			{
				EBP7A8 |= 0x04;
			}

			int EBP7CC = this->GetTokenNumber();

			if(EBP7CC > 0)
			{
				EBP7A8 |= 0x02;
			}

			int EBP7D0 = this->GetTokenNumber();

			if(EBP7D0 > 0)
			{
				EBP7A8 |= 0x01;
			}

			if( ItemType >= 0 && ItemType < 512 &&
				ItemIndex >= 0 && ItemIndex < 512)
			{
				int iItemNumber = ItemGetNumberMake(ItemType, ItemIndex);

				if( iItemNumber == ITEMGET(0,19) || 
					iItemNumber == ITEMGET(4,18) || 
					iItemNumber == ITEMGET(5,10) || 
					iItemNumber == ITEMGET(2,13))
				{
					EBP7A8 = 63;	
				}


				//LogAddTD("[GMSystem] [Item Create] Name: [%s] Map: [%d] X: [%d] Y: [%d] Item: [%d] Level: [%d] Dur: [%d] Skill: [%d] Luck: [%d] Opt: [%d] ExcOpt: [%d] AncOpt: [%d]", lpUser->Name, lpUser->MapNumber, lpUser->X, lpUser->Y, iItemNumber, EBP798, iDur, EBP79C, EBP7A0, EBP7A4, EBP7A8, EBP7B4);
				g_GMChatLog.Output("[GMSystem] [Item Create] Name: [%s] Map: [%d] X: [%d] Y: [%d] Item: [%d] Level: [%d] Dur: [%d] Skill: [%d] Luck: [%d] Opt: [%d] ExcOpt: [%d] AncOpt: [%d]", lpUser->Name, lpUser->MapNumber, lpUser->X, lpUser->Y, iItemNumber, EBP798, iDur, EBP79C, EBP7A0, EBP7A4, EBP7A8, EBP7B4);

				for( int i =0;i < ItemCount; i++ )
				{
					ItemSerialCreateSend(lpUser->m_Index, lpUser->MapNumber, lpUser->X, lpUser->Y, iItemNumber, EBP798, iDur, EBP79C, EBP7A0, EBP7A4, -1, EBP7A8, EBP7B4); 
				}

			}
		}
		break;
		// --
	case Command::MakeRandomSet:
		{
			int Count = this->GetTokenNumber();
			// ----
			if( Count == -1 || Count > 10)
			{
				MsgOutput(lpUser->m_Index, "Out of range 1-10");
				return;
			}

			if( Count <= 0 )
			{
				Count = 1;
			}

			if( Count > 10 )
			{
				Count = 10;
			}

			// ----

			//LogAddTD("[GMSystem] [RandomSet] GMName: [%s] Count: [%d]", lpUser->Name, Count);
			g_GMChatLog.Output("[GMSystem] [RandomSet] GMName: [%s] Count: [%d]", lpUser->Name, Count);

			for( int i = 0; i < Count; i++ )
			{
				MakeRandomSetItem(lpUser->m_Index);
			}
		}
		break;
		// ---
	case Command::HideOn:
		{
			gObjAddBuffEffect(lpUser, BUFF_INVISIBILITY, 0, 0, 0, 0, -10);
			gObjViewportListProtocolDestroy(lpUser);
		}
		break;
		// ---
	case Command::HideOff:
		{
			gObjRemoveBuffEffect(lpUser, BUFF_INVISIBILITY);
			gObjViewportListProtocolCreate(lpUser);
		}
		break;
		// --
	case Command::ClearDrop:
		{
			int Distance = this->GetTokenNumber();
			// ----
			if( Distance == -1 )
			{
				MsgOutput(lpUser->m_Index, "Syntax error in command");
				return;
			}
			// ----
			MapC[lpUser->MapNumber].ClearItem(lpUser, Distance);
		}
		break;
		// --
	case Command::ClearInventory:
		{
			LPSTR UserName = this->GetTokenString();
			// ----
			if( UserName == 0 )
			{
				MsgOutput(lpUser->m_Index, "Syntax error in command");
				return;
			}
			// ----
			LPOBJ lpTarget = this->GetUserInfo(lpUser, UserName);
			// ----
			if( !lpTarget )
			{
				return;
			}
			//LogAddTD("[GMSystem] [ClearInv] GMName: [%s] -> Name: [%s] ", lpUser->Name, lpTarget->Name);
			g_GMChatLog.Output("[GMSystem] [ClearInv] GMName: [%s] -> Name: [%s] ", lpUser->Name, lpTarget->Name);
			// ----
			for( int i = INVETORY_WEAR_SIZE; i < MAIN_INVENTORY_SIZE; i++ )
			{
				if( lpTarget->pInventory[i].IsItem() )
				{
					gObjInventoryDeleteItem(lpTarget->m_Index, i);
				}
			}
			// ----
			GCItemListSend(lpTarget->m_Index);
		}
		break;
		// --
	case Command::Skin:
		{
			LPSTR UserName = this->GetTokenString();
			int SkinNumber = this->GetTokenNumber();
			// ----
			if( UserName == 0 )
			{
				MsgOutput(lpUser->m_Index, "Syntax error in command");
				return;
			}

			//To test

			if(SkinNumber > 675 || SkinNumber < -1)
			{
				MsgOutput(lpUser->m_Index, "Skin out of range -1 - 675");
				return;
			}

			// ----
			LPOBJ lpTarget = this->GetUserInfo(lpUser, UserName);
			// ----
			if( !lpTarget )
			{
				return;
			}
			// ----

			lpTarget->m_Change = SkinNumber;

			//LogAddTD("[GMSystem] [Skin] GMName: [%s] -> Name: [%s] Skin: [%d]", lpUser->Name, lpTarget->Name, SkinNumber);
			g_GMChatLog.Output("[GMSystem] [Skin] GMName: [%s] -> Name: [%s] Skin: [%d]", lpUser->Name, lpTarget->Name, SkinNumber);
			gObjViewportListProtocolCreate(lpTarget);
		}
		break;
		// --
	case Command::PartyInfo:
		{
			LPSTR UserName = this->GetTokenString();
			char Text[80] = { 0 };
			// ----
			if( UserName == 0 )
			{
				MsgOutput(lpUser->m_Index, "Syntax error in command");
				return;
			}
			// ----
			LPOBJ lpTarget = this->GetUserInfo(lpUser, UserName);
			// ----
			if( !lpTarget )
			{
				return;
			}
			// ----
			if( lpTarget->PartyNumber < 0 )
			{
				MsgOutput(lpUser->m_Index, "Party not found");
				return;
			}
			// ----
			PARTY_STRUCT* lpParty = &gParty.m_PartyS[lpTarget->PartyNumber];
			// ----
			if( !lpParty )
			{
				MsgOutput(lpUser->m_Index, "Party data not found");
				return;
			}
			// ----
			MsgOutput(lpUser->m_Index, "Party:");
			// ----
			int PartyCount = 0;
			int PartyNumber = -1;
			// ----
			for( int i = 0; i < 5; i++ )
			{
				PartyNumber = lpParty->Number[i];
				// ----
				if( PartyNumber < 0 )
				{
					continue;
				}
				// ----
				LPOBJ lpPartyObj = &gObj[PartyNumber];
				// ----
				if( lpPartyObj != NULL )
				{
					if( lpPartyObj->Connected >= PLAYER_PLAYING )
					{
						PartyCount++;
						strcat(Text, lpPartyObj->Name);
						// ----
						if( i == 0 )
						{
							strcat(Text, "(Leader)");
						}
						// ----
						if( lpParty->Count > PartyCount)
						{
							strcat(Text, ", ");
						}
					}
				}
			}
			// ----
			MsgOutput(lpUser->m_Index, Text);
		}
		break;
		// --
	case Command::FireCrack:
		{
			for( int i = 0; i < 3; i++ )
			{
				PMSG_SERVERCMD ServerCmd = { 0 };
				PHeadSubSetB((LPBYTE)&ServerCmd, 0xF3, 0x40, sizeof(ServerCmd));
				ServerCmd.CmdType = 0;
				ServerCmd.X = lpUser->X + (rand() % 5) * 2 - 4;
				ServerCmd.Y = lpUser->Y + (rand() % 5) * 2 - 4;
				MsgSendV2(lpUser, (LPBYTE)&ServerCmd, sizeof(ServerCmd));
				DataSend(lpUser->m_Index, (LPBYTE)&ServerCmd, sizeof(ServerCmd));
			}
		}
		break;
		// --
	case Command::Trans:
		{
			LPSTR UserName = this->GetTokenString();
			int MapNumber = this->GetTokenNumber();
			int X = this->GetTokenNumber();
			int Y = this->GetTokenNumber();
			// ---
			if( UserName == 0 )
			{
				MsgOutput(lpUser->m_Index, "Syntax error in command");
				return;
			}


			if (MapNumber > 92 || MapNumber < 0)
			{
				MsgOutput(lpUser->m_Index, "Map out of range use only 0 - 92");
				return;
			}

			if (X < 1 || Y < 1 || X > 255 || Y > 255)
			{
				MsgOutput(lpUser->m_Index, "Coords out of range use only 1 - 255");
				return;
			}

			// ----
			LPOBJ lpObj;
			LPOBJ lpTarget = this->GetUserInfo(lpUser, UserName);
			// ----
			if( !lpTarget )
			{
				return;
			}
			// ----

			//LogAddTD("[GMSystem] [GMove] GMName: [%s] -> Name: [%s] Map: [%d] X: [%d] Y: [%d]", lpUser->Name, lpTarget->Name, MapNumber, X, Y);
			g_GMChatLog.Output("[GMSystem] [GMove] GMName: [%s] -> Name: [%s] Map: [%d] X: [%d] Y: [%d]", lpUser->Name, lpTarget->Name, MapNumber, X, Y);

			gObjTeleport(lpTarget->m_Index, MapNumber, X, Y);
		}
		break;
		// --
	case Command::Track:
		{
			LPSTR UserName = this->GetTokenString();
			// ---
			if( UserName == 0 )
			{
				MsgOutput(lpUser->m_Index, "Syntax error in command");
				return;
			}
			// ----
			LPOBJ lpTarget = this->GetUserInfo(lpUser, UserName);
			// ----
			if( !lpTarget )
			{
				return;
			}
			// ----
			gObjTeleport(lpTarget->m_Index, lpUser->MapNumber, lpUser->X, lpUser->Y);
		}
		break;
		// --
	case Command::Trace:
		{
			LPSTR UserName = this->GetTokenString();
			// ---
			if( UserName == 0 )
			{
				MsgOutput(lpUser->m_Index, "Syntax error in command");
				return;
			}
			// ----
			LPOBJ lpTarget = this->GetUserInfo(lpUser, UserName);
			// ----
			if( !lpTarget )
			{
				return;
			}
			// ----
			gObjTeleport(lpUser->m_Index, lpTarget->MapNumber, lpTarget->X, lpTarget->Y);
		}
		break;
		// --
	case Command::CastleSiege1:
		{
			LPSTR GuildName = this->GetTokenString();
			// ---
			if( GuildName == 0 )
			{
				MsgOutput(lpUser->m_Index, "Syntax error in command");
				return;
			}
			// ----
			g_CastleSiege.OperateGmCommand(lpUser->m_Index, 0, GuildName);
		}
		break;
		// --
	case Command::CastleSiege2:
		{
			g_CastleSiege.OperateGmCommand(lpUser->m_Index, 1, 0);
		}
		break;
		// --
	case Command::CastleSiege3:
		{
			g_CastleSiege.OperateGmCommand(lpUser->m_Index, 2, 0);
		}
		break;
		// --
	case Command::CastleSiege4:
		{
			g_CastleSiege.OperateGmCommand(lpUser->m_Index, 3, 0);
		}
		break;
		// --
	case Command::CastleSiege5:
		{
			g_CastleSiege.OperateGmCommand(lpUser->m_Index, 4, 0);
		}
		break;
		// --
	case Command::CastleSiege6:
		{
			g_CastleSiege.OperateGmCommand(lpUser->m_Index, 5, 0);
		}
		break;
		// --
	case Command::CastleSiege7:
		{
			g_CastleSiege.OperateGmCommand(lpUser->m_Index, 6, 0);
		}
		break;
		// --
	case Command::CastleSiege8:
		{
			LPSTR GuildName = this->GetTokenString();
			// ---
			if( GuildName == 0 )
			{
				MsgOutput(lpUser->m_Index, "Syntax error in command");
				return;
			}
			// ----
			g_CastleSiege.OperateGmCommand(lpUser->m_Index, 8, GuildName);
		}
		break;
		// --
	case Command::Crywolf1:
		{
			g_Crywolf.OperateGmCommand(lpUser->m_Index, 1);
		}
		break;
		// --
	case Command::Crywolf2:
		{
			g_Crywolf.OperateGmCommand(lpUser->m_Index, 2);
		}
		break;
		// --
	case Command::Crywolf3:
		{
			g_Crywolf.OperateGmCommand(lpUser->m_Index, 3);
		}
		break;
		// --
	case Command::Crywolf4:
		{
			g_Crywolf.OperateGmCommand(lpUser->m_Index, 0);
		}
		break;
		// --
	case Command::Request:
		{
			LPSTR Mode = this->GetTokenString();
			// ---
			if( Mode == 0 )
			{
				MsgOutput(lpUser->m_Index, "Syntax error in command");
				return;
			}
			// ----
			BYTE State = 3;
			// ---
			if( strcmp(Mode, "on" ) == 0 )
			{
				State = 1;
			}
			else if( strcmp(Mode, "off") == 0 )
			{
				State = 0;
			}
			// ---
			if( State >= FALSE && State <= TRUE )
			{
				gObjSetTradeOption(lpUser->m_Index, State);
				gObjSetDuelOption(lpUser->m_Index, State);
			}
		}
		break;
		// --
	case Command::PartyLeader:
		{
			LPSTR TargetName	= this->GetTokenString();
			int TargetIndex		= gObjGetIndex(TargetName);
			// ----
			if( !gParty.SetLeader(lpUser->m_Index, TargetIndex) )
			{
				MsgOutput(lpUser->m_Index, "Syntax error in command");
				return;
			}
		}
		break;
		// --
	case Command::Move:
		{
			LPSTR MapName = this->GetTokenString();
			// ---
			if( MapName == 0 )
			{
				MsgOutput(lpUser->m_Index, "Syntax error in command");
				return;
			}
			// ----
			if( lpUser->Teleport != 0 )
			{
				GCServerMsgStringSend(lMsg.Get(MSGGET(6, 68)), lpUser->m_Index, 1);
				return;
			}
			// ----
			if( (lpUser->m_IfState.use) != 0 )
			{
				if( lpUser->m_IfState.type  == 3 )
				{
					lpUser->TargetShopNumber = -1;
					lpUser->m_IfState.type = 0;
					lpUser->m_IfState.use = 0;
				}
			}
			// ----
			if( lpUser->m_IfState.use > 0 )
			{
				GCServerMsgStringSend(lMsg.Get(MSGGET(6, 68)), lpUser->m_Index, 1);
				return;
			}
			// ----
			//PK can move
			if( lpUser->m_PK_Level > 6 )
			{
				GCServerMsgStringSend(lMsg.Get(MSGGET(4, 101)), lpUser->m_Index, 1);
				return;
			}
			// ----
			gMoveCommand.Move(lpUser, MapName);
		}
		break;
		// --
	case Command::Post:
		{
			if (lpUser->ChatLimitTime > 0) {
				MsgOutput(lpUser->m_Index, lMsg.Get(MSGGET(4, 223)), lpUser->ChatLimitTime);
				return;
			}

			if(g_bPostFloodProtect == 1)
			{
				if(lpUser->m_dwPostTickCount != 0)
				{
					if ( 1000 * g_bPostFloodProtectTime >= GetTickCount() - lpUser->m_dwPostTickCount )
					{
						MsgOutput(lpUser->m_Index, "Please wait 3 seconds before next post.");
						return;
					}
				}
			}

			lpUser->m_dwPostTickCount = GetTickCount();
			
			BroadCastMessageInfo lpRequest = { 0 };
			lpRequest.h.set((LPBYTE)&lpRequest, 0xCD, sizeof(lpRequest));
			lpRequest.Type = 1;
			strcat(lpRequest.Text, &Text[strlen(Command) + 1]);

			g_PostLog.Output("Name: [%s] Message: [%s]", lpUser->Name, Text);

			CopyMemory(lpRequest.Sender, lpUser->Name, 10);
			
			wsJServerCli.DataSend((PCHAR)&lpRequest, sizeof(BroadCastMessageInfo));

			/*USER_REQ_POST lpInfo = { 0 };
			PHeadSetB((LPBYTE)&lpInfo, 0, sizeof(lpInfo));
			lpInfo.h.headcode = 2;
			lpInfo.chatmsg[0] = '!';
			memcpy(lpInfo.id, lpUser->Name, 10);
			strcat(lpInfo.chatmsg, &Text[strlen(Command) + 1]);
			DataSendAll((LPBYTE)&lpInfo, sizeof(lpInfo));*/
		}
		break;
		// --
	case Command::AddStr:
	case Command::AddAgi:
	case Command::AddVit:
	case Command::AddEne:
	case Command::AddCom:
		{
			int AddValue = this->GetTokenNumber();
			// ----
			if( AddValue < 0 || AddValue >= g_MaxStatValue )
			{
				MsgOutput(lpUser->m_Index, "Wrong value for add.");
				return;
			}
			// ----
			if( lpUser->LevelUpPoint < AddValue )
			{
				MsgOutput(lpUser->m_Index, "You do not have enough level up points!");
				return;
			}
			// ----
			if(		lpCommand->Index == Command::AddCom 
				&&	lpUser->Class != CLASS_DARKLORD )
			{
				MsgOutput(lpUser->m_Index, "Your class in invalid for this command");
				return;
			}
			// ----
			PMSG_LVPOINTADDRESULT pMsg;
			PHeadSubSetB((LPBYTE)&pMsg, 0xF3, 0x06, sizeof(pMsg));
			// ----
			if( lpCommand->Index == Command::AddStr )
			{
				if( lpUser->Strength + AddValue > g_MaxStatValue )
				{
					return;
				}
				lpUser->Strength += AddValue;
				pMsg.NewValue = lpUser->Strength;
				pMsg.ResultType = 0x15;
			}
			else if( lpCommand->Index == Command::AddAgi )
			{
				if( lpUser->Dexterity + AddValue > g_MaxStatValue )
				{
					return;
				}
				lpUser->Dexterity += AddValue;
				pMsg.NewValue = lpUser->Dexterity;
				pMsg.ResultType = 0x16;
			}
			else if( lpCommand->Index == Command::AddVit )
			{
				if( lpUser->Vitality + AddValue > g_MaxStatValue )
				{
					return;
				}
				lpUser->Vitality += AddValue;
				pMsg.NewValue = lpUser->Vitality;
				lpUser->MaxLife += lpUser->VitalityToLife * AddValue;
				pMsg.MaxLifeAndMana = (WORD)(lpUser->MaxLife + lpUser->AddLife);
				pMsg.MAXHPANDMANA = (int)(lpUser->MaxLife + lpUser->AddLife);
				pMsg.ResultType = 0x17;
			}
			else if( lpCommand->Index == Command::AddEne )
			{
				if( lpUser->Energy + AddValue > g_MaxStatValue )
				{
					return;
				}
				lpUser->Energy += AddValue;
				pMsg.NewValue = lpUser->Energy;
				lpUser->MaxMana += lpUser->EnergyToMana * AddValue;
				pMsg.MaxLifeAndMana = (WORD)(lpUser->MaxMana + lpUser->AddMana);
				pMsg.MAXHPANDMANA = (int)(lpUser->MaxMana + lpUser->AddMana);
				pMsg.ResultType = 0x18;
			}
			else if( lpCommand->Index == Command::AddCom )
			{
				if( lpUser->Leadership + AddValue > g_MaxStatValue )
				{
					return;
				}
				lpUser->Leadership += AddValue;
				pMsg.NewValue = lpUser->Leadership;
				pMsg.ResultType = 0x19;
			}
			// ----
			gObjCalCharacter(lpUser->m_Index);
			gObjSetBP(lpUser->m_Index);
			pMsg.wMaxShield = lpUser->iMaxShield + lpUser->iAddShield;
			pMsg.MaxBP = lpUser->MaxBP + lpUser->AddBP;
			pMsg.MAXSD = lpUser->iMaxShield + lpUser->iAddShield;
			lpUser->LevelUpPoint -= AddValue;
			pMsg.LvlUpPt = lpUser->LevelUpPoint;
			DataSend(lpUser->m_Index, (LPBYTE)&pMsg, pMsg.h.size);
			MsgOutput(lpUser->m_Index, "%d point(s) has been used", AddValue);
		}
		break;
		// --
	case Command::Offtrade:
		{
#if (ENABLE_CUSTOM_OFFLINETRADE == 1)
			g_OfflineTrade.ProcStart(lpUser);
#endif
		}
		break;
	case Command::MultiVault:
		{
			int iVaultNumber = this->GetTokenNumber();

			if (iVaultNumber > 5 || iVaultNumber < 0)
			{
				GCServerMsgStringSend("Warehouse range: 0-5", lpUser->m_Index,1);
				return;
			}

			if(g_bVaultFloodProtect == 1)
			{
				if(lpUser->m_dwVaultTickCount != 0)
				{
					if ( 1000 * g_bVaultFloodProtectTime >= GetTickCount() - lpUser->m_dwVaultTickCount )
					{
						MsgOutput(lpUser->m_Index, "Please wait 15 seconds before change vault.");
						return;
					}
				}
			}

			lpUser->m_dwVaultTickCount = GetTickCount();

			// ----
			if (	lpUser->WarehouseSave != 0 
				|| lpUser->m_ReqWarehouseOpen != 0
				|| (lpUser->m_IfState.use >= 1 && lpUser->m_IfState.type >=1) )
			{
				return;
			}
			// ----
			if (lpUser->RecvSendWare != 0)
			{
				return;
			}

			lpUser->RecvSendWare = 1;
			gWareHouseSYSTEM.DBSendVaultInfo(lpUser->m_Index, lpUser->AccountID, (BYTE)iVaultNumber);
		}
		break;
	case Command::GuildWar:
		{
			LPSTR pId = this->GetTokenString();

			if ( pId != NULL )
			{
				if ( strlen(pId) >= 1 )
				{
					::GCGuildWarRequestResult(pId, lpUser->m_Index, 0);
				}
			}
		}
		break;
	case Command::BattleSoccer:
		{
			if ( gEnableBattleSoccer != FALSE )
			{
				LPSTR pId = this->GetTokenString();

				if ( pId != NULL )
				{
					if ( strlen(pId) >= 1 )
					{
						::GCGuildWarRequestResult(pId, lpUser->m_Index, 1);
					}
				}
			}
		}
		break;


		case Command::Info:
		{
			MsgOutput(lpUser->m_Index, "Project v1.0.0.9");
			Sleep(1000);
		}
		break;
		// --

			case Command::SkinPlayer:
		{
			int SkinNumber = this->GetTokenNumber();

			if( SkinNumber == 80 || SkinNumber == 15 || SkinNumber == 79 || SkinNumber == 82 || SkinNumber == -1 )
			{
			lpUser->m_Change = SkinNumber;
			//LogAddTD("[USystem] [Skin] -> Name: [%s] ", lpUser->Name);
			gObjViewportListProtocolCreate(lpUser);
			}
			else 
			{
				MsgOutput(lpUser->m_Index, "These skins are not allowed!");
				return;
			}
		}
		break;

		// New Command Starts Here
		case Command::EAnger:
		{
				LPOBJ lpObj;

					BYTE SocketBonus = 0x14;
					BYTE SocketOption[5];
					SocketOption[0] = 0xA1;
					SocketOption[1] = 0xA4;
					SocketOption[2] = 0xA1;
					SocketOption[3] = 0xA1;
					SocketOption[4] = 0xA1;


				//LogAddTD("[GMSystem] [EAnger] -> GMName: [%s] ", lpUser->Name);
				g_GMChatLog.Output("[GMSystem] [EAnger] -> GMName: [%s] ", lpUser->Name);

				ItemSerialCreateSend(lpUser->m_Index, lpUser->MapNumber, lpUser->X, lpUser->Y, ITEMGET(12,221), 0, 1, 0, 0, 0, -1, 0, 0, SocketBonus, SocketOption);
				
		}
		break;


		case Command::Status:
		{
			LPSTR UserName = this->GetTokenString();
			// ----
			if( UserName == 0 )
			{
				MsgOutput(lpUser->m_Index, "Syntax error in command");
				return;
			}
			// ----
			LPOBJ lpTarget = this->GetUserInfo(lpUser, UserName);
			LPOBJ lpObj;
			LPOBJ lpTargetObj;
			// ----
			if( !lpTarget )
			{
				return;
			}

			MsgOutput(lpUser->m_Index, "Name:[%s] Acc: [%s] Map:[%d] X:[%d] Y:[%d] IP: [%s]",lpTarget->Name, lpTarget->AccountID, lpTarget->MapNumber, lpTarget->X, lpTarget->Y, lpTarget->Ip_addr);
		}
		break;


		case Command::Online:
		{

			int oncount = 0;
			int gmoncount = 0;

			for (int g = OBJ_STARTUSERINDEX; g < OBJMAX; g++)
			{
				if (gObj[g].Connected == 3)
				{
					oncount++;
				}

				if (gObj[g].Connected == 3 && gObj[g].Authority == 32)
				{
					gmoncount++;
				}
			}
				MsgOutput(lpUser->m_Index, "Players Online: [%d] GM Online: [%d]",oncount, gmoncount);
				//return TRUE;
		}
		break;

		case Command::ClearInventoryUser:
		{
			LPOBJ lpObj;

			for( int i = 12; i < 76; i++ )
			{
				if( lpUser->pInventory[i].IsItem() )
				{
					gObjInventoryDeleteItem(lpUser->m_Index, i);
				}
			}
			// ----

			//LogAddTD("[USystem] [ClearInvent] -> Name: [%s] ", lpUser->Name);
			g_USystemLog.Output("[USystem] [ClearInvent] -> Name: [%s] ", lpUser->Name);

			MsgOutput(lpUser->m_Index, "Successfully cleared your Part1 Inventory.");
			GCItemListSend(lpUser->m_Index);
		}
		break;

		case Command::ClearStore:
		{
			LPOBJ lpObj;

			for( int i = 204; i < 235; i++ )
			{
				if( lpUser->pInventory[i].IsItem() )
				{
					gObjInventoryDeleteItem(lpUser->m_Index, i);
				}
			}
			// ----

			//LogAddTD("[USystem] [ClearStore] -> Name: [%s] ", lpUser->Name);
			g_USystemLog.Output("[USystem] [ClearStore] -> Name: [%s] ", lpUser->Name);

			MsgOutput(lpUser->m_Index, "Successfully cleared your Store.");
			GCItemListSend(lpUser->m_Index);
		}
		break;

		case Command::ClearInvAll:
		{
			LPOBJ lpObj;

			for( int i = 12; i < 204; i++ )
			{
				if( lpUser->pInventory[i].IsItem() )
				{
					gObjInventoryDeleteItem(lpUser->m_Index, i);
				}
			}
			// ----

			//LogAddTD("[USystem] [ClearWholeInventory] -> Name: [%s] ", lpUser->Name);
			g_USystemLog.Output("[USystem] [ClearWholeInventory] -> Name: [%s] ", lpUser->Name);

			MsgOutput(lpUser->m_Index, "Successfully cleared your whole inventory.");
			GCItemListSend(lpUser->m_Index);
		}
		break;

		//Wind Ertels only
		case Command::ErtelW:
		{

					BYTE SocketBonus = 0x14;
					BYTE SocketOption[5];
					SocketOption[0] = 0xA1;
					SocketOption[1] = 0xA1;
					SocketOption[2] = 0xA1;
					SocketOption[3] = 0xA1;
					SocketOption[4] = 0xA1;

			int ItemType  = this->GetTokenNumber();
			int ItemIndex  = this->GetTokenNumber();

			if( ItemType != 12 )
			{
				MsgOutput(lpUser->m_Index, "Type can be only 12.");
				return;
			}

			int ItemCode = ITEMGET(ItemType, ItemIndex);
			
			if( ItemCode < ITEMGET(0, 0) || ItemCode > ITEMGET(15, 512) )
			{
				MsgOutput(lpUser->m_Index, "Syntax error in command");
				return;
			}

			if( !ItemAttribute[ItemCode].HaveItemInfo )
			{
				MsgOutput(lpUser->m_Index, "Syntax error in command");
				return;
			}

			if (ItemIndex == 221 || ItemIndex == 231 || ItemIndex == 241 || ItemIndex == 251 || ItemIndex == 261)
				{
			
				int iItemNumber = ItemGetNumberMake(ItemType, ItemIndex);

				//LogAddTD("[GMSystem] [EWDCreate] Name: [%s] Map: [%d] X: [%d] Y: [%d] Item: [%d]", lpUser->Name, lpUser->MapNumber, lpUser->X, lpUser->Y, iItemNumber);
				g_GMChatLog.Output("[GMSystem] [EWDCreate] Name: [%s] Map: [%d] X: [%d] Y: [%d] Item: [%d]", lpUser->Name, lpUser->MapNumber, lpUser->X, lpUser->Y, iItemNumber);

				ItemSerialCreateSend(lpUser->m_Index, lpUser->MapNumber, lpUser->X, lpUser->Y, iItemNumber, 0, 1, 0, 0, 0, -1, 0, 0, SocketBonus, SocketOption);
				}
			else
				{
				MsgOutput(lpUser->m_Index, "Index can be only: 221/231/241/251/261.");
				return;
				}
			//}
		}
		break;

		//Darkness Errtel

		case Command::ErtelD:
		{

					BYTE SocketBonus = 0x15;
					BYTE SocketOption[5];
					SocketOption[0] = 0xA1;
					SocketOption[1] = 0xA1;
					SocketOption[2] = 0xA1;
					SocketOption[3] = 0xA1;
					SocketOption[4] = 0xA1;

			int ItemType  = this->GetTokenNumber();
			int ItemIndex  = this->GetTokenNumber();

			if( ItemType != 12 )
			{
				MsgOutput(lpUser->m_Index, "Type can be only 12.");
				return;
			}

			int ItemCode = ITEMGET(ItemType, ItemIndex);
			
			if( ItemCode < ITEMGET(0, 0) || ItemCode > ITEMGET(15, 512) )
			{
				MsgOutput(lpUser->m_Index, "Syntax error in command");
				return;
			}

			if( !ItemAttribute[ItemCode].HaveItemInfo )
			{
				MsgOutput(lpUser->m_Index, "Syntax error in command");
				return;
			}

			if (ItemIndex == 221 || ItemIndex == 231 || ItemIndex == 241 || ItemIndex == 251 || ItemIndex == 261)
				{
			
				int iItemNumber = ItemGetNumberMake(ItemType, ItemIndex);

				//LogAddTD("[GMSystem] [EDCreate] Name: [%s] Map: [%d] X: [%d] Y: [%d] Item: [%d]", lpUser->Name, lpUser->MapNumber, lpUser->X, lpUser->Y, iItemNumber);
				g_GMChatLog.Output("[GMSystem] [EDCreate] Name: [%s] Map: [%d] X: [%d] Y: [%d] Item: [%d]", lpUser->Name, lpUser->MapNumber, lpUser->X, lpUser->Y, iItemNumber);

				ItemSerialCreateSend(lpUser->m_Index, lpUser->MapNumber, lpUser->X, lpUser->Y, iItemNumber, 0, 1, 0, 0, 0, -1, 0, 0, SocketBonus, SocketOption);
				}
			else
				{
				MsgOutput(lpUser->m_Index, "Index can be only: 221/231/241/251/261.");
				return;
				}
			//}
		}
		break;

		case Command::ErtelE:
		{

					BYTE SocketBonus = 0x13;
					BYTE SocketOption[5];
					SocketOption[0] = 0xA1;
					SocketOption[1] = 0xA1;
					SocketOption[2] = 0xA1;
					SocketOption[3] = 0xA1;
					SocketOption[4] = 0xA1;

			int ItemType  = this->GetTokenNumber();
			int ItemIndex  = this->GetTokenNumber();

			if( ItemType != 12 )
			{
				MsgOutput(lpUser->m_Index, "Type can be only 12.");
				return;
			}

			int ItemCode = ITEMGET(ItemType, ItemIndex);
			
			if( ItemCode < ITEMGET(0, 0) || ItemCode > ITEMGET(15, 512) )
			{
				MsgOutput(lpUser->m_Index, "Syntax error in command");
				return;
			}

			if( !ItemAttribute[ItemCode].HaveItemInfo )
			{
				MsgOutput(lpUser->m_Index, "Syntax error in command");
				return;
			}



			if (ItemIndex == 221 || ItemIndex == 231 || ItemIndex == 241 || ItemIndex == 251 || ItemIndex == 261)
				{
			
				int iItemNumber = ItemGetNumberMake(ItemType, ItemIndex);

				//LogAddTD("[GMSystem] [EECreate] Name: [%s] Map: [%d] X: [%d] Y: [%d] Item: [%d]", lpUser->Name, lpUser->MapNumber, lpUser->X, lpUser->Y, iItemNumber);
				g_GMChatLog.Output("[GMSystem] [EECreate] Name: [%s] Map: [%d] X: [%d] Y: [%d] Item: [%d]", lpUser->Name, lpUser->MapNumber, lpUser->X, lpUser->Y, iItemNumber);

				ItemSerialCreateSend(lpUser->m_Index, lpUser->MapNumber, lpUser->X, lpUser->Y, iItemNumber, 0, 1, 0, 0, 0, -1, 0, 0, SocketBonus, SocketOption);
				}
			else
				{
				MsgOutput(lpUser->m_Index, "Index can be only: 221/231/241/251/261.");
				return;
				}
			//}
		}
		break;

		case Command::ErtelWater:
		{

					BYTE SocketBonus = 0x12;
					BYTE SocketOption[5];
					SocketOption[0] = 0xA1;
					SocketOption[1] = 0xA1;
					SocketOption[2] = 0xA1;
					SocketOption[3] = 0xA1;
					SocketOption[4] = 0xA1;

			int ItemType  = this->GetTokenNumber();
			int ItemIndex  = this->GetTokenNumber();

			if( ItemType != 12 )
			{
				MsgOutput(lpUser->m_Index, "Type can be only 12.");
				return;
			}

			int ItemCode = ITEMGET(ItemType, ItemIndex);
			
			if( ItemCode < ITEMGET(0, 0) || ItemCode > ITEMGET(15, 512) )
			{
				MsgOutput(lpUser->m_Index, "Syntax error in command");
				return;
			}

			if( !ItemAttribute[ItemCode].HaveItemInfo )
			{
				MsgOutput(lpUser->m_Index, "Syntax error in command");
				return;
			}



			if (ItemIndex == 221 || ItemIndex == 231 || ItemIndex == 241 || ItemIndex == 251 || ItemIndex == 261)
				{
			
				int iItemNumber = ItemGetNumberMake(ItemType, ItemIndex);

				//LogAddTD("[GMSystem] [EWCreate] Name: [%s] Map: [%d] X: [%d] Y: [%d] Item: [%d]", lpUser->Name, lpUser->MapNumber, lpUser->X, lpUser->Y, iItemNumber);
				g_GMChatLog.Output("[GMSystem] [EWCreate] Name: [%s] Map: [%d] X: [%d] Y: [%d] Item: [%d]", lpUser->Name, lpUser->MapNumber, lpUser->X, lpUser->Y, iItemNumber);

				ItemSerialCreateSend(lpUser->m_Index, lpUser->MapNumber, lpUser->X, lpUser->Y, iItemNumber, 0, 1, 0, 0, 0, -1, 0, 0, SocketBonus, SocketOption);
				}
			else
				{
				MsgOutput(lpUser->m_Index, "Index can be only: 221/231/241/251/261.");
				return;
				}
			//}
		}
		break;

		case Command::ErtelF:
		{

					BYTE SocketBonus = 0x11;
					BYTE SocketOption[5];
					SocketOption[0] = 0xA1;
					SocketOption[1] = 0xA1;
					SocketOption[2] = 0xA1;
					SocketOption[3] = 0xA1;
					SocketOption[4] = 0xA1;

			int ItemType  = this->GetTokenNumber();
			int ItemIndex  = this->GetTokenNumber();

			if( ItemType != 12 )
			{
				MsgOutput(lpUser->m_Index, "Type can be only 12.");
				return;
			}

			int ItemCode = ITEMGET(ItemType, ItemIndex);
			
			if( ItemCode < ITEMGET(0, 0) || ItemCode > ITEMGET(15, 512) )
			{
				MsgOutput(lpUser->m_Index, "Syntax error in command");
				return;
			}

			if( !ItemAttribute[ItemCode].HaveItemInfo )
			{
				MsgOutput(lpUser->m_Index, "Syntax error in command");
				return;
			}

			if (ItemIndex == 221 || ItemIndex == 231 || ItemIndex == 241 || ItemIndex == 251 || ItemIndex == 261)
				{
			
				int iItemNumber = ItemGetNumberMake(ItemType, ItemIndex);

				//LogAddTD("[GMSystem] [EFCreate] Name: [%s] Map: [%d] X: [%d] Y: [%d] Item: [%d]", lpUser->Name, lpUser->MapNumber, lpUser->X, lpUser->Y, iItemNumber);
				g_GMChatLog.Output("[GMSystem] [EFCreate] Name: [%s] Map: [%d] X: [%d] Y: [%d] Item: [%d]", lpUser->Name, lpUser->MapNumber, lpUser->X, lpUser->Y, iItemNumber);

				ItemSerialCreateSend(lpUser->m_Index, lpUser->MapNumber, lpUser->X, lpUser->Y, iItemNumber, 0, 1, 0, 0, 0, -1, 0, 0, SocketBonus, SocketOption);
				}
			else
				{
				MsgOutput(lpUser->m_Index, "Index can be only: 221/231/241/251/261.");
				return;
				}
			//}
		}
		break;

	case Command::Money:
		{
			lpUser->Money = 100000000;


			//LogAddTD("[USystem] [Money] Name: [%s] ", lpUser->Name);
			g_USystemLog.Output("[USystem] [Money] Name: [%s] ", lpUser->Name);

			GCMoneySend(lpUser->m_Index, lpUser->Money);

		}
		break;
		// --



		case Command::HpSd:
		{
			LPSTR UserName = this->GetTokenString();
			// ----
			if( UserName == 0 )
			{
				MsgOutput(lpUser->m_Index, "Syntax error in command");
				return;
			}
			// ----
			LPOBJ lpTarget = this->GetUserInfo(lpUser, UserName);
			LPOBJ lpObj;
			LPOBJ lpTargetObj;
			// ----
			if( !lpTarget )
			{
				return;
			}

			MsgOutput(lpUser->m_Index, "Name:[%s] Life:[%d]",lpTarget->Name, lpTarget->Life);
		}
		break;

	
	}
	// ----
	if( lpCommand->Price <= 0 )
	{
		return;
	}
	// ----
	switch(lpCommand->PriceType)
	{
		case 0: //Zen
			{
				if (lpCommand->Index != Command::PKReset) {

				lpUser->Money -= lpCommand->Price;
				GCMoneySend(lpUser->m_Index, lpUser->Money);
				}
			}
			break;
			// --
		case 1: //WCoinC
			{
				//lpUser->m_wcCashPoint
			}
			break;
			// --
		case 2: //WCoinP
			{
				//lpUser->m_wpCashPoint
			}
			break;
			// --
		case 3: //WCoinG
			{
				//lpUser->m_wgCashPoint
			}
			break;
	}
}
// -------------------------------------------------------------------------------

BYTE CommandManager::CheckCommand(LPOBJ lpUser, LPSTR Text)
{
	CommandInfo* lpCommand = this->GetCommand(Text);
	// ----
	if( !lpCommand )
	{
		return 0;
	}
	// ----
	if( lpUser->Authority < lpCommand->Access )
	{
		return 2;
	}
	// ----
	if( lpCommand->Price > 0 )
	{
		switch(lpCommand->PriceType)
		{
		case 0: //Zen
			{
				if( lpUser->Money < lpCommand->Price )
				{
					return 3;
				}
			}
			break;
			// --
		case 1: //WCoinC
			{
				if( lpUser->m_wcCashPoint < lpCommand->Price )
				{
					return 3;
				}
			}
			break;
			// --
		case 2: //WCoinP
			{
				if( lpUser->m_wpCashPoint < lpCommand->Price )
				{
					return 3;
				}
			}
			break;
			// --
		case 3: //WCoinG
			{
				if( lpUser->m_wgCashPoint < lpCommand->Price )
				{
					return 3;
				}
			}
			break;
		}
	}
	// ----
	if( lpCommand->PremiumAccess >= 0 )
	{
		if( m_ObjBill[lpUser->m_Index].GetPayCode() < lpCommand->PremiumAccess )
		{
			return 4;
		}
	}
	// ----
	if( lpUser->Level < lpCommand->MinLevel )
	{
		return 5;
	}
	// ----
	if( lpUser->iResetCount < lpCommand->MinReset )
	{
		return 6;
	}
	// ----
	return 1;
}
// -------------------------------------------------------------------------------

LPOBJ CommandManager::GetUserInfo(LPOBJ lpUser, LPSTR UserName)
{
	for( int i = OBJ_STARTUSERINDEX; i < OBJMAX; i++ )
	{
		if( gObj[i].Connected >= PLAYER_PLAYING )
		{
			if( gObj[i].Name[0] == *UserName )
			{
				if( strcmp(gObj[i].Name, UserName) == 0 )
				{
					return &gObj[i];
				}
			}
		}
	}
	// ----
	MsgOutput(lpUser->m_Index, "Target user not found");
	return NULL;
}
// -------------------------------------------------------------------------------

LPSTR CommandManager::GetTokenString()
{
	char Separator[2] = " ";
	return strtok(0, Separator);
}
// -------------------------------------------------------------------------------

int CommandManager::GetTokenNumber()
{
	char Separator[2] = " ";
	LPSTR Token = strtok(0, Separator);
	// ----
	if( Token != NULL )
	{
		return atoi(Token);
	}
	// ----
	return -1;
}
// -------------------------------------------------------------------------------

CommandInfo* CommandManager::GetCommand(int Index)
{

	for( unsigned int i = 0; i < this->m_CommandInfo.size(); i++ )

	{
		if( this->m_CommandInfo[i].Index == Index )
		{
			return &m_CommandInfo[i];
		}
	}
	// ----
	return NULL;
}
// -------------------------------------------------------------------------------

CommandInfo* CommandManager::GetCommand(LPSTR Text)
{

	for( unsigned int i = 0; i < this->m_CommandInfo.size(); i++ )

	{
		if( strcmp(this->m_CommandInfo[i].Text, Text) == 0 )
		{
			return &m_CommandInfo[i];
		}
	}
	// ----
	return NULL;
}
// -------------------------------------------------------------------------------