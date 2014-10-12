#include "Koth.h"
#include "Player.h"


Koth::Koth()
{
    m_QueuedPlayers.clear();
    m_RQueuedPlayers.clear();

    m_fighterGUIDs.resize(KOTH_FIGHTER_MAX);
    KothCreatures.resize(KOTH_NPC_MAX);
    Reset();
    for (uint8 i = 0; i < KOTH_NPC_MAX; i++)
    {
        AddCreature(KOTH_CreatureInfo[i], i, TeleportPositions[i + 5].GetPositionX(), TeleportPositions[i + 5].GetPositionY(), TeleportPositions[i + 5].GetPositionZ(), TeleportPositions[i + 5].GetOrientation());
    }
}

Koth::~Koth()
{

}

void Koth::Reset()
{
    m_fighterGUIDs.clear();
    m_fighterGUIDs.resize(KOTH_FIGHTER_MAX);
    m_maxFighters = 2;
    m_WaitingCount = 0;
    m_fighterCount = 0;
    m_oldwinnerGUID = ObjectGuid::Empty;
    m_streak = 0;
    SetState(KOTH_STATE_WAITING);
}


//MESSAGE
void Koth::SendMessageToQueue(std::string text)
{
	WorldPacket data;
	ChatHandler::BuildChatPacket(data, CHAT_MSG_SYSTEM, LANG_UNIVERSAL, NULL, NULL, text);
	Player* player = nullptr;
	for (KothQueuedPlayersMap::iterator itr = m_QueuedPlayers.begin(); itr != m_QueuedPlayers.end(); itr++)
	{
		player = sObjectAccessor->FindPlayer(itr->first);
		if (!player) //offline
			continue;
		player->GetSession()->SendPacket(&data);
	}

    //also message the waiting fighters
    for (uint8 i = 0; i < m_maxFighters; i++)
    {
        ObjectGuid guid = GetFighterGUID(i);
        if (guid == 0)
            continue;
        player = sObjectAccessor->FindPlayer(guid);
        player->GetSession()->SendPacket(&data);
    }
}

void Koth::SendMessageToFighters(std::string text)
{
    Player* player = nullptr;
    WorldPacket data;
    ChatHandler::BuildChatPacket(data, CHAT_MSG_SYSTEM, LANG_UNIVERSAL, NULL, NULL, text);
    for (uint8 i = 0; i < m_maxFighters; i++)
    {
        player = sObjectAccessor->FindPlayer(GetFighterGUID(i));
        if (player)
            player->GetSession()->SendPacket(&data);
    }
}

void Koth::SendCreatureAnnounce(std::string text)
{
    Creature* creature = GetKOTHCreature(KOTH_NPC_ANNOUNCE);
    creature->Yell(text, LANG_UNIVERSAL);
}

void Koth::SendMessageToPlayer(Player* player, std::string text)
{
	if (!player)
		return;
	WorldPacket data;
	ChatHandler::BuildChatPacket(data, CHAT_MSG_SYSTEM, LANG_UNIVERSAL, NULL, NULL, text);
	player->GetSession()->SendPacket(&data);
}


//QUEUE
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

	std::string text = "KOTH: " + player->GetName() + " has joined the queue";
	SendMessageToQueue(text);
	text = "Queue position: " + std::to_string(m_QueuedPlayers.size());
	SendMessageToPlayer(player, text);

    return;
}

void Koth::QueueRemovePlayer(Player* player)
{
	if (!player)
		return;
    KothQueuedPlayersMap::iterator itr;
    KothRQueuedPlayersMap::iterator ritr;

    itr = m_QueuedPlayers.find(player->GetGUID());
    if (itr == m_QueuedPlayers.end())
    {
		std::string playerName = player->GetName();
        TC_LOG_ERROR("bg.battleground", "KOTH Queue: couldn't find player %s (GUID: %u)", playerName.c_str(), player->GetGUID().GetCounter());
        return;
    }

    ritr = m_RQueuedPlayers.find(itr);
    m_RQueuedPlayers.erase(ritr);
    m_QueuedPlayers.erase(itr);
	std::string text = "KOTH: You have left the queue";
	SendMessageToPlayer(player, text);
}

