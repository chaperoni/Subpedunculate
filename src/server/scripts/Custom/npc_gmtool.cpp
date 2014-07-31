//hcl4 gmool

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "GameEventMgr.h"
#include "Player.h"
#include "WorldSession.h"
#include "BattlegroundMgr.h"

//Categories
#define CAT_BG "Battlegrounds"

//Battlegrounds
#define BG_MODE "Mode"
#define BG_TIMER "Timer"
#define BG_FLAGS "Flags"

//timers
#define BG_TIMER_1 "20 Minutes"
#define BG_TIMER_2 "30 Minutes"
#define BG_TIMER_3 "60 Minutes"

//flags

#define BG_FLAGS_1 "1 Flag"
#define BG_FLAGS_2 "3 Flags"
#define BG_FLAGS_3 "5 Flags"
#define BG_FLAGS_4 "10 Flags"



class npc_gmtool : public CreatureScript
{
public:
	npc_gmtool() : CreatureScript("npc_gmtool") { }

	bool OnGossipHello(Player* player, Creature* creature) override
	{
		if (Battleground* bg = player->GetBattleground())
			player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, CAT_BG, GOSSIP_SENDER_MAIN, 100);

		player->TalkedToCreature(creature->GetEntry(), creature->GetGUID());
		player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());

		return true;
	};

	bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
	{
		player->PlayerTalkClass->ClearMenus();
		switch (action)
		{
		case 100:
			player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, BG_TIMER, 100, 101);
			player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, BG_FLAGS, 100, 110);
			break;

		case 101:
			player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, BG_TIMER_1, 100, 102);
			player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, BG_TIMER_2, 100, 103);
			player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, BG_TIMER_3, 100, 106);
			break;

		case 102:
		case 103:
		case 106:			
			if (Battleground* bg = player->GetBattleground())				
				bg->SetTimeLimit(uint8((action - 100) * 10));
			break;

		case 110:
			player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, BG_FLAGS_1, 100, 111);
			player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, BG_FLAGS_2, 100, 113);
			player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, BG_FLAGS_3, 100, 115);
			player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, BG_FLAGS_4, 100, 120);
			break;

		case 111:
		case 113:
		case 115:
		case 120:			
			if (Battleground* bg = player->GetBattleground())
				bg->SetMaxFlags(uint8(action - 110));
			break;		

		}
		player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());

		return true;
	};

};

void AddSC_npc_gmtool()
{
	new npc_gmtool();
}