#pragma once
#include <windows.h>
#include <atlimage.h>
#include <math.h>
#include <algorithm>
#include <vector>
#include "Item.h"
#include <deque>
#include "../Core/Common.h"

// ?꾩뿭 蹂???좎뼵
extern int g_playerAfterImageInterval;
extern int g_playerAfterImageIntervalSlowMo;
extern int g_playerAfterImageCount;
extern float g_timeSlowScale;
extern int g_slowMoDurationLimit;
extern float g_slowMoJumpForceScale;
extern float g_slowMoMoveForceScale;
extern int g_maxJumpHoldTime;

enum class PlayerState {
    PS_IDLE, PS_IDLE_TO_WALK, PS_WALK, PS_WALK_TO_IDLE, PS_RUN,
    PS_JUMP_UP, PS_FALL,
    PS_PREVDOWN, PS_DOWN, PS_POSTDOWN,
    PS_ROLL, PS_ATTACK,
    PS_WALL_GRAB, PS_WALL_SLIDE, PS_WALL_FLIP,
    PS_DOOR_KICK, PS_DOOR_KICK_FULL,
    PS_DEAD, PS_DEAD_FLY_BEGIN, PS_DEAD_FLY_LOOP, PS_DEAD_GROUND, PS_PIT_DEATH
};

class Player {
private:
    float m_x, m_y, m_vx, m_vy;
    PlayerState m_state;
    bool m_isJumping, m_isFacingRight;
    int m_currentFrame;
    DWORD m_lastTime;
    DWORD m_prevTime; // 추가

    float m_colW, m_colH, m_moveSpeedWalk, m_moveSpeedRoll, m_accelRate, m_frictionRate;
    const float m_jumpPower = -11.0f;
    const float m_gravityNormal = 1.0f;
    const float m_gravityHold = 0.45f;
    const float m_maxFallSpeed = 30.0f;

    float m_dashRadius, m_dashSpeed;
    DWORD m_attackCooldown, m_lastAttackTime;

    DWORD m_wallHangTime;
    float m_wallSlideSpeed, m_wallSlideFastSpeed, m_wallJumpPowerY, m_wallJumpPowerX;

    float m_speedIdleToWalk;
    DWORD m_aniDelayIdleToWalk, m_aniDelayWalkToIdle, m_aniDelayIdle, m_aniDelayWalk, m_aniDelayRun, m_aniDelayJumpFall, m_aniDelayCrouch, m_aniDelayRoll, m_aniDelayAttack, m_aniDelaySlash, m_aniDelayWallGrab, m_aniDelayWallSlide, m_aniDelayWallFlip;

    bool m_canRoll, m_canJump, m_canAirYDash;
    DWORD m_jumpHoldTimer, m_maxJumpHoldTime;

    DWORD m_wallGrabTime;
    int m_wallDir;

    float m_attackTargetX, m_attackTargetY, m_attackDirX, m_attackDirY, m_dashDirX, m_dashDirY, m_attackAngle, m_attackHitW, m_attackHitH, m_attackHitOffset;

    bool m_hasLeapedInAir, m_isAttackClicked;
    int m_slashIndex = 0; // Sequential slash sound index (0-2)

    struct AfterImageData {
        float x, y;
        PlayerState state;
        int frame;
        bool isFacingRight;
        float attackAngle;
        bool active;
    };
    std::vector<AfterImageData> m_afterImages;
    DWORD m_lastAfterImageTime;

    bool m_isSlowMo, m_canSlowMo;
    DWORD m_slowMoStartTime;
    float m_batteryLevel, m_lastTimeScale;
    float m_dustOffsetX[3] = { -2.0f, -15.0f, -28.0f }; 
    float m_dustOffsetY[3] = { 3.0f, -2.0f, 5.0f };
    bool m_wasMoving = false;
    int m_lastRollFrame = -1;

    const float m_batteryMax = 11.0f;
    float m_slowMoDuration = 6.5f;
    float m_slowMoRecoveryTime = 11.0f;

    bool m_isGodMode;

    CImage imgIdle[11], imgWalk[10], imgRun[10], imgJumpUp[4], imgFall[4], imgPrevDown[2], imgDown[1], imgPostDown[2], imgRoll[6], imgAttack[7], imgSlashFX[5], imgWallGrab[2], imgWallSlide[1], imgWallFlip[11], imgIdleToWalk[4], imgWalkToIdle[5], imgDoorKick[6], imgDoorKickFull[10];
    CImage imgHurtFlyBegin[2], imgHurtFlyLoop[4], imgHurtGround[6];

public:
    Player();
    ~Player();

    void Init();
    void Update(int mouseX, int mouseY, float camX, float camY, float rs, float ox, float oy, bool fv);
    void UpdateAnimation();
    void Render(HDC hMemDC, Gdiplus::Graphics* g, float camX, float camY, float mapScale, float playerScale, float g_renderMapScale, float g_mapOffsetX, float g_mapOffsetY, bool g_isFullMapView, bool g_showDebugRect, float stageTimer = 10.0f);
    void RenderSilhouette(Gdiplus::Graphics* g, float camX, float camY, float mapScale); // 추가
    
    void SetState(PlayerState state);