bool Koth::IsInQueue(ObjectGuid guid)
{
    KothQueuedPlayersMap::iterator itr = m_QueuedPlayers.find(guid);
    if (itr == m_QueuedPlayers.end())
        return false;
    return true;
}

bool Koth::QueueInvitePlayer(ObjectGuid guid, uint8 slot)
{
    KothQueuedPlayersMap::iterator itr = m_QueuedPlayers.find(guid); //guaranteed to be in queue

    KothQueueInfo& kinfo = itr->second;
    Player* player = sObjectAccessor->FindPlayer(itr->first);
    if (!kinfo.IsInvited())
    {
        kinfo.InvitedSlot = slot + 1; //slot offset by 1 for check
        kinfo.RemoveInviteTime = getMSTime() + KOTH_TIME_INVITE_COUNTDOWN;

        player->SetInvitedForKoth(true);

        KothQueueRemoveEvent* removeEvent = new KothQueueRemoveEvent(player->GetGUID(), kinfo.RemoveInviteTime);
        m_events.AddEvent(removeEvent, m_events.CalculateTime(KOTH_TIME_INVITE_COUNTDOWN));

		std::string text = "KOTH: You have been invited to King of the Hill. You have 30 seconds to respond to the invitation.";

		SendMessageToPlayer(player, text);
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
        SetFighterGUID(ObjectGuid::Empty, kinfo.InvitedSlot - 1);
    }

    player->SetInvitedForKoth(false);
    kinfo.InvitedSlot = 0;
    if (IsInQueue(player->GetGUID()))
        QueueRemovePlayer(player);
    return;
}

void Koth::CancelInvite(Player* player)
{
    if (GetKothState() > KOTH_STATE_WAITING)
        return;
    if (player->GetKothFighterSlot() == 0)
        return;

    DecreaseWaitingCount();
    DecreaseFighterCount();
    SetFighterGUID(ObjectGuid::Empty, player->GetKothFighterSlot() - 1);
    SendMessageToPlayer(player, "KOTH: invite cancelled");
    player->SetInvitedForKoth(false);
    player->RemoveKothFighterSlot();    
    TeleportFighter(player, 4);
    return;
}

