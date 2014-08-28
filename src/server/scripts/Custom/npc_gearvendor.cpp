//hcl4 gearvendor

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "GameEventMgr.h"
#include "Player.h"
#include "WorldSession.h"
#include "ItemEnchantmentMgr.h"


//Menu
#define MENU_ITEMS "items"
#define MENU_CLOSE "close"

//items
#define ITEM_1 12006
#define ITEM_2 51994
#define ITEM_3 6586
#define ITEM_4 6587
#define ITEM_5 15331
#define ITEM_6 36487
#define ITEM_7 11967




class npc_gearvendor : public CreatureScript
{
public:
    npc_gearvendor() : CreatureScript("npc_gearvendor") { }

    void GenerateSuffixList(Player* player, const ItemTemplate* itemp)
    {      
        
        if (itemp->RandomProperty > 0)
        {            
            std::vector<ItemRandomPropertiesEntry const*> enchantModList = GetRandomPropertiesList(itemp->RandomProperty);
            std::sort(enchantModList.begin(), enchantModList.end(),
                [](ItemRandomPropertiesEntry const* &lhs, ItemRandomPropertiesEntry const* &rhs)
            { 
                std::string name1 = lhs->nameSuffix[0];
                std::string name2 = rhs->nameSuffix[0];
                std::string search = "the ";
                size_t index = name1.find(search);
                if (index != std::string::npos)
                    name1.erase(index, search.length());                

                index = name2.find(search);
                if (index != std::string::npos)
                    name2.erase(index, search.length());                

                return (name1 < name2) || ((name1 == name2) && (lhs->enchant_id[0] < rhs->enchant_id[0])) || ((name1 == name2) && (lhs->enchant_id[0] == lhs->enchant_id[0]) && (lhs->enchant_id[1] < rhs->enchant_id[1])); 
            });
            std::vector<ItemRandomPropertiesEntry const*>::const_iterator itr;
            std::vector<ItemRandomPropertiesEntry const*>::const_iterator itr2;
            
            for (itr = enchantModList.begin(), itr2 = enchantModList.begin() + 1; itr != enchantModList.end(); ++itr)
            {
                if (itr2 == enchantModList.end())
                {
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, (*itr)->nameSuffix[0], itemp->ItemId, (*itr)->ID);
                    break;
                }
                if (strcmp((*itr)->nameSuffix[0], (*itr2)->nameSuffix[0]) != 0)
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, (*itr)->nameSuffix[0], itemp->ItemId, (*itr)->ID);
                itr2++;                
            }
        }
        else
        {
            std::vector<ItemRandomSuffixEntry const*> enchantModList = GetRandomSuffixList(itemp->RandomSuffix);
            std::sort(enchantModList.begin(), enchantModList.end(),
                [](ItemRandomSuffixEntry const* &lhs, ItemRandomSuffixEntry const* &rhs)
            {
                std::string name1 = lhs->nameSuffix[0];
                std::string name2 = rhs->nameSuffix[0];
                std::string search = "the ";
                size_t index = name1.find(search);
                if (index != std::string::npos)
                    name1.erase(index, search.length());

                index = name2.find(search);
                if (index != std::string::npos)
                    name2.erase(index, search.length());

                return name1 < name2;
            });
            for (std::vector<ItemRandomSuffixEntry const*>::const_iterator itr = enchantModList.begin(); itr != enchantModList.end(); ++itr)
            {                             
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, (*itr)->nameSuffix[0], itemp->ItemId, (*itr)->ID);
            }
        }
        
    }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, MENU_ITEMS, 0, 1);
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, MENU_CLOSE, 0, 2);
        player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());

        return true;
    };

    bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action) override
    {
        player->PlayerTalkClass->ClearMenus();

        if (sender == 0 && action > 0)
        {

            switch (action)
            {
            case 1:
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "meadow ring", ITEM_1, 0);
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "tumultuous cloak", ITEM_2, 0);
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "scouting g", ITEM_3, 0);
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "scouting t", ITEM_4, 0);
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "wrang wrist", ITEM_5, 0);
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "thing", ITEM_6, 0);
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "ringthing", ITEM_7, 0);
                player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());
                break;
           
            default:
                break;
            }
        }
        
        else if (sender > 0 && action == 0)
        {
            ItemTemplate const* itemp = sObjectMgr->GetItemTemplate(sender);
            GenerateSuffixList(player, itemp);
            player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());            
        }

        else
        {
            ItemTemplate const* itemp = sObjectMgr->GetItemTemplate(sender);
            int32 neg = 1;
            if (itemp->RandomSuffix)
                neg = -1;
            ItemPosCountVec dest;
            uint8 msg = player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, sender, 1);
            if (msg == EQUIP_ERR_OK)
            {
                Item* item = player->StoreNewItem(dest, sender, true, action*neg);
                player->SendNewItem(item, 1, true, false);
            }
            player->CLOSE_GOSSIP_MENU();
        }

        return true;
    };

};

void AddSC_npc_gearvendor()
{
    new npc_gearvendor();
}
