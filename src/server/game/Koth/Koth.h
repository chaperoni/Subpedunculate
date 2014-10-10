#ifndef _KOTH_H
#define _KOTH_H


#include "Common.h"
#include "DBCEnums.h"
#include "EventProcessor.h"
#include "WorldSession.h"
#include "Chat.h"

enum KothStates
{
    KOTH_STATE_WAITING,
    KOTH_STATE_PREMATCH,
    KOTH_STATE_INPROGRESS,
    KOTH_STATE_POSTMATCH,
    KOTH_STATE_END
};

enum KothFighters
{
    KOTH_FIGHTER_KING = 0,

    KOTH_FIGHTER_CHALLENGER_BEGIN = 0,
    KOTH_FIGHTER_CHALLENGER_1 = 1,
    KOTH_FIGHTER_CHALLENGER_2 = 2,
    KOTH_FIGHTER_CHALLENGER_3 = 3,
    KOTH_FIGHTER_MAX
};

enum KothTimeIntervals
{
    KOTH_TIME_INVITE_COUNTDOWN = 10000,
    KOTH_TIME_COUNTDOWN = 15000,
    KOTH_TIME_POSTMATCH = 15000
};

const Position TeleportPositions[] = 
{
    {5729.76f, 618.68f, 571.4f, 5.58f},
    {5825.41f, 595.22f, 571.4f, 2.48f},
    {5756.02f, 651.2f, 571.4f, 5.60f},
    {5799.0f, 562.67f, 571.4f, 2.47f},
    {5788.0f, 619.0f, 610.0f, 0.98f}
    //{-9612.28f, -2160.41f, 116.7f, 5.82f},
    //{-9612.28f, -2160.41f, 116.7f, 5.82f},
    //{-9612.28f, -2160.41f, 116.7f, 5.82f},
    //{-9612.28f, -2160.41f, 116.7f, 5.82f},
    //{-9576.73f, -2178.20f, 86.3f, 0.93f}
};

struct KothQueueInfo
{
    uint32 LastOnlineTime;
    uint32 JoinTime;
    uint32 RemoveInviteTime;
    uint8 InvitedSlot;
    bool IsInvited() { return InvitedSlot > 0; }
};

class Koth
{

private:
    Koth();
    ~Koth();

public:
    static Koth* instance()
    {
        static Koth instance;
        return &instance;
    }

    typedef std::map<uint64, KothQueueInfo> KothQueuedPlayersMap;

    struct cmpByJoinTime
    {
        bool operator()(const KothQueuedPlayersMap::iterator& lhs, const KothQueuedPlayersMap::iterator& rhs) const { return lhs->second.JoinTime < rhs->second.JoinTime; }
    };

    typedef std::map <KothQueuedPlayersMap::iterator, uint64, cmpByJoinTime> KothRQueuedPlayersMap;

    KothQueuedPlayersMap m_QueuedPlayers;
    KothRQueuedPlayersMap m_RQueuedPlayers;

    void Reset(bool init);

    KothStates GetKothState() { return KothState; }


    //messaging
	void SendMessageToQueue(std::string text);
	void SendMessageToPlayer(Player* player, std::string text);
    void SendMessageToFighters(std::string text);

    //Queue
    bool IsInQueue(uint64 guid);
    void KothQueueUpdate(uint32 diff);
    void QueueAddPlayer(Player* player);
    void QueueRemovePlayer(Player* player);

    //invites
    bool QueueInvitePlayer(uint64 guid, uint8 slot);
    void PlayerInviteResponse(Player* player, bool accept);
    void SetState(KothStates state) { KothState = state; }
    void IncreaseWaitingCount() { m_WaitingCount++; }
    void DecreaseWaitingCount() { m_WaitingCount--; }

    void CancelInvite(Player* player);

    //Arena
    void IncreaseFighterCount() { m_fighterCount++ ; }
    void DecreaseFighterCount() { m_fighterCount--; }
    uint8 GetFighterCount() { return m_fighterCount; }
    uint8 GetMaxFighters() { return m_maxFighters; }
    void TeleportFighter(Player* player, uint8 slot);

    uint64 GetFighterGUID(uint8 slot) { return m_fighterGUIDs[slot]; }
    void SetFighterGUID(uint64 fighterGUID, uint8 slot) { m_fighterGUIDs[slot] = fighterGUID; }
    void ArenaAddPlayer(Player* player, uint8 slot);
    void ArenaRemovePlayer(Player* player);

    bool PrepareArena();

    void TeleportFightersStartPosition();

    //debug
    void Debug(Player* player, uint8 mode);
    bool m_Exec;
private:
    std::vector<uint64> m_fighterGUIDs;
    uint64 m_oldwinnerGUID;
    uint8 m_streak;
    uint8 m_fighterCount;
    uint8 m_maxFighters;

    KothStates KothState;

    //players invited/waiting for match to start (accepted invitation) "temporary" taking of slot
    uint32 m_WaitingCount;

    EventProcessor m_events;
};

class KothQueueRemoveEvent : public BasicEvent
{
public:
    KothQueueRemoveEvent(uint64 pl_guid, uint32 removeTime)
        : m_PlayerGuid(pl_guid), m_RemoveTime(removeTime)
    { }

    virtual ~KothQueueRemoveEvent() { }

    virtual bool Execute(uint64 e_time, uint32 p_time) override;
    virtual void Abort(uint64 e_time) override;
private:
    uint64 m_PlayerGuid;
    uint32 m_RemoveTime;
};

class KothArenaCountdown : public BasicEvent
{
public:
    KothArenaCountdown(uint32 timeLimit)
        : m_TimeLimit(timeLimit)
    { }

    virtual ~KothArenaCountdown() { }

    virtual bool Execute(uint64 e_time, uint32 p_time) override;
    virtual void Abort(uint64 e_time) override;
private:
    uint32 m_TimeLimit;
};

class KothArenaPostCountdown : public BasicEvent
{
public:
    KothArenaPostCountdown(uint32 timeLimit)
        : m_TimeLimit(timeLimit)
    { }

    virtual ~KothArenaPostCountdown() { }

    virtual bool Execute(uint64 e_time, uint32 p_time) override;
    virtual void Abort(uint64 e_time) override;
private:
    uint32 m_TimeLimit;
};

#define sKothMgr Koth::instance()
#endif