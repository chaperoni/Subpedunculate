#include "Koth.h"
#include "Player.h"


Koth::Koth()
{
    Reset(true);
}

Koth::~Koth()
{

}

void Koth::Reset(bool init)
{
    Player* player = nullptr;
    if (init)
    {
        for (KothQueuedPlayersMap::iterator itr = m_QueuedPlayers.begin(); itr != m_QueuedPlayers.end(); itr++)
        {
            player = sObjectAccessor->FindPlayer(itr->first);
            if (!player)
                continue;
            player->RemoveKothFighterSlot();
            player->SetInvitedForKoth(false);
        }

        for (uint8 i = KOTH_FIGHTER_KING; i < KOTH_FIGHTER_MAX; i++)
        {            
            player = sObjectAccessor->FindPlayer(GetFighterGUID(i));
            if (!player)
                continue;
            player->RemoveKothFighterSlot();
            player->SetInvitedForKoth(false);
        }

        m_QueuedPlayers.clear();
        m_RQueuedPlayers.clear();
        m_fighterGUIDs.clear();

        for (uint8 i = KOTH_FIGHTER_KING; i < KOTH_FIGHTER_MAX; i++)
        {
            m_fighterGUIDs.push_back(0);            
        }
        m_maxFighters = 2;
        m_Exec = false;
        m_WaitingCount = 0;
        m_fighterCount = 0;
    }

    for (uint8 i = KOTH_FIGHTER_CHALLENGER_1; i < m_maxFighters; i++)
    {
        if (GetFighterGUID(i) == 0)
            continue;
        player = sObjectAccessor->FindPlayer(GetFighterGUID(i));
        player->RemoveKothFighterSlot();
        player->SetInvitedForKoth(false);
    }
    SetState(KOTH_STATE_WAITING);


}

void Koth::QueueAddPlayer(Player* player)
{
    if (player->GetKothFighterSlot() || player->IsInvitedForKoth())
        return;
        

    KothQueueInfo& kinfo = m_QueuedPlayers[player->GetGUID()];
    kinfo.JoinTime = getMSTime();
    kinfo.LastOnlineTime = getMSTime();
    kinfo.RemoveInviteTime = 0;
    kinfo.InvitedSlot = 0;

    KothQueuedPlayersMap::iterator kitr = m_QueuedPlayers.find(player->GetGUID());

    m_RQueuedPlayers[kitr] = player->GetGUID();

    return;
}

void Koth::QueueRemovePlayer(uint64 guid)
{
    KothQueuedPlayersMap::iterator itr;
    KothRQueuedPlayersMap::iterator ritr;

    itr = m_QueuedPlayers.find(guid);
    if (itr == m_QueuedPlayers.end())
    {
        std::string playerName = "Unknown";
        if (Player* player = ObjectAccessor::FindPlayer(guid))
            playerName = player->GetName();
        TC_LOG_ERROR("bg.battleground", "KOTH Queue: couldn't find player %s (GUID: %u)", playerName.c_str(), GUID_LOPART(guid));
        return;
    }

    ritr = m_RQueuedPlayers.find(itr);
    m_RQueuedPlayers.erase(ritr);
    m_QueuedPlayers.erase(itr);

}

bool Koth::QueueInvitePlayer(uint64 guid, uint8 slot)
{
    KothQueuedPlayersMap::iterator itr = m_QueuedPlayers.find(guid); //guaranteed to be in queue

    KothQueueInfo& kinfo = itr->second;
    Player* player = sObjectAccessor->FindPlayer(itr->first);
    if (!kinfo.IsInvited())
    {
        kinfo.InvitedSlot = slot + 1;
        kinfo.RemoveInviteTime = getMSTime() + KOTH_INVITE_ACCEPT_TIME;

        player->SetInvitedForKoth(true);

        KothQueueRemoveEvent* removeEvent = new KothQueueRemoveEvent(player->GetGUID(), kinfo.RemoveInviteTime);
        m_events.AddEvent(removeEvent, m_events.CalculateTime(KOTH_INVITE_ACCEPT_TIME));

        std::string str = "invited to slot " + std::to_string(slot);
        //TODO notify player of invite
        player->Say(str, LANG_UNIVERSAL);
        SetFighterGUID(player->GetGUID(), slot);
        IncreaseWaitingCount();
        return true;
    }
    return false;
}

void Koth::PlayerInviteResponse(Player* player, bool accept)
{
    KothQueuedPlayersMap::iterator itr = m_QueuedPlayers.find(player->GetGUID());
    if (itr == m_QueuedPlayers.end())
        return;

    KothQueueInfo& kinfo = itr->second;
    if (!kinfo.IsInvited())
        return;

    if (accept)
    {
        uint8 slot = kinfo.InvitedSlot;
        if (!slot)
            return;

        ArenaAddPlayer(player, slot - 1); //slot offset by 1 for check, put it back to normal
    }
    else
    {
        DecreaseWaitingCount();
        SetFighterGUID(0, kinfo.InvitedSlot - 1);
    }

    player->SetInvitedForKoth(false);
    kinfo.InvitedSlot = 0;
    QueueRemovePlayer(player->GetGUID());
    return;
}