    float GetX() const { return m_x; }
    float GetY() const { return m_y; }
    float GetColW() const { return m_colW; }
    float GetColH() const { return m_colH; }
    float GetAttackDirX() const { return m_attackDirX; }
    float GetAttackDirY() const { return m_attackDirY; }
    float GetDashDirX() const { return m_dashDirX; }
    float GetDashDirY() const { return m_dashDirY; }
    float GetAttackHitW() const { return m_attackHitW; }
    float GetAttackHitH() const { return m_attackHitH; }
    float GetAttackHitX() const { return m_x + m_colW / 2.0f + m_attackDirX * m_attackHitOffset - m_attackHitW / 2.0f; }
    float GetAttackHitY() const { return m_y + m_colH / 2.0f + m_attackDirY * m_attackHitOffset - m_attackHitH / 2.0f; }
    float GetAttackHitOffset() const { return m_attackHitOffset; }
    RECT GetAttackRect() const { 
        float cX = m_x + m_colW / 2.0f, cY = m_y + m_colH / 2.0f;
        float hX = cX + m_attackDirX * 40.0f - 40.0f, hY = cY + m_attackDirY * 40.0f - 30.0f;
        return { (int)hX, (int)hY, (int)(hX + 80.0f), (int)(hY + 60.0f) };
    }
    int GetCurrentFrame() const { return m_currentFrame; }
    PlayerState GetState() const { return m_state; }
    bool GetIsSlowMo() const { return m_isSlowMo; }
    float GetBatteryLevel() const { return m_batteryLevel; }
    bool IsDead() const;
    bool IsDeathAnimationFinished() const;
    bool IsPitFalling() const { return m_state == PlayerState::PS_PIT_DEATH; }

    enum class ReplayEvent { ENEMY_DIE, DOOR_OPEN };
    struct ReplayEventData {
        ReplayEvent type;
        int targetIdx;
    };

    struct PlayerSnapshot {
        float x, y;
        PlayerState state;
        int frame;
        bool isFacingRight;
        float attackAngle;
        std::vector<ReplayEventData> events;
    };
    std::deque<PlayerSnapshot> m_history;
    std::vector<PlayerSnapshot> m_snapshots;
    
    void AddReplayEvent(ReplayEvent type, int idx);

    bool m_isRewinding = false;
    int m_rewindSpeed = 1;
    int m_maxHistorySize = 600;

    void ClearAfterImages();
    void StartRewind(int speed = 2);
    void StopRewind() { m_isRewinding = false; }
    bool IsRewinding() const { return m_isRewinding; }
    void SetMaxHistory(int seconds) { m_maxHistorySize = seconds * 60; ClearHistory(); }
    void ClearHistory() { m_history.clear(); }
    void SetPos(float x, float y) { m_x = x; m_y = y; m_vx = 0; m_vy = 0; }
    int GetHistorySize() const { return (int)m_history.size(); }
    void ClearSnapshots() { m_snapshots.clear(); }
    const std::vector<PlayerSnapshot>& GetSnapshots() const { return m_snapshots; }
    void SetGodMode(bool god) { m_isGodMode = god; }
    bool IsGodMode() const { return m_isGodMode; }
    float m_bloodDistance;
    DWORD m_lastBleedTime;
    void OnTakeDamage(float damage, float kvx = 0.0f, float kvy = 0.0f, float sourceX = -1.0f, float sourceY = -1.0f, DeathCause cause = DeathCause::SWORD);

    // Item management
    class Item* m_pHeldItem = nullptr;
    float m_itemPopupTimer = 0.0f;
    ItemType m_popupItemType;
    
    // Flamethrower state
    bool m_isFiringFlamethrower = false;
    float m_flamethrowerHoldTime = 0.0f;
    float m_flamethrowerFuel = 4.0f; // Max 4 seconds
    float m_flameDirX = 1.0f;
    float m_flameDirY = 0.0f;
    CImage imgFlamethrowerUI[8];
    
    void PickUpItem(class Item* item);
    void ThrowItem(int mouseX, int mouseY, float camX, float camY, float rs, float ox, float oy, bool fv);
    void DiscardHeldItem();
    
    bool HasHeldItem() const { return m_pHeldItem != nullptr; }
    int GetHeldItemType() const;
    bool IsFiringFlamethrower() const { return m_isFiringFlamethrower; }
    float GetFlamethrowerHoldTime() const { return m_flamethrowerHoldTime; }
    float GetFlameDirX() const { return m_flameDirX; }
    float GetFlameDirY() const { return m_flameDirY; }
    float GetFlamethrowerFuel() const { return m_flamethrowerFuel; }


    // Stun logic
    bool m_isStunned = false;
    float m_stunTimer = 0.0f;
    float m_maxStunTime = 0.0f;
    void Stun(float duration, float kvx, float kvy);

    // Struggle/Cutscene control
    bool m_isVisible = true;
    bool m_hasHitThisSwing = false; // Prevents multiple hits in one attack
    void SetVisible(bool visible) { m_isVisible = visible; }
    void ForceStop() { m_vx = 0; m_vy = 0; }
    bool HasHitThisSwing() const { return m_hasHitThisSwing; }
    void SetHasHitThisSwing(bool hit) { m_hasHitThisSwing = hit; }
};
