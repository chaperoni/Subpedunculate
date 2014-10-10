#include "Koth.h"
#include "Player.h"


Koth::Koth()
{
    Reset(true);
}

Koth::~Koth()
{

}

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

    //also need to message the waiting fighters
    for (uint8 i = 0; i < m_maxFighters; i++)
    {
        uint64 guid = GetFighterGUID(i);
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

void Koth::SendMessageToPlayer(Player* player, std::string text)
{
	if (!player)
		return;
	WorldPacket data;
	ChatHandler::BuildChatPacket(data, CHAT_MSG_SYSTEM, LANG_UNIVERSAL, NULL, NULL, text);
	player->GetSession()->SendPacket(&data);
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
        if (m_QueuedPlayers.size() > 0)
        {
            for (uint8 i = KOTH_FIGHTER_KING; i < KOTH_FIGHTER_MAX; i++)
            {
                player = sObjectAccessor->FindPlayer(GetFighterGUID(i));
                if (!player)
                    continue;
                player->RemoveKothFighterSlot();
                player->SetInvitedForKoth(false);
            }
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
        m_oldwinnerGUID = 0;
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
        TC_LOG_ERROR("bg.battleground", "KOTH Queue: couldn't find player %s (GUID: %u)", playerName.c_str(), GUID_LOPART(player->GetGUID()));
        return;
    }

    ritr = m_RQueuedPlayers.find(itr);
    m_RQueuedPlayers.erase(ritr);
    m_QueuedPlayers.erase(itr);
	std::string text = "KOTH: You have left the queue";
	SendMessageToPlayer(player, text);
}

bool Koth::IsInQueue(uint64 guid)
{
    KothQueuedPlayersMap::iterator itr = m_QueuedPlayers.find(guid);
    if (itr == m_QueuedPlayers.end())
        return false;
    return true;
}

bool Koth::QueueInvitePlayer(uint64 guid, uint8 slot)
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

		std::string text = "KOTH: You have been invited to King of the Hill. You have 60 seconds to respond to the invitation.";
		//TODO notify player of invite
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
        SetFighterGUID(0, kinfo.InvitedSlot - 1);
        SendMessageToPlayer(player, "KOTH: invite cancelled");
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
    SetFighterGUID(0, player->GetKothFighterSlot() - 1);
    SendMessageToPlayer(player, "KOTH: invite cancelled");
    player->SetInvitedForKoth(false);
    player->RemoveKothFighterSlot();
    return;
}

void Koth::TeleportFighter(Player* player, uint8 slot)
{
    player->TeleportTo(player->GetMapId(), TeleportPositions[slot].GetPositionX(), TeleportPositions[slot].GetPositionY(), TeleportPositions[slot].GetPositionZ(), TeleportPositions[slot].GetOrientation());
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

void Koth::KothQueueUpdate(uint32 diff)
{
    m_events.Update(diff);
	KothRQueuedPlayersMap::iterator itr;
    switch (KothState)
    {
    case KOTH_STATE_WAITING: //Queue state
        //First check if we can proceed state
        if (m_fighterCount == m_maxFighters)
        {
            bool ArenaReady = PrepareArena();
            if (ArenaReady)
            {
                SetState(KOTH_STATE_PREMATCH);
                KothArenaCountdown* countdownEvent = new KothArenaCountdown(KOTH_TIME_COUNTDOWN);
                m_events.AddEvent(countdownEvent, m_events.CalculateTime(KOTH_TIME_COUNTDOWN));
                SendMessageToFighters("KOTH: Arena filled! Match will begin in " + std::to_string(KOTH_TIME_COUNTDOWN / 1000) + " seconds.");
                break;
            }
        }

        //Check if queue is populated, then process queue
        if (m_QueuedPlayers.empty() || m_Exec == false)
            break;

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

	case KOTH_STATE_PREMATCH:
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
                        SetFighterGUID(0, i);
                        SetFighterGUID(winner->GetGUID(), KOTH_FIGHTER_KING);
                        m_streak = 1;
                        m_oldwinnerGUID = winner->GetGUID();
                    }
                    else
                        m_streak++;
                    winner->SetKothInProgress(false);
                    winner->SetByteFlag(UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_FLAG_SANCTUARY);
                    winner->SetKothFighterSlot(KOTH_FIGHTER_KING);

                    SendMessageToPlayer(winner, "KOTH: You won! Current streak: " + std::to_string(m_streak));
                    SendMessageToQueue("KOTH: Next match begins in " + std::to_string(KOTH_TIME_POSTMATCH / 1000) + " seconds.");
                    SetState(KOTH_STATE_POSTMATCH);

                    KothArenaPostCountdown* postCountdownEvent = new KothArenaPostCountdown(KOTH_TIME_POSTMATCH);
                    m_events.AddEvent(postCountdownEvent, m_events.CalculateTime(KOTH_TIME_POSTMATCH));
                    break;
                }
            }
        }
        break;

    case KOTH_STATE_POSTMATCH:
        break;



    }
}

void Koth::ArenaAddPlayer(Player* player, uint8 slot)
{
    m_fighterCount++;
    player->SetKothFighterSlot(slot);
    //stuff
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
            SetFighterGUID(0, i);
            offlineCount++;
            continue;
        }
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
			info = "Queue Empty";
			SendMessageToPlayer(player, info);
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
			SendMessageToPlayer(player, info);
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
