//hcl4 debugtool

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "GameEventMgr.h"
#include "Player.h"
#include "WorldSession.h"
#include "Koth.h"

#define T_KOTH_ACCEPT "Accept Invitation"
#define T_KOTH_DECLINE "Decline Invitation"
#define T_KOTH_LEAVE "Leave Queue"
#define T_KOTH_QUEUE "Queue"
#define T_KOTH_CANCEL_INVITE "Cancel Invite"
#define T_KOTH_MAX "Set Challenger Count"
#define T_KOTH_RETIRE "Retire"
#define T_KOTH_CANCEL "Nevermind"
#define T_KOTH_DEBUG "Debug"
#define T_KOTH_INFO "Koth Info"
#define T_KOTH_FIGHTER_INFO "Fighter info"
#define T_KOTH_QUEUE_INFO "Queue info"

class npc_koth : public CreatureScript
{
public:
    npc_koth() : CreatureScript("npc_koth") { }
    
    void PlayerInfo(Player* player, uint32 lowguid, bool queue)
    {
        Player* kothplayer = sObjectMgr->GetPlayerByLowGUID(lowguid);
            if (!kothplayer)
        {
            sKothMgr->SendMessageToPlayer(player, "Invalid player");
            return;
        }      
        sKothMgr->SendMessageToPlayer(player, "Name: " + kothplayer->GetName());
        sKothMgr->SendMessageToPlayer(player, "Invited to koth: " + std::to_string(kothplayer->IsInvitedForKoth()));
        if (queue)
        {
            sKothMgr->SendMessageToPlayer(player, "Queue time: " + std::to_string(sKothMgr->GetQueueTime(kothplayer)) + " seconds.");
            return;
        }        
        sKothMgr->SendMessageToPlayer(player, "Fighter slot: " + std::to_string(kothplayer->GetKothFighterSlot()));
        
    }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        KothStates state = sKothMgr->GetKothState();
        if (player->GetKothFighterSlot() == 1)
        {
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, T_KOTH_MAX, 101, 7);
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_VENDOR, T_KOTH_RETIRE, 0, 6);
        }
        else if (player->IsInvitedForKoth())
        {
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, T_KOTH_ACCEPT, 0, 3);
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, T_KOTH_DECLINE, 0, 4);
        }
        else if (sKothMgr->m_QueuedPlayers.find(player->GetGUID()) != sKothMgr->m_QueuedPlayers.end())
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, T_KOTH_LEAVE, 0, 2);
        else if (!player->GetKothFighterSlot())
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_TRAINER, T_KOTH_QUEUE, 0, 1);
        else if (state == KOTH_STATE_WAITING)
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, T_KOTH_CANCEL_INVITE, 0, 5);

        if (player->GetSession()->GetSecurity() >= SEC_GAMEMASTER)
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, T_KOTH_DEBUG, 101, 10);

        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, T_KOTH_CANCEL, 0,9);
        player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());
        return true;
    };

    bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action) override
    {
        player->PlayerTalkClass->ClearMenus();
        switch (action)
        {
        case 1:
            if (!player->GetKothFighterSlot())
                sKothMgr->QueueAddPlayer(player);
            break;

        case 2:
            sKothMgr->QueueRemovePlayer(player);
            break;

        case 3:
            sKothMgr->PlayerInviteResponse(player, true);
            break;

        case 4:
            sKothMgr->PlayerInviteResponse(player, false);
            break;

        case 5:
            sKothMgr->CancelInvite(player);
            break;

        case 6:
            if (sKothMgr->GetFighterCount() == 1)
                sKothMgr->Retire(player);
            break;

        case 7:
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "2", 2, 8);
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "3", 3, 8);
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "4", 4, 8);
            player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());
            break;

        case 8:
            sKothMgr->SetMaxFighters(player, sender);
            break;

        case 10:
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, T_KOTH_INFO, 0, 11);
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, T_KOTH_FIGHTER_INFO, 101, 11);
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, T_KOTH_QUEUE_INFO, 102, 11);
            player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());
            break;

        case 11:            
            sKothMgr->Debug(sender % 100, player, creature);
            break;

        case 12:
            PlayerInfo(player, sender, false);
            player->CLOSE_GOSSIP_MENU();
            break;

        case 13:
            PlayerInfo(player, sender, true);
            player->CLOSE_GOSSIP_MENU();
            break;
        }



        if (sender < 100)
            player->CLOSE_GOSSIP_MENU();
        return true;
    };

};

void AddSC_npc_koth()
{
    new npc_koth();
}