void Koth::KothQueueUpdate(uint32 diff)
{
    m_events.Update(diff);
	KothRQueuedPlayersMap::iterator itr;
    switch (KothState)
    {
    case KOTH_STATE_WAITING: //Queue state
        //First check if we have enough fighters
        if (m_fighterCount == m_maxFighters)
        {
            bool ArenaReady = PrepareArena();
            if (ArenaReady)
            {
                SetState(KOTH_STATE_PREMATCH);
                Creature* settings = GetKOTHCreature(KOTH_NPC_SETTINGS);
                settings->AddAura(10032, settings);
                KothArenaCountdown* countdownEvent = new KothArenaCountdown(KOTH_TIME_COUNTDOWN);
                m_events.AddEvent(countdownEvent, m_events.CalculateTime(KOTH_TIME_COUNTDOWN));
                SendCreatureAnnounce("We have found our fighters! The match will begin in " + std::to_string(KOTH_TIME_COUNTDOWN / 1000) + " seconds.");
                break;
            }
        }

        //Check if queue is populated, then process queue
        if (m_QueuedPlayers.empty())
            break;

        //Check if there are slots to fill
        if (m_WaitingCount < m_maxFighters)
        {
            //Fill empty slots, giving time for people to respond to pending invitations
            for (uint8 i = KOTH_FIGHTER_KING; i < m_maxFighters; i++)
            {
                if (m_QueuedPlayers.empty())
                    break;
                if (GetFighterGUID(i)) //someone assigned to this slot already
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

	case KOTH_STATE_PREMATCH:
        //Timer set
		break;

    case KOTH_STATE_INPROGRESS:
        if (m_fighterCount == 1)
        {
            Player* winner = nullptr;
            for (uint8 i = 0; i < m_maxFighters; i++)
            {
                winner = sObjectAccessor->FindPlayer(GetFighterGUID(i));
                if (winner)
                {
                    if (winner->GetGUID() != m_oldwinnerGUID)
                    {
                        SetFighterGUID(ObjectGuid::Empty, i);
                        SetFighterGUID(winner->GetGUID(), KOTH_FIGHTER_KING);
                        m_streak = 1;
                        m_oldwinnerGUID = winner->GetGUID();
                    }
                    else
                        m_streak++;
                    winner->SetKothInProgress(false);
                    winner->SetByteFlag(UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_FLAG_SANCTUARY);
                    winner->SetKothFighterSlot(KOTH_FIGHTER_KING);

                    SendCreatureAnnounce(winner->GetName() + " is the winner! They are on a streak of " + std::to_string(m_streak));
                    SendMessageToQueue("KOTH: Next match begins in " + std::to_string(KOTH_TIME_POSTMATCH / 1000) + " seconds.");
                    SetState(KOTH_STATE_POSTMATCH);
                    Creature* settings = GetKOTHCreature(KOTH_NPC_SETTINGS);
                    settings->RemoveAura(10032);
                    KothArenaPostCountdown* postCountdownEvent = new KothArenaPostCountdown(KOTH_TIME_POSTMATCH);
                    m_events.AddEvent(postCountdownEvent, m_events.CalculateTime(KOTH_TIME_POSTMATCH));
                    break;
                }
            }
        }
        break;

    case KOTH_STATE_POSTMATCH:
        //Timer set
        break;
    }
}

void Koth::Retire(Player* player)
{
    //todo: dispense reward here    
    Player* next = nullptr;
    player->RemoveKothFighterSlot();
    player->RemoveByteFlag(UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_FLAG_SANCTUARY);
    TeleportFighter(player, 4);
    SetFighterGUID(ObjectGuid::Empty, KOTH_FIGHTER_KING);
    for (uint8 i = 1; i < m_maxFighters; i++)
    {
        next = sObjectAccessor->FindPlayer(GetFighterGUID(i));
        if (next)
        {
            next->SetKothFighterSlot(KOTH_FIGHTER_KING);
            m_streak = 0;
            SetFighterGUID(ObjectGuid::Empty, i);
            DecreaseFighterCount();
            DecreaseWaitingCount();
            SetFighterGUID(next->GetGUID(), KOTH_FIGHTER_KING);
            SendMessageToPlayer(next, "KOTH: The King has retired, you have been promoted as the King");
            break;
        }
        if (i == m_maxFighters - 1) //didn't find a successor
            Reset();
    }
      
}

void Koth::SetMaxFighters(Player* player, uint8 count)
{
    if (count < m_fighterCount)
    {
        SendMessageToPlayer(player, "KOTH: We have already found " + std::to_string(m_fighterCount) + " fighters, please choose a higher amount or wait until the end of this round.");
        return;
    }
    m_maxFighters = count;
    SendMessageToQueue("KOTH: The maximum number of fighters has been changed to " + std::to_string(count));
}

//ARENA
void Koth::TeleportFighter(Player* player, uint8 slot)
{
    player->TeleportTo(571, TeleportPositions[slot].GetPositionX(), TeleportPositions[slot].GetPositionY(), TeleportPositions[slot].GetPositionZ(), TeleportPositions[slot].GetOrientation());
}

void Koth::TeleportFightersStartPosition()
{
    Player* player = nullptr;
    for (uint8 i = 0; i < m_maxFighters; i++)
    {
        player = sObjectAccessor->FindPlayer(GetFighterGUID(i));
        if (!player)
            break;
        TeleportFighter(player, i);
        player->SetKothInProgress(true);
    }
}

void Koth::ArenaAddPlayer(Player* player, uint8 slot)
{
    m_fighterCount++;
    player->SetKothFighterSlot(slot);
    TeleportFighter(player, slot);
    player->SetByteFlag(UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_FLAG_SANCTUARY);
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
            SetFighterGUID(ObjectGuid::Empty, i);
            offlineCount++;
            continue;
        }
    }
    if (offlineCount == 0)
        return true;
    m_fighterCount -= offlineCount;
    return false;
}

