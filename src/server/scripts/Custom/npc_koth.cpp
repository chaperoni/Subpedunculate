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

class npc_koth : public CreatureScript
{
public:
    npc_koth() : CreatureScript("npc_koth") { }
    
    bool OnGossipHello(Player* player, Creature* creature) override
    {
        KothStates state = sKothMgr->GetKothState();
        if (player->GetKothFighterSlot() == 1)
        {
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, T_KOTH_MAX, 0, 7);
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
        }

        if (action != 7)
            player->CLOSE_GOSSIP_MENU();
        return true;
    };

};

void AddSC_npc_koth()
{
    new npc_koth();
}
