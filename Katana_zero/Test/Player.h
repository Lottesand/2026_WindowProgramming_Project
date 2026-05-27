#pragma once
#include <windows.h>
#include <atlimage.h>
#include <math.h>
#include <algorithm>
#include "Physics.h"

enum class PlayerState {
    IDLE, IDLE_TO_WALK, WALK, WALK_TO_IDLE, RUN,
    JUMP_UP, FALL,
    PREVDOWN, DOWN, POSTDOWN,
    ROLL, ATTACK,
    WALL_GRAB, WALL_SLIDE, WALL_FLIP
};

class Player {
private:
    float m_x, m_y;
    float m_vx, m_vy;
    PlayerState m_state;
    bool m_isJumping;
    bool m_isFacingRight;
    int m_currentFrame;
    DWORD m_lastTime;

    // 튜닝 변수들
    float m_colW, m_colH;
    float m_moveSpeedWalk;
    float m_moveSpeedRoll;
    float m_accelRate;
    float m_frictionRate;
    const float m_jumpPower = -10.5f;
    const float m_gravityNormal = 1.0f;
    const float m_gravityHold = 0.45f;
    const float m_maxFallSpeed = 30.0f;

    float m_dashRadius;
    float m_dashSpeed;
    DWORD m_attackCooldown;
    DWORD m_lastAttackTime;

    DWORD m_wallHangTime;
    float m_wallSlideSpeed;
    float m_wallSlideFastSpeed;
    float m_wallJumpPowerY;
    float m_wallJumpPowerX;

    float m_speedIdleToWalk;
    DWORD m_aniDelayIdleToWalk;
    DWORD m_aniDelayWalkToIdle;

    DWORD m_aniDelayIdle;
    DWORD m_aniDelayWalk;
    DWORD m_aniDelayRun;
    DWORD m_aniDelayJumpFall;
    DWORD m_aniDelayCrouch;
    DWORD m_aniDelayRoll;
    DWORD m_aniDelayAttack;
    DWORD m_aniDelaySlash;
    DWORD m_aniDelayWallGrab;
    DWORD m_aniDelayWallSlide;
    DWORD m_aniDelayWallFlip;

    bool m_canRoll;
    bool m_canJump;
    bool m_canAirYDash;

    DWORD m_wallGrabTime;
    int m_wallDir;

    float m_attackTargetX;
    float m_attackTargetY;
    float m_attackDirX;
    float m_attackDirY;
    float m_dashDirX;
    float m_dashDirY;
    float m_attackAngle;

    float m_attackHitW;
    float m_attackHitH;
    float m_attackHitOffset;

    // 이미지 에셋
    CImage imgIdle[11], imgWalk[10], imgRun[10], imgJumpUp[4], imgFall[4];
    CImage imgPrevDown[2], imgDown[1], imgPostDown[2], imgRoll[6], imgAttack[7], imgSlashFX[5];
    CImage imgWallGrab[2], imgWallSlide[1], imgWallFlip[11];
    CImage imgIdleToWalk[4];
    CImage imgWalkToIdle[5];

public:
    Player();
    ~Player();

    void Init();
    void Update(int mouseX, int mouseY, float camX, float camY, float g_renderMapScale, float g_mapOffsetX, float g_mapOffsetY, bool g_isFullMapView);
    void UpdateAnimation();
    void Render(HDC hMemDC, float camX, float camY, float mapScale, float playerScale, float g_renderMapScale, float g_mapOffsetX, float g_mapOffsetY, bool g_isFullMapView, bool g_showDebugRect);

    // Getters
    float GetX() const { return m_x; }
    float GetY() const { return m_y; }
    float GetColW() const { return m_colW; }
    float GetColH() const { return m_colH; }
    float GetAttackDirX() const { return m_attackDirX; }
    float GetAttackDirY() const { return m_attackDirY; }
    float GetAttackHitW() const { return m_attackHitW; }
    float GetAttackHitH() const { return m_attackHitH; }
    float GetAttackHitOffset() const { return m_attackHitOffset; }
    int GetCurrentFrame() const { return m_currentFrame; }
    PlayerState GetState() const { return m_state; }
};
