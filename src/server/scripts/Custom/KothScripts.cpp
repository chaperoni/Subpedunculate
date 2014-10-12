#include "Player.h"
#include "ScriptMgr.h"
#include "Koth.h"

class KothScripts_player : public PlayerScript
{
public:
    KothScripts_player()
        : PlayerScript("KothScripts_player")
    {
    }

    void OnPVPKill(Player* killer, Player* killed) override
    {
        uint8 slot = killed->GetKothFighterSlot();
        if (slot && sKothMgr->GetKothState() == KOTH_STATE_INPROGRESS)
        {
            killed->SetKothInProgress(false);
            killed->RemoveKothFighterSlot();
            killed->RemoveByteFlag(UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_FLAG_FFA_PVP);
            sKothMgr->SetFighterGUID(ObjectGuid::Empty, slot - 1);
            sKothMgr->TeleportFighter(killed, 4);
            killed->ResurrectPlayer(1.0f);
            killed->SpawnCorpseBones();
            killed->SaveToDB();
            sKothMgr->DecreaseFighterCount();
        }
    }
};

void AddSC_KothScripts_player()
{
    new KothScripts_player();
}