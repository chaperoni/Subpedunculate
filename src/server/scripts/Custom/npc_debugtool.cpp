//hcl4 debugtool

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "GameEventMgr.h"
#include "Player.h"
#include "WorldSession.h"
#include "Koth.h"
#include "BattlegroundMgr.h"
#include "ItemEnchantmentMgr.h"

#define T_DEBUG_ITEM "Item"
#define T_DEBUG_KOTH "Koth"
#define T_DEBUG_KOTH_INFO "Info"
#define T_DEBUG_KOTH_QUEUE "Queue"
#define T_DEBUG_KOTH_LEAVE "leave"
#define T_DEBUG_KOTH_ACCEPT "accept"
#define T_DEBUG_KOTH_DECLINE "decline"
#define T_DEBUG_KOTH_EDIT "test"
#define T_DEBUG_KOTH_EXEC "toggle exec"
#define T_CLOSE "Close"

//item
#define T_ITEM_SUFFIXFACTOR "Suffix Factor"



class npc_debugtool : public CreatureScript
{
public:
    npc_debugtool() : CreatureScript("npc_debugtool") { }

    typedef std::map<uint64, KothQueueInfo> KothQueuedPlayersMap;
    bool OnGossipHello(Player* player, Creature* creature) override
    {        
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, T_DEBUG_KOTH, 0, 2);
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, T_CLOSE, 0, 0);
        player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());
        return true;
    };

    bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action) override
    {
        player->PlayerTalkClass->ClearMenus();
        switch (action)
        {
        case 1:
            for (uint8 i = EQUIPMENT_SLOT_START; i < EQUIPMENT_SLOT_END; i++)
            {
                Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i);
                if (!item)
                    continue;
                uint32 suffixfactor = GenerateEnchSuffixFactor(item->GetEntry());
                if (suffixfactor == 0)
                    continue;
                std::string name = sObjectMgr->GetItemTemplate(item->GetEntry())->Name1;
                std::string text = std::to_string(suffixfactor);
                name = name + " = " + text;
                creature->Say(name.c_str(), LANG_UNIVERSAL, NULL);
            }
            break;

        case 2:
            if (player->IsInvitedForKoth())
            {
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, T_DEBUG_KOTH_ACCEPT, 0, 6);
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, T_DEBUG_KOTH_DECLINE, 0, 7);
            }

            else if (sKothMgr->m_QueuedPlayers.find(player->GetGUID()) != sKothMgr->m_QueuedPlayers.end())
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, T_DEBUG_KOTH_LEAVE, 0, 5);
            else if (!player->GetKothFighterSlot())
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, T_DEBUG_KOTH_QUEUE, 0, 4);


            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, T_DEBUG_KOTH_INFO, 0, 3);
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, T_DEBUG_KOTH_EDIT, 1, 3);
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, T_DEBUG_KOTH_EXEC, 0, 8);
            player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());
            break;

        case 3:
            sKothMgr->Debug(player, sender);
            break;

        case 4:
            sKothMgr->QueueAddPlayer(player);
            break;

        case 5:
            sKothMgr->QueueRemovePlayer(player->GetGUID());
            break;

        case 6:
            sKothMgr->PlayerInviteResponse(player, true);
            break;

        case 7:
            sKothMgr->PlayerInviteResponse(player, false);
            break;

        case 8:
            sKothMgr->m_Exec = !sKothMgr->m_Exec;
            break;
        }

        if (action > 2)
            player->CLOSE_GOSSIP_MENU();
        return true;
    };

};

void AddSC_npc_debugtool()
{
    new npc_debugtool();
}