void Koth::KothQueueUpdate(uint32 diff)
{
    m_events.Update(diff);
    switch (KothState)
    {
    case KOTH_STATE_WAITING:
        //First check if we can proceed state
        if (m_fighterCount == m_maxFighters)
        {
            bool ArenaReady = PrepareArena();
            if (ArenaReady)
            {
                SetState(KOTH_STATE_PREMATCH);
                KothArenaTimeExpire* expireEvent = new KothArenaTimeExpire(KOTH_ARENA_TIME_LIMIT);
                m_events.AddEvent(expireEvent, m_events.CalculateTime(KOTH_ARENA_TIME_LIMIT));
                break;
            }
        }

        //Check if queue is populated, then process queue
        if (m_QueuedPlayers.empty() || m_Exec == false)
            break;

        //Get person in queue the longest
        KothRQueuedPlayersMap::iterator itr;

        //Check if there are slots to fill
        if (m_WaitingCount < m_maxFighters)
        {
            //Fill empty slots, giving time for people to respond to pending invitations
            for (uint8 i = KOTH_FIGHTER_KING; i < m_maxFighters; i++)
            {
                if (m_QueuedPlayers.empty())
                    break;
                if (GetFighterGUID(i) > 0) //someone assigned to this slot already
                    continue;
                for (itr = m_RQueuedPlayers.begin(); itr != m_RQueuedPlayers.end(); itr++)
                {
                    if (itr->first->second.IsInvited())
                        continue;
                    QueueInvitePlayer(itr->second, i);
                    break;
                }
            }
        }
        break;
    }
}

void Koth::ArenaAddPlayer(Player* player, uint8 slot)
{
    m_fighterCount++;
    player->SetKothFighterSlot(slot + 1);
    //stuff
}

bool Koth::PrepareArena()
{
    Player* player = nullptr;
    uint8 offlineCount = 0;
    for (uint8 i = 0; i < m_fighterCount; i++)
    {
        player = sObjectAccessor->FindPlayer(m_fighterGUIDs[i]);
        DecreaseWaitingCount();
        if (!player)
        {
            SetFighterGUID(0, i);
            offlineCount++;
            continue;
        }
        player->Say("hooray", LANG_UNIVERSAL);

    }
    if (offlineCount == 0)
        return true;
    m_fighterCount -= offlineCount;
    return false;
}

void Koth::Debug(Player* player, uint8 mode)
{
    
    KothQueuedPlayersMap::iterator itr;
    KothRQueuedPlayersMap::iterator ritr;
    std::string name;
    uint32 JoinTime;
    bool IsInvited;
    std::string info;
    switch (mode)
    {
    case 0:
        if (m_QueuedPlayers.empty())
        {
            player->Say("Queue empty", LANG_UNIVERSAL);
            return;
        }
        for (itr = m_QueuedPlayers.begin(); itr != m_QueuedPlayers.end(); itr++)
        {
            name = sObjectAccessor->FindPlayer(itr->first)->GetName();
            JoinTime = itr->second.JoinTime;
            IsInvited = itr->second.IsInvited();
            std::stringstream fmt;
            fmt << "Name: " << name << " Jointime: " << JoinTime << " Invited: " << IsInvited;
            info = fmt.str();
            player->Say(info, LANG_UNIVERSAL);
        }
        break;

    case 1:
        Reset(true);
        m_maxFighters = urand(2, 4);
        player->Say(std::to_string(m_maxFighters), LANG_UNIVERSAL);
        break;
    }
}

bool KothQueueRemoveEvent::Execute(uint64 /*e_time*/, uint32 /*p_time*/)
{
    Player* player = sObjectAccessor->FindPlayer(m_PlayerGuid);
    if (!player) //player logged off
    {
        sKothMgr->DecreaseWaitingCount();
        return true;
    }


    if (!player->IsInvitedForKoth()) //player responded to invite queue manually
        return true;

    sKothMgr->PlayerInviteResponse(player, false);

    //TODO notify player of invitation expiration
    player->Say("Invite expired", LANG_UNIVERSAL);
    return true;

}

void KothQueueRemoveEvent::Abort(uint64 /*e_time*/)
{
    //nothing
}

bool KothArenaTimeExpire::Execute(uint64 /*e_time*/, uint32 /*p_time*/)
{
    if (sKothMgr->GetKothState() != KOTH_STATE_PREMATCH)
        return true;
    //simulate winner
    uint8 i = 0;    
    Player* player = nullptr;
    uint8 fightercount = sKothMgr->GetFighterCount();
    uint8 rand = urand(0, fightercount - 1);
    Player* winner = sObjectAccessor->FindPlayer(sKothMgr->GetFighterGUID(rand));    
    for (i = 0; i < sKothMgr->GetMaxFighters(); i++)
    {
        player = sObjectAccessor->FindPlayer(sKothMgr->GetFighterGUID(i));
        if (player)
            player->RemoveKothFighterSlot();
        sKothMgr->SetFighterGUID(0, i);
        sKothMgr->DecreaseFighterCount();
    }

    winner->SetKothFighterSlot(KOTH_FIGHTER_KING);
    winner->Say("woop", LANG_UNIVERSAL);
    sKothMgr->SetFighterGUID(winner->GetGUID(), KOTH_FIGHTER_KING);
    sKothMgr->IncreaseWaitingCount();
    sKothMgr->IncreaseFighterCount();

    sKothMgr->Reset(false);
    return true;
}

void KothArenaTimeExpire::Abort(uint64 /*e_time*/)
{
    //nothing
}
