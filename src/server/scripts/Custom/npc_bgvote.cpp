//hcl4 bgvote

#include "Chat.h"
#include "Language.h"
#include "ObjectAccessor.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "GameEventMgr.h"
#include "Player.h"
#include "WorldSession.h"
#include "BattlegroundMgr.h"
#include "boost/lexical_cast.hpp"

//Categories
#define CAT_BG "Battlegrounds"

//modes
#define BG_OPTION_1 "Capture the Flag"
#define BG_OPTION_2 "Team Deatchmatch"
#define BG_OPTION_3 "Deathmatch"


//timers
#define BG_TIMER_1 "10 Minutes"
#define BG_TIMER_2 "20 Minutes"
#define BG_TIMER_3 "30 Minutes"
#define BG_TIMER_4 "60 Minutes"

//flags

#define BG_FLAGS_1 "1 Flag"
#define BG_FLAGS_2 "3 Flags"
#define BG_FLAGS_3 "5 Flags"
#define BG_FLAGS_4 "10 Flags"



class npc_bgvote : public CreatureScript
{
public:
    npc_bgvote() : CreatureScript("npc_bgvote") { }


    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (Battleground* bg = player->GetBattleground())
        {

            if (bg->HasVoted(player->GetGUID()))
            {
                creature->MonsterWhisper("You have already voted on this phase", player);
                return true;
            }
            switch (bg->GetVotePhase())
            {
            case BG_VOTE_PHASE_1:
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, BG_OPTION_1, 100, 110);
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, BG_OPTION_2, 100, 111);
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, BG_OPTION_3, 100, 112);
                break;

            case BG_VOTE_PHASE_2:
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, BG_TIMER_1, 100, 120);
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, BG_TIMER_2, 100, 121);
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, BG_TIMER_3, 100, 122);
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, BG_TIMER_4, 100, 123);
                break;

            case BG_VOTE_PHASE_3:
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, BG_FLAGS_1, 100, 130);
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, BG_FLAGS_2, 100, 131);
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, BG_FLAGS_3, 100, 132);
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, BG_FLAGS_4, 100, 133);
                break;

            default:
                break;

            }
            player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());
        }
        return true;

    };

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        player->PlayerTalkClass->ClearMenus();
        uint8 vote = (action % 10);                
        Battleground* bg = player->GetBattleground();

        if (!bg)
            return true;

        switch (action)
        {
        case 110:
        case 111:
        case 112:
        case 120:
        case 121:
        case 122:
        case 123:
        case 130:
        case 131:
        case 132:
        case 133:
            if (!bg->HasVoted(player->GetGUID()))
                bg->CastVote(player->GetGUID(), vote);            
            break;

        default:
            break;            

        }        
        player->CLOSE_GOSSIP_MENU();
        
        return true;
    };

};

void AddSC_npc_bgvote()
{
    new npc_bgvote();
}
