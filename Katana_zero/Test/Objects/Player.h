#pragma once
#include <windows.h>
#include <atlimage.h>
#include <math.h>
#include <algorithm>
#include <vector>

extern int g_playerAfterImageInterval;
extern int g_playerAfterImageIntervalSlowMo;
extern int g_playerAfterImageCount;
extern float g_timeSlowScale;
extern int g_slowMoDurationLimit;
extern float g_slowMoJumpForceScale;
extern float g_slowMoMoveForceScale;
extern int g_maxJumpHoldTime;
#include "Physics.h"

enum class PlayerState {
    IDLE, IDLE_TO_WALK, WALK, WALK_TO_IDLE, RUN,
    JUMP_UP, FALL,
    PREVDOWN, DOWN, POSTDOWN,
    ROLL, ATTACK,
    WALL_GRAB, WALL_SLIDE, WALL_FLIP,
    DOOR_KICK, DOOR_KICK_FULL,
    DEAD
};

class Player {
private:
    static Player* s_instance;
    float m_x, m_y, m_vx, m_vy;
    PlayerState m_state;
    bool m_isJumping, m_isFacingRight;
    int m_currentFrame;
    DWORD m_lastTime;

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

    float GetBatteryConsumptionPerSec() const { return m_batteryMax / m_slowMoDuration; }
    float GetBatteryRecoveryPerSec() const { return m_batteryMax / m_slowMoRecoveryTime; }

    CImage imgIdle[11], imgWalk[10], imgRun[10], imgJumpUp[4], imgFall[4], imgPrevDown[2], imgDown[1], imgPostDown[2], imgRoll[6], imgAttack[7], imgSlashFX[5], imgWallGrab[2], imgWallSlide[1], imgWallFlip[11], imgIdleToWalk[4], imgWalkToIdle[5], imgDoorKick[6], imgDoorKickFull[10];

public:
    Player();
    ~Player();

    static Player& GetInstance() { return *s_instance; }

    void Init();
    void Update(int mouseX, int mouseY, float camX, float camY, float rs, float ox, float oy, bool fv);
    void UpdateAnimation();
    void Render(HDC hMemDC, float camX, float camY, float mapScale, float playerScale, float g_renderMapScale, float g_mapOffsetX, float g_mapOffsetY, bool g_isFullMapView, bool g_showDebugRect);
    void OnTakeDamage(float damage);
    void SetState(PlayerState state) { 
        if (m_state != state) { 
            m_state = state; 
            m_currentFrame = 0; 
            if (state == PlayerState::DEAD) {
                m_vx = 0;
                m_vy = 0;
            }
        } 
    }

    float GetX() const { return m_x; }
    float GetY() const { return m_y; }
    float GetColW() const { return m_colW; }
    float GetColH() const { return m_colH; }
    float GetAttackDirX() const { return m_attackDirX; }
    float GetAttackDirY() const { return m_attackDirY; }
    float GetAttackHitW() const { return m_attackHitW; }
    float GetAttackHitH() const { return m_attackHitH; }
    float GetAttackHitX() const { return m_x + m_colW / 2.0f + m_attackDirX * m_attackHitOffset - m_attackHitW / 2.0f; }
    float GetAttackHitY() const { return m_y + m_colH / 2.0f + m_attackDirY * m_attackHitOffset - m_attackHitH / 2.0f; }
    float GetAttackHitOffset() const { return m_attackHitOffset; }
    int GetCurrentFrame() const { return m_currentFrame; }
    PlayerState GetState() const { return m_state; }
    bool GetIsSlowMo() const { return m_isSlowMo; }
    float GetBatteryLevel() const { return m_batteryLevel; }
    RECT GetRect() const { return { (int)m_x, (int)m_y, (int)(m_x + m_colW), (int)(m_y + m_colH) }; }

    enum class ReplayEvent { ENEMY_DIE, DOOR_OPEN };
    struct ReplayEventData {
        ReplayEvent type;
        int targetIdx; // 적 인덱스 또는 문 인덱스
    };

    struct PlayerSnapshot {
        float x, y;
        PlayerState state;
        int frame;
        bool isFacingRight;
        float attackAngle;
        std::vector<ReplayEventData> events;
    };
    std::vector<PlayerSnapshot> m_history;
    std::vector<PlayerSnapshot> m_snapshots;
    
    void AddReplayEvent(ReplayEvent type, int idx) {
        if (!m_snapshots.empty()) {
            m_snapshots.back().events.push_back({ type, idx });
        }
    }
    bool m_isRewinding = false;
    int m_rewindSpeed = 1;
    int m_maxHistorySize = 600;

    void ClearAfterImages() {
        for (auto& img : m_afterImages) img.active = false;
    }

    void StartRewind(int speed = 2) { 
        m_isRewinding = true; 
        m_rewindSpeed = speed; 
        m_isSlowMo = false; // 리와인드 시작 시 슬로우 모션 강제 종료
        m_batteryLevel = m_batteryMax; // 배터리(슬로우 모드 게이지) 풀 회복
        ClearAfterImages();
    }
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
};