//CREATURE
Creature* Koth::AddCreature(uint32 entry, uint32 type, float x, float y, float z, float o, TeamId /*teamId = TEAM_NEUTRAL*/, uint32 respawntime /*= 0*/)
{
    // If the assert is called, means that BgCreatures must be resized!
    ASSERT(type < KothCreatures.size());

    Map* map = sMapMgr->FindMap(571, 0);
    if (!map)
        return NULL;

    Creature* creature = new Creature();
    if (!creature->Create(sObjectMgr->GenerateLowGuid(HIGHGUID_UNIT), map, PHASEMASK_NORMAL, entry, x, y, z, o))
    {
        TC_LOG_ERROR("bg.battleground", "Koth::AddCreature: cannot create creature (entry: %u)!",
            entry);
        delete creature;
        return NULL;
    }

    creature->SetHomePosition(x, y, z, o);

    CreatureTemplate const* cinfo = sObjectMgr->GetCreatureTemplate(entry);
    if (!cinfo)
    {
        TC_LOG_ERROR("bg.battleground", "Koth::AddCreature: creature template (entry: %u) does not exist!",
            entry);
        delete creature;
        return NULL;
    }

    if (!map->AddToMap(creature))
    {
        delete creature;
        return NULL;
    }

    KothCreatures[type] = creature->GetGUID();

    if (respawntime)
        creature->SetRespawnDelay(respawntime);

    return creature;
}

Creature* Koth::GetKOTHCreature(uint32 type)
{
    Map* map = sMapMgr->FindMap(571, 0);
    Creature* creature = map->GetCreature(KothCreatures[type]);
    if (!creature)
    {
        TC_LOG_ERROR("bg.battleground", "Koth::GetBGCreature: creature (type: %u, GUID: %u) not found!",
        type, KothCreatures[type].GetCounter());
    }
    return creature;
}

bool KothQueueRemoveEvent::Execute(uint64 /*e_time*/, uint32 /*p_time*/)
{
    Player* player = sObjectAccessor->FindPlayer(m_PlayerGuid);
    if (!player) //player logged off           
        return true;
    

    if (!player->IsInvitedForKoth()) //player responded to invite queue manually
        return true;

    sKothMgr->PlayerInviteResponse(player, false);

    //TODO notify player of invitation expiration
	std::string text = "KOTH: Your invitation has expired";
	sKothMgr->SendMessageToPlayer(player, text);
    return true;

}

void KothQueueRemoveEvent::Abort(uint64 /*e_time*/)
{
    //nothing
}

bool KothArenaCountdown::Execute(uint64 /*e_time*/, uint32 /*p_time*/)
{
    sKothMgr->TeleportFightersStartPosition();
    sKothMgr->SetState(KOTH_STATE_INPROGRESS);
    sKothMgr->SendMessageToFighters("KOTH: The battle has begun!");
    return true;
}

void KothArenaCountdown::Abort(uint64 /*e_time*/)
{
    //nothing
}

bool KothArenaPostCountdown::Execute(uint64 /*e_time*/, uint32 /*p_time*/)
{
    sKothMgr->SetState(KOTH_STATE_WAITING);
    sKothMgr->SendMessageToFighters("KOTH: Searching for new challengers.");
    if (sKothMgr->GetFighterGUID(KOTH_FIGHTER_KING))
        sKothMgr->IncreaseWaitingCount();
    return true;
}

void KothArenaPostCountdown::Abort(uint64 /*e_time*/)
{
    //nothing
}
