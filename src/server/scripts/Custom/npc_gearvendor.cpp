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

enum GossipSteps
{
    GOSSIP_STEP_MENU = 0,
    GOSSIP_STEP_ITEM = 1,
    GOSSIP_STEP_SUFFIX = 2,
    GOSSIP_STEP_ENCHANT = 3,
    GOSSIP_STEP_END = 4
};




class npc_gearvendor : public CreatureScript
{
public:
    npc_gearvendor() : CreatureScript("npc_gearvendor") { }

    struct PlayerItemInfo
    {
        uint8 step = GOSSIP_STEP_MENU;
        uint32 itemid = 0;
        uint32 suffixid = 0;
        uint32 enchantid = 0;
        std::vector<SpellItemEnchantmentEntry const*> enchantList;
    };

    std::map<ObjectGuid, PlayerItemInfo> itemmap;



    bool InitEnchantList(uint32 id, Player* player)
    {
        PlayerItemInfo& info = itemmap[player->GetGUID()];
        info.enchantList.clear();
        ItemTemplate const* itemp = sObjectMgr->GetItemTemplate(id);
        uint32 type = itemp->InventoryType;
        uint32 level = itemp->ItemLevel;        
        std::vector<uint32> multitype;
        multitype.push_back(type);

        if (type == 17 || type == 21 || type == 22)
            multitype.push_back(13);

        if (type == 20)
            multitype.push_back(5);

        

        uint8 failed = 0;
        for (uint8 i = 0; i < multitype.size(); i++)
        {
            PreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_SEL_ENCHANTIDS);
            stmt->setUInt32(0, multitype[i]);
            PreparedQueryResult result = WorldDatabase.Query(stmt);
            if (!result)
            {
                failed++;
                continue;
            }
            do
            {
                Field* fields = result->Fetch();
                uint32 spellid = fields[0].GetUInt32();
                SpellEntry const* pSpell = sSpellStore.LookupEntry(spellid);
                if (pSpell->baseLevel > level)
                    continue;
                SpellItemEnchantmentEntry const* pEnchant = sSpellItemEnchantmentStore.LookupEntry(pSpell->EffectMiscValue[0]);
                
                if (pEnchant)
                    info.enchantList.push_back(pEnchant);
            } while (result->NextRow());
        }
        
        return failed != multitype.size();
    }

    void GenerateSuffixList(Player* player, const ItemTemplate* itemp)
    {

        if (itemp->RandomProperty > 0)
        {
            std::vector<ItemRandomPropertiesEntry const*> enchantModList = GetRandomPropertiesList(itemp->RandomProperty);
            std::sort(enchantModList.begin(), enchantModList.end(),
                [](ItemRandomPropertiesEntry const* lhs, ItemRandomPropertiesEntry const* rhs)
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
                [](ItemRandomSuffixEntry const* lhs, ItemRandomSuffixEntry const* rhs)
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

    void GenerateEnchantList(Player* player)
    {
        PlayerItemInfo& info = itemmap[player->GetGUID()];
        if (info.enchantList.empty())
            return;

        std::sort(info.enchantList.begin(), info.enchantList.end(),
            [](SpellItemEnchantmentEntry const* lhs, SpellItemEnchantmentEntry const* rhs)
        {
            std::string name1 = lhs->description[0];
            std::string name2 = rhs->description[0];
            return name1 < name2;
        });
        for (std::vector<SpellItemEnchantmentEntry const*>::const_iterator itr = info.enchantList.begin(); itr != info.enchantList.end(); ++itr)
        {
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, (*itr)->description[0], 0, (*itr)->ID);
        }
    }

    void CreateItem(Player* player)
    {
        PlayerItemInfo& info = itemmap[player->GetGUID()];
        ItemTemplate const* itemp = sObjectMgr->GetItemTemplate(info.itemid);
        int32 neg = 1;
        if (itemp->RandomSuffix)
        neg = -1;
        ItemPosCountVec dest;
        uint8 msg = player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, info.itemid, 1);
        if (msg == EQUIP_ERR_OK)
        {
            Item* item = player->StoreNewItem(dest, info.itemid, true, info.suffixid*neg);
            if (info.enchantid)
            {
                player->ApplyEnchantment(item, PERM_ENCHANTMENT_SLOT, false);
                item->SetEnchantment(PERM_ENCHANTMENT_SLOT, info.enchantid, 0, 0);
                player->ApplyEnchantment(item, PERM_ENCHANTMENT_SLOT, true);
            }
            player->SendNewItem(item, 1, true, false);
        }
    }

    void EndGossip(Player* player, Creature* creature)
    {
        player->CLOSE_GOSSIP_MENU();
        CreateItem(player);
        std::map<ObjectGuid, PlayerItemInfo>::iterator itr = itemmap.find(player->GetGUID());
        itemmap.erase(itr);
        PlayerItemInfo& info = itemmap[player->GetGUID()];
        info.step = GOSSIP_STEP_ITEM;
        player->GetSession()->SendListInventory(creature->GetGUID());
    }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        PlayerItemInfo& info = itemmap[player->GetGUID()];
        info.step = GOSSIP_STEP_MENU;
        info.itemid = 0;
        info.enchantid = 0;
        info.suffixid = 0;
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, MENU_ITEMS, 1000004, 1);
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, MENU_ITEMS, 1000005, 1);
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, MENU_CLOSE, 0, 2);
        player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());
        return true;
    };

    bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action) override
    {
        PlayerItemInfo& info = itemmap[player->GetGUID()];

        player->PlayerTalkClass->ClearMenus();
        ItemTemplate const* itemp = nullptr;
        switch (info.step)
        {
        case GOSSIP_STEP_MENU:
            if (action == 2)
                player->CLOSE_GOSSIP_MENU();
            else
            {
                player->GetSession()->SendListInventory(creature->GetGUID());
                info.step = GOSSIP_STEP_ITEM;
            }
            break;

        case GOSSIP_STEP_ITEM:
            info.itemid = sender;
            itemp = sObjectMgr->GetItemTemplate(sender);
            if (itemp->RandomProperty || itemp->RandomSuffix)
            {
                GenerateSuffixList(player, itemp);
                info.step = GOSSIP_STEP_SUFFIX;
                player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());
            }
            else if (InitEnchantList(info.itemid, player))
            {
                GenerateEnchantList(player);
                info.step = GOSSIP_STEP_ENCHANT;
                player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());
            }
            else
                EndGossip(player, creature);
            break;

        case GOSSIP_STEP_SUFFIX:
            info.suffixid = action;
            if (InitEnchantList(info.itemid, player))
            {
                GenerateEnchantList(player);
                info.step = GOSSIP_STEP_ENCHANT;
                player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());
            }
            else
                EndGossip(player, creature);
            break;

        case GOSSIP_STEP_ENCHANT:
            info.enchantid = action;
            EndGossip(player, creature);
            break;
        }

        return true;
    };

};

void AddSC_npc_gearvendor()
{
    new npc_gearvendor();
}
