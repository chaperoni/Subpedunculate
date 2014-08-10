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



class npc_bgvote : public CreatureScript
{
public:
    npc_bgvote() : CreatureScript("npc_bgvote") { }

   
    void CreateVoteOptionsList(Player* player)
    {
        Battleground* bg = player->GetBattleground();
        if (!bg)
            return;
        uint8 id;
        uint8 phase;
        int8 mode;        
        std::string name;
        BattlegroundVoteOptionMap bgvotemap = sBattlegroundMgr->GetVoteOptions();
        for (BattlegroundVoteOptionMap::const_iterator itr = bgvotemap.begin(); itr != bgvotemap.end(); ++itr)
        {
            phase = itr->second.phase;
            mode = itr->second.mode;
            if (phase != bg->GetVotePhase())
                continue;
            if (mode != bg->GetMode() && mode != -1)
                continue;
            id = itr->first;
            name = itr->second.name;
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, name, 100, id);
        }
    }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (Battleground* bg = player->GetBattleground())
        {

            if (bg->HasVoted(player->GetGUID()))
            {
                creature->MonsterWhisper("You have already voted on this phase.", player);
                return true;
            }         
            CreateVoteOptionsList(player);
            player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());
        }
        return true;

    };

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        player->PlayerTalkClass->ClearMenus();                  
        Battleground* bg = player->GetBattleground();

        if (!bg)
            return true;

        BattlegroundVoteOption const* bgvote = sBattlegroundMgr->GetVoteOptionById(action);
        if (!bgvote)
            return false;
        if (bgvote->phase != bg->GetVotePhase())        
            creature->MonsterWhisper("The voting period for that vote has closed. Please cast a new vote.", player);                 
        else if (!bg->HasVoted(player->GetGUID()))
            bg->CastVote(player->GetGUID(), action);
      
        player->CLOSE_GOSSIP_MENU();
        
        return true;
    };

};

void AddSC_npc_bgvote()
{
    new npc_bgvote();
}
