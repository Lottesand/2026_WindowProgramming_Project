#define NOMINMAX
#include "Player.h"
#include "Physics.h"
#include "../SceneAndMap/StageManager.h"
#include "../Effects/EffectManager.h"
#include <objidl.h>
#include <gdiplus.h>
#include <map>
#include <deque>
#include <cstring>
#include <cmath>

int g_playerAttackCooldown = 130; 
int g_playerAttackDuration = 2;
int g_playerAfterImageInterval = 1;
int g_playerAfterImageIntervalSlowMo = 30;
int g_playerAfterImageCount = 4;
float g_timeSlowScale = 0.3f;
int g_slowMoDurationLimit = 6500;
float g_slowMoJumpForceScale = 1.0f;
float g_slowMoMoveForceScale = 1.0f;
int g_maxJumpHoldTime = 200;

Player::Player() {
    m_x = 100.0f; m_y = 200.0f; m_vx = 0.0f; m_vy = 0.0f;
    m_state = PlayerState::PS_IDLE; m_isJumping = false; m_isFacingRight = true;
    m_currentFrame = 0; m_lastTime = GetTickCount();
    m_colW = 40.0f; m_colH = 64.0f;
    m_moveSpeedWalk = 12.0f; m_moveSpeedRoll = 15.0f;
    m_accelRate = 0.6f; m_frictionRate = 0.3f;
    m_dashRadius = 150.0f; m_dashSpeed = 25.0f;
    m_attackCooldown = (DWORD)g_playerAttackCooldown; m_lastAttackTime = 0;
    m_wallHangTime = 150; m_wallSlideSpeed = 2.5f; m_wallSlideFastSpeed = 12.0f;
    m_wallJumpPowerY = -11.0f; m_wallJumpPowerX = 14.0f;
    m_speedIdleToWalk = 1.0f; m_aniDelayIdleToWalk = 60; m_aniDelayWalkToIdle = 60;
    m_aniDelayIdle = 150; m_aniDelayWalk = 80; m_aniDelayRun = 80;
    m_aniDelayJumpFall = 100; m_aniDelayCrouch = 80; m_aniDelayRoll = 50;
    m_aniDelayAttack = 40; m_aniDelaySlash = 40;
    m_aniDelayWallGrab = 80; m_aniDelayWallSlide = 100; m_aniDelayWallFlip = 40;
    m_canRoll = true; m_canJump = true; m_canAirYDash = true;
    m_jumpHoldTimer = 0; m_maxJumpHoldTime = 200;
    m_wallGrabTime = 0; m_wallDir = 0;
    m_attackTargetX = 0.0f; m_attackTargetY = 0.0f;
    m_attackDirX = 0.0f; m_attackDirY = 0.0f;
    m_dashDirX = 0.0f; m_dashDirY = 0.0f;
    m_attackAngle = 0.0f;
    m_attackHitW = 80.0f; m_attackHitH = 60.0f;
    m_attackHitOffset = 40.0f;
    m_afterImages.resize(g_playerAfterImageCount);
    for (int i = 0; i < g_playerAfterImageCount; i++) m_afterImages[i].active = false;
    m_lastAfterImageTime = GetTickCount();
    m_hasLeapedInAir = false; m_isAttackClicked = false;
    m_isSlowMo = false; m_canSlowMo = true; m_slowMoStartTime = 0;
    m_batteryLevel = 11.0f; m_lastTimeScale = 1.0f;
    m_isGodMode = false;
    m_bloodDistance = 0.0f;
    m_snapshots.reserve(3000);
}

Player::~Player() {}

void Player::Init() {
    if (!imgIdle[0].IsNull()) return;
    TCHAR path[256];
    for (int i = 0; i < 11; i++) { wsprintf(path, TEXT("assets/player/idle/%d.png"), i); imgIdle[i].Load(path); }
    for (int i = 0; i < 10; i++) { wsprintf(path, TEXT("assets/player/walk/%d.png"), i); imgWalk[i].Load(path); }
    for (int i = 0; i < 10; i++) { wsprintf(path, TEXT("assets/player/run/%d.png"), i); imgRun[i].Load(path); }
    for (int i = 0; i < 4; i++) { wsprintf(path, TEXT("assets/player/jump/%d.png"), i); imgJumpUp[i].Load(path); }
    for (int i = 0; i < 4; i++) { wsprintf(path, TEXT("assets/player/fall/%d.png"), i); imgFall[i].Load(path); }
    for (int i = 0; i < 2; i++) { wsprintf(path, TEXT("assets/player/prevdown/%d.png"), i); imgPrevDown[i].Load(path); }
    for (int i = 0; i < 1; i++) { wsprintf(path, TEXT("assets/player/down/%d.png"), i); imgDown[i].Load(path); }
    for (int i = 0; i < 2; i++) { wsprintf(path, TEXT("assets/player/postdown/%d.png"), i); imgPostDown[i].Load(path); }
    for (int i = 0; i < 6; i++) { wsprintf(path, TEXT("assets/player/roll/%d.png"), i); imgRoll[i].Load(path); }
    for (int i = 0; i < 2; i++) { wsprintf(path, TEXT("assets/player/wallgrab/%d.png"), i); imgWallGrab[i].Load(path); }
    for (int i = 0; i < 1; i++) { wsprintf(path, TEXT("assets/player/wallslide/%d.png"), i); imgWallSlide[i].Load(path); }
    for (int i = 0; i < 11; i++) { wsprintf(path, TEXT("assets/player/wallflip/%d.png"), i); imgWallFlip[i].Load(path); }
    for (int i = 0; i < 4; i++) { wsprintf(path, TEXT("assets/player/idletowalk/%d.png"), i); imgIdleToWalk[i].Load(path); }
    for (int i = 0; i < 5; i++) { wsprintf(path, TEXT("assets/player/walktoidle/%d.png"), i); imgWalkToIdle[i].Load(path); }
    for (int i = 0; i < 7; i++) { wsprintf(path, TEXT("assets/player/attack/%d.png"), i); imgAttack[i].Load(path); }
    for (int i = 0; i < 5; i++) { wsprintf(path, TEXT("assets/player/slash/%d.png"), i); imgSlashFX[i].Load(path); }
    for (int i = 0; i < 6; i++) { wsprintf(path, TEXT("assets/player/spr_doorbreak/%d.png"), i); imgDoorKick[i].Load(path); }
    for (int i = 0; i < 10; i++) { wsprintf(path, TEXT("assets/player/spr_doorbreak_full/%d.png"), i); imgDoorKickFull[i].Load(path); }
    for (int i = 0; i < 2; i++) { wsprintf(path, TEXT("assets/player/spr_hurtfly_begin/%d.png"), i); imgHurtFlyBegin[i].Load(path); }
    for (int i = 0; i < 4; i++) { wsprintf(path, TEXT("assets/player/spr_hurtfly_loop/%d.png"), i); imgHurtFlyLoop[i].Load(path); }
    for (int i = 0; i < 6; i++) { wsprintf(path, TEXT("assets/player/spr_hurtground/%d.png"), i); imgHurtGround[i].Load(path); }
}

void Player::Update(int mouseX, int mouseY, float camX, float camY, float rs, float ox, float oy, bool fv) {
    bool isDead = IsDead();
    if (isDead && !m_isRewinding && m_state != PlayerState::PS_DEAD_FLY_BEGIN && m_state != PlayerState::PS_DEAD_FLY_LOOP && m_state != PlayerState::PS_DEAD_GROUND && m_state != PlayerState::PS_PIT_DEATH) return;
    if (m_state == PlayerState::PS_DEAD_GROUND && !m_isRewinding) return;

    DWORD ct = GetTickCount();
    
    // 아이템 팝업 타이머 업데이트
    if (m_itemPopupTimer > 0) {
        float ts = m_isSlowMo ? 0.3f : 1.0f;
        m_itemPopupTimer -= 0.016f * ts; // 약 60fps 기준
        if (m_itemPopupTimer < 0) m_itemPopupTimer = 0;
    }

    if (m_isRewinding) {
        if (m_history.empty()) { m_x = StageManager::GetPlayerStartX(); m_y = StageManager::GetPlayerStartY() - m_colH; return; }
        for (int i = 0; i < m_rewindSpeed; i++) { if (m_history.empty()) break; PlayerSnapshot d = m_history.back(); m_history.pop_back(); m_x = d.x; m_y = d.y; m_state = d.state; m_currentFrame = d.frame; m_isFacingRight = d.isFacingRight; m_attackAngle = d.attackAngle; }
        return;
    }
    
    PlayerSnapshot snap;
    snap.x = m_x; snap.y = m_y; snap.state = m_state; snap.frame = m_currentFrame; 
    snap.isFacingRight = m_isFacingRight; snap.attackAngle = m_attackAngle;

    if (!m_isSlowMo) {
        m_history.push_back(snap);
        if (m_history.size() > (size_t)m_maxHistorySize) m_history.pop_front();
    }
    m_snapshots.push_back(snap);

    // Item interaction
    bool curR = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
    static bool prR = false;
    
    bool pickupHappened = false;
    auto items = StageManager::GetCurrentItems();
    if (items) {
        for (auto it = items->begin(); it != items->end(); ) {
            if (it->GetState() == ItemState::ON_GROUND) {
                float dx = it->GetX() - (m_x + m_colW / 2.0f);
                float dy = it->GetY() - (m_y + m_colH / 2.0f);
                float dist = sqrt(dx * dx + dy * dy);
                
                // 디버그: 거리 출력
                // TCHAR buf[128]; wsprintf(buf, TEXT("Dist: %f, Auto: %d\n"), dist, (m_pHeldItem == nullptr && dist < 30.0f)); OutputDebugString(buf);

                // 자동 습득: 들고 있는 아이템이 없을 때만 가까이 가면 습득
                bool canAutoPickup = (m_pHeldItem == nullptr && dist < 30.0f);
                // 수동 습득: 들고 있는 아이템이 있어도 우클릭 시 교체 가능 (200px)
                bool canManualPickup = (curR && !prR && dist < 200.0f);

                if (canAutoPickup || canManualPickup) {
                    Item* pickedItem = new Item(*it); 
                    PickUpItem(pickedItem);
                    it = items->erase(it);
                    pickupHappened = true;
                    continue; // erase 후에는 it++를 하지 않음
                }

                // 화살표 표시 (항상 가능)
                if (dist < 200.0f) {
                    it->SetShowIndicator(true);
                } else {
                    it->SetShowIndicator(false);
                }
                it++;
            } else {
                it++;
            }
        }
    }

    // 아이템 습득이 일어나지 않았고, 아이템을 들고 있는 상태에서 우클릭 시 던지기
    if (curR && !prR && !pickupHappened && m_pHeldItem) {
        ThrowItem(mouseX, mouseY, camX, camY, rs, ox, oy, fv);
    }
    prR = curR;
    
    // Original update code continues...
    bool isW = false, isA = false, isS = false, isD = false, isJ = false;
    if (!isDead) { isW = GetAsyncKeyState('W') & 0x8000; isA = GetAsyncKeyState('A') & 0x8000; isS = GetAsyncKeyState('S') & 0x8000; isD = GetAsyncKeyState('D') & 0x8000; isJ = (GetAsyncKeyState('W') & 0x8000) || (GetAsyncKeyState(VK_SPACE) & 0x8000); }
    int twd = 0; if (CheckSpecificCollision(m_x - 3.0f, m_y, m_colW, m_colH, 3)) twd = -1; else if (CheckSpecificCollision(m_x + 3.0f, m_y, m_colW, m_colH, 3)) twd = 1;
    bool isShift = false; if (!isDead) isShift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
    static DWORD lt_player = ct; float dT = (ct - lt_player) / 1000.0f; lt_player = ct;
    if (isShift) { if (m_canSlowMo && m_batteryLevel >= 1.0f) { if (!m_isSlowMo) { m_isSlowMo = true; m_slowMoStartTime = ct; } } if (m_isSlowMo) { m_batteryLevel -= (11.0f / 6.5f) * dT; if (m_batteryLevel <= 0.0f) { m_batteryLevel = 0.0f; m_isSlowMo = false; m_canSlowMo = false; } } }
    else { m_isSlowMo = false; m_canSlowMo = true; }
    if (!m_isSlowMo && m_batteryLevel < 11.0f) { m_batteryLevel += (11.0f / 11.0f) * dT; if (m_batteryLevel > 11.0f) m_batteryLevel = 11.0f; }
    float ts = m_isSlowMo ? 0.3f : 1.0f;

    if (m_itemPopupTimer > 0) {
        m_itemPopupTimer -= dT * ts;
        if (m_itemPopupTimer < 0) m_itemPopupTimer = 0;
    }

    if (isDead && !m_isRewinding) {
        m_bloodDistance += (float)sqrt(m_vx * m_vx + m_vy * m_vy) * ts;
        if (m_bloodDistance >= 15.0f) {
            m_bloodDistance -= 15.0f;
            float length = (float)sqrt(m_vx * m_vx + m_vy * m_vy);
            if (length > 0) {
                float nvx = m_vx / length; float nvy = m_vy / length;
                float perpX1 = -nvy; float perpY1 = nvx;
                float perpX2 = nvy; float perpY2 = -nvx;
                float angle1 = atan2(perpY1, perpX1); float angle2 = atan2(perpY2, perpX2);
                float speed1 = 2.0f + (rand() % 30) / 10.0f; float speed2 = 2.0f + (rand() % 30) / 10.0f;
                EffectManager::AddBloodSplatter(m_x + m_colW / 2.0f, m_y + m_colH / 2.0f, perpX1 * speed1 + ((rand() % 100) / 100.0f - 0.5f), perpY1 * speed1 + ((rand() % 100) / 100.0f - 0.5f), angle1, ct);
                EffectManager::AddBloodSplatter(m_x + m_colW / 2.0f, m_y + m_colH / 2.0f, perpX2 * speed2 + ((rand() % 100) / 100.0f - 0.5f), perpY2 * speed2 + ((rand() % 100) / 100.0f - 0.5f), angle2, ct);
                
                // Add persistent map blood only if on visible background
                if (!IsMapTransparent((int)(m_x + m_colW / 2.0f), (int)(m_y + m_colH / 2.0f))) {
                    bool isMovingFast = (sqrt(m_vx * m_vx + m_vy * m_vy) > 2.0f);
                    EffectManager::AddMapBlood(m_x + m_colW / 2.0f, m_y + m_colH / 2.0f, atan2(m_vy, m_vx), isMovingFast);
                }
            }
        }
        m_vx *= 0.98f; 
    }

    if (ts != m_lastTimeScale) { float f = ts / m_lastTimeScale; m_vx *= f; m_vy *= f; m_lastTimeScale = ts; }
    float cAcc = 0.6f * ts * ts, cFri = 0.3f * ts * ts, cDSp = 25.0f * ts, cWSp = 12.0f * ts;
    float cJP = -11.0f * ts, cWJP_Y = -11.0f * ts, cWJP_X = 14.0f * (m_isSlowMo ? ts : 1.0f);
    bool air = m_isJumping || (m_vy != 0.0f); if (!isJ) m_canJump = true;
    if (!isDead && isJ && m_canJump) {
        if (m_state == PlayerState::PS_WALL_GRAB || m_state == PlayerState::PS_WALL_SLIDE) {
            m_state = PlayerState::PS_WALL_FLIP; m_currentFrame = 0; m_vy = cWJP_Y; m_vx = (m_wallDir == 1) ? -cWJP_X : cWJP_X; m_isFacingRight = (m_wallDir == -1); m_isJumping = true; m_canJump = false; m_x += (m_wallDir == 1) ? -2.0f : 2.0f; twd = 0; m_jumpHoldTimer = ct;
            if (!m_isSlowMo) EffectManager::AddJumpCloudVFX(m_x + ((m_wallDir == 1) ? m_colW + 2.0f : -2.0f), m_y + m_colH / 2.0f, ct, (m_wallDir == 1) ? -1.5708f : 1.5708f);
        } else if (!air && m_state != PlayerState::PS_ROLL && m_state != PlayerState::PS_ATTACK && m_state != PlayerState::PS_WALL_FLIP && m_state != PlayerState::PS_DOOR_KICK && m_state != PlayerState::PS_DOOR_KICK_FULL) {
            m_vy = cJP; m_isJumping = true; m_canJump = false; m_jumpHoldTimer = ct; if (!m_isSlowMo) EffectManager::AddJumpCloudVFX(m_x + m_colW / 2.0f, m_y + m_colH, ct);
        }
    }
    bool curL = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0; static bool prL = false;
    float wx = (mouseX - ox) / rs, wy = (mouseY - oy) / rs; if (!fv) { wx += camX; wy += camY; }
    if (curL && !prL && m_state != PlayerState::PS_ATTACK && m_state != PlayerState::PS_PREVDOWN && m_state != PlayerState::PS_DOWN && m_state != PlayerState::PS_DOOR_KICK && m_state != PlayerState::PS_DOOR_KICK_FULL) {
        if (ct - m_lastAttackTime >= (DWORD)g_playerAttackCooldown) {
            m_state = PlayerState::PS_ATTACK; m_currentFrame = 0; m_lastAttackTime = ct; m_isAttackClicked = true; if (!air) m_hasLeapedInAir = false;
            float dx = wx - (m_x + m_colW / 2.0f), dy = wy - (m_y + m_colH / 2.0f), dist = sqrt(dx * dx + dy * dy);
            m_attackAngle = atan2(dy, dx); m_isFacingRight = (dx >= 0);
            if (dist > 0) { m_attackDirX = dx / dist; m_attackDirY = dy / dist; } else { m_attackDirX = 1.0f; m_attackDirY = 0.0f; }
            m_dashDirX = m_attackDirX; m_dashDirY = m_attackDirY;
            if (dy < 0) { if (m_canAirYDash) m_canAirYDash = false; else { m_dashDirY = 0.0f; m_dashDirX = (dx >= 0) ? 1.0f : -1.0f; } }
            float dD = (dist < m_dashRadius) ? dist : m_dashRadius; m_attackTargetX = m_x + m_dashDirX * dD; m_attackTargetY = m_y + m_dashDirY * dD;
        }
    }
    prL = curL;
    if (air && twd != 0 && m_state != PlayerState::PS_ATTACK && m_state != PlayerState::PS_ROLL && m_state != PlayerState::PS_DOOR_KICK && m_state != PlayerState::PS_DOOR_KICK_FULL) {
        if (m_state != PlayerState::PS_WALL_GRAB && m_state != PlayerState::PS_WALL_SLIDE) {
            if (((twd == -1 && isA) || (twd == 1 && isD)) || (m_state == PlayerState::PS_WALL_FLIP && twd != m_wallDir)) { m_state = PlayerState::PS_WALL_GRAB; m_currentFrame = 0; m_wallGrabTime = ct; m_wallDir = twd; m_isFacingRight = (m_wallDir == 1); m_canAirYDash = true; }
        } else { if ((m_wallDir == 1 && isA) || (m_wallDir == -1 && isD) || twd != m_wallDir) m_state = PlayerState::PS_FALL; }
    } else if (m_state == PlayerState::PS_WALL_GRAB || m_state == PlayerState::PS_WALL_SLIDE) { if (!air) m_state = PlayerState::PS_IDLE; else m_state = PlayerState::PS_FALL; }
    float tvx = 0.0f;
    if (m_state == PlayerState::PS_ATTACK) {
        float dx = m_attackTargetX - m_x, dy = m_attackTargetY - m_y; float dtt = sqrt(dx * dx + dy * dy);
        if (dtt > cDSp) { if (!CheckMapCollision(m_x + m_dashDirX * cDSp, m_y, m_colW, m_colH)) m_x += m_dashDirX * cDSp; if (!CheckMapCollision(m_x, m_y + m_dashDirY * cDSp, m_colW, m_colH)) m_y += m_dashDirY * cDSp; }
        else { m_x = m_attackTargetX; m_y = m_attackTargetY; }
        m_vy = 0.0f; m_vx = 0.0f;
    } else if (m_state == PlayerState::PS_ROLL) m_vx = m_isFacingRight ? 15.0f * ts : -15.0f * ts;
    else if (m_state == PlayerState::PS_WALL_GRAB || m_state == PlayerState::PS_WALL_SLIDE || m_state == PlayerState::PS_DOOR_KICK || m_state == PlayerState::PS_DOOR_KICK_FULL) m_vx = 0.0f;
    else if (m_state == PlayerState::PS_IDLE_TO_WALK) { if (isA) { tvx = -1.0f * ts; m_isFacingRight = false; } if (isD) { tvx = 1.0f * ts; m_isFacingRight = true; } }
    else if (m_state != PlayerState::PS_WALK_TO_IDLE && m_state != PlayerState::PS_PREVDOWN && m_state != PlayerState::PS_DOWN && m_state != PlayerState::PS_POSTDOWN) { if (isA) { tvx = -cWSp; m_isFacingRight = false; } if (isD) { tvx = cWSp; m_isFacingRight = true; } }
    if (tvx != 0.0f && m_state != PlayerState::PS_ROLL && m_state != PlayerState::PS_ATTACK && m_state != PlayerState::PS_WALL_GRAB && m_state != PlayerState::PS_WALL_SLIDE && m_state != PlayerState::PS_WALL_FLIP && !isDead) m_vx += (tvx - m_vx) * cAcc;
    else if (m_state != PlayerState::PS_ROLL && m_state != PlayerState::PS_ATTACK && m_state != PlayerState::PS_WALL_GRAB && m_state != PlayerState::PS_WALL_SLIDE && m_state != PlayerState::PS_WALL_FLIP) { 
        float curFri = isDead ? 0.02f * ts : cFri;
        m_vx += (0.0f - m_vx) * curFri; 
        if (fabs(m_vx) < 0.1f) m_vx = 0.0f; 
    }
    if (m_vx != 0.0f && m_state != PlayerState::PS_ATTACK && m_state != PlayerState::PS_WALL_GRAB && m_state != PlayerState::PS_WALL_SLIDE) {
        float nx = m_x + m_vx;
        if (!CheckMapCollision(nx, m_y, m_colW, m_colH - 5)) { m_x = nx; }
        else { bool stepped = false; for (int i = 1; i <= 16; i++) { if (!CheckMapCollision(m_x, m_y - i, m_colW, m_colH - 5) && !CheckMapCollision(nx, m_y - i, m_colW, m_colH - 5)) { m_x = nx; m_y -= (float)i; stepped = true; break; } }
            if (!stepped) { float si = (m_vx > 0) ? 1.0f : -1.0f; int f = 0; while (!CheckMapCollision(m_x + si, m_y, m_colW, m_colH - 5) && f++ < (int)fabs(m_vx) + 2) { m_x += si; } if (m_state != PlayerState::PS_WALL_FLIP) m_vx = 0.0f; }
        }
    }
    if (isS && !air && m_state != PlayerState::PS_ROLL && m_state != PlayerState::PS_ATTACK && m_state != PlayerState::PS_WALL_GRAB && m_state != PlayerState::PS_WALL_SLIDE && m_state != PlayerState::PS_WALL_FLIP) {
        if (GetCollisionType((int)m_x, (int)(m_y + m_colH + 1)) == 2 || GetCollisionType((int)(m_x + m_colW / 2), (int)(m_y + m_colH + 1)) == 2 || GetCollisionType((int)(m_x + m_colW), (int)(m_y + m_colH + 1)) == 2) { m_y += 4.0f; m_isJumping = true; m_vy = 1.0f; }
    }
    if (m_state != PlayerState::PS_ATTACK) {
        if (m_state == PlayerState::PS_WALL_GRAB) { m_vy = 0.0f; if (ct - m_wallGrabTime >= 150) { m_state = PlayerState::PS_WALL_SLIDE; m_currentFrame = 0; } }
        else { bool jhv = isJ && (ct - m_jumpHoldTimer < (DWORD)200); float cg = (jhv && m_vy < 0.0f) ? 0.45f * ts * ts : 1.0f * ts * ts; m_vy += cg;
            float mf = 30.0f * ts; if (m_state == PlayerState::PS_WALL_SLIDE && m_vy >= 0.0f) { mf = isS ? 12.0f * ts : 2.5f * ts; m_vy = mf; } else if (m_vy > mf) m_vy = mf;
        }
        float ny = m_y + m_vy;
        if (m_vy > 0) { 
            bool hf = false; 
            for (float sy = m_y; sy <= ny; sy += 1.0f) {
                int tl = GetCollisionType((int)(m_x + 2.0f), (int)(sy + m_colH));
                int tc = GetCollisionType((int)(m_x + m_colW / 2.0f), (int)(sy + m_colH));
                int tr = GetCollisionType((int)(m_x + m_colW - 2.0f), (int)(sy + m_colH)); 
                bool hit = false;
                if (tl == 1 || tl == 3 || tc == 1 || tc == 3 || tr == 1 || tr == 3) hit = true; 
                else if (tl == 2 || tc == 2 || tr == 2) { if (m_y + m_colH <= sy + m_colH) hit = true; }
                if (hit) { m_y = sy; hf = true; break; }
            }
            if (hf) { 
                if (isDead) { m_state = PlayerState::PS_DEAD_GROUND; m_currentFrame = 0; m_vx = 0; }
                if (air && !m_isSlowMo) EffectManager::AddLandCloudVFX(m_x + m_colW / 2.0f, m_y + m_colH, ct); m_isJumping = false; m_vy = 0; m_canAirYDash = true; m_hasLeapedInAir = false; 
            } else { 
                m_y = ny; 
                int tl2 = GetCollisionType((int)(m_x + 2.0f), (int)(m_y + 1.0f + m_colH));
                int tc2 = GetCollisionType((int)(m_x + m_colW / 2.0f), (int)(m_y + 1.0f + m_colH));
                int tr2 = GetCollisionType((int)(m_x + m_colW - 2.0f), (int)(m_y + 1.0f + m_colH));
                bool hit2 = false;
                if (tl2 == 1 || tl2 == 3 || tc2 == 1 || tc2 == 3 || tr2 == 1 || tr2 == 3) hit2 = true;
                else if (tl2 == 2 || tc2 == 2 || tr2 == 2) { if (m_y + m_colH <= m_y + 1.0f + m_colH) hit2 = true; }
                if (!hit2) m_isJumping = true; else { m_isJumping = false; m_canAirYDash = true; } 
            }
        }
 else if (m_vy < 0) { 
            bool hc = false; 
            for (float sy = m_y; sy >= ny; sy -= 1.0f) {
                if (GetCollisionType((int)(m_x + 2.0f), (int)sy) % 2 != 0 || GetCollisionType((int)(m_x + m_colW / 2.0f), (int)sy) % 2 != 0 || GetCollisionType((int)(m_x + m_colW - 2.0f), (int)sy) % 2 != 0) {
                    m_y = sy; hc = true; break;
                }
            }
            if (hc) m_vy = 0; else m_y = ny;
        }
        int ml = StageManager::GetMap().IsNull() ? 720 : StageManager::GetMap().GetHeight(); 
        if (m_y >= ml) { if (!m_isGodMode) { if (m_state != PlayerState::PS_PIT_DEATH) { m_state = PlayerState::PS_PIT_DEATH; m_vx = 0; } } }
    }
    if (isDead && m_state != PlayerState::PS_PIT_DEATH) return;
    PlayerState nst = m_state;
    if (m_state == PlayerState::PS_ATTACK && m_currentFrame >= 5) nst = air ? PlayerState::PS_FALL : PlayerState::PS_IDLE;
    else if (m_state == PlayerState::PS_DOOR_KICK && m_currentFrame >= 5) nst = air ? PlayerState::PS_FALL : PlayerState::PS_IDLE;
    else if (m_state == PlayerState::PS_DOOR_KICK_FULL && m_currentFrame >= 9) nst = air ? PlayerState::PS_FALL : PlayerState::PS_IDLE;
    else if (m_state == PlayerState::PS_WALL_FLIP && m_currentFrame >= 10) nst = air ? (m_vy < 0.0f ? PlayerState::PS_JUMP_UP : PlayerState::PS_FALL) : PlayerState::PS_IDLE;
    else if (m_state == PlayerState::PS_ROLL && m_currentFrame >= 6) nst = (isA || isD) ? PlayerState::PS_WALK : (isS ? PlayerState::PS_DOWN : PlayerState::PS_IDLE);
    else if (m_state == PlayerState::PS_PREVDOWN && m_currentFrame >= 2) nst = PlayerState::PS_DOWN;
    else if (m_state == PlayerState::PS_POSTDOWN && m_currentFrame >= 2) nst = PlayerState::PS_IDLE;
    if (!isS) { m_canRoll = true; if (nst == PlayerState::PS_PREVDOWN || nst == PlayerState::PS_DOWN) nst = PlayerState::PS_POSTDOWN; }
    else {
        if (m_canRoll && (isA || isD) && !air && nst != PlayerState::PS_ROLL && nst != PlayerState::PS_ATTACK && nst != PlayerState::PS_WALL_GRAB && nst != PlayerState::PS_WALL_SLIDE && nst != PlayerState::PS_WALL_FLIP && nst != PlayerState::PS_DOOR_KICK && nst != PlayerState::PS_DOOR_KICK_FULL) { nst = PlayerState::PS_ROLL; m_canRoll = false; m_isFacingRight = isD; }
        else if (!m_canRoll && (isA || isD) && !air && nst != PlayerState::PS_ROLL && nst != PlayerState::PS_ATTACK && nst != PlayerState::PS_WALL_GRAB && nst != PlayerState::PS_WALL_SLIDE && nst != PlayerState::PS_WALL_FLIP && nst != PlayerState::PS_DOOR_KICK && nst != PlayerState::PS_DOOR_KICK_FULL) nst = PlayerState::PS_WALK;
        else if (!(isA || isD) && !air && nst != PlayerState::PS_ROLL && nst != PlayerState::PS_ATTACK && nst != PlayerState::PS_PREVDOWN && nst != PlayerState::PS_DOWN && nst != PlayerState::PS_POSTDOWN && nst != PlayerState::PS_WALL_GRAB && nst != PlayerState::PS_WALL_SLIDE && nst != PlayerState::PS_WALL_FLIP && nst != PlayerState::PS_DOOR_KICK && nst != PlayerState::PS_DOOR_KICK_FULL) nst = PlayerState::PS_PREVDOWN;
        else if (!(isA || isD) && nst == PlayerState::PS_POSTDOWN) nst = PlayerState::PS_PREVDOWN;
    }
    if (nst != PlayerState::PS_ROLL && nst != PlayerState::PS_ATTACK && nst != PlayerState::PS_PREVDOWN && nst != PlayerState::PS_DOWN && nst != PlayerState::PS_POSTDOWN && nst != PlayerState::PS_WALL_GRAB && nst != PlayerState::PS_WALL_SLIDE && nst != PlayerState::PS_WALL_FLIP && nst != PlayerState::PS_DOOR_KICK && nst != PlayerState::PS_DOOR_KICK_FULL && nst != PlayerState::PS_PIT_DEATH) {
        if (air) nst = (m_vy < 0.0f) ? PlayerState::PS_JUMP_UP : PlayerState::PS_FALL;
        else { if (isA || isD) nst = (m_state == PlayerState::PS_IDLE_TO_WALK) ? (m_currentFrame >= 3 ? PlayerState::PS_WALK : PlayerState::PS_IDLE_TO_WALK) : ((m_state == PlayerState::PS_WALK || m_state == PlayerState::PS_RUN) ? PlayerState::PS_WALK : PlayerState::PS_IDLE_TO_WALK);
            else nst = (m_state == PlayerState::PS_WALK_TO_IDLE) ? (m_currentFrame >= 4 ? PlayerState::PS_IDLE : PlayerState::PS_WALK_TO_IDLE) : ((m_state == PlayerState::PS_WALK || m_state == PlayerState::PS_RUN || m_state == PlayerState::PS_IDLE_TO_WALK || m_state == PlayerState::PS_FALL) ? PlayerState::PS_WALK_TO_IDLE : PlayerState::PS_IDLE);
        }
    } else if (!air && (nst == PlayerState::PS_WALL_FLIP || nst == PlayerState::PS_WALL_SLIDE || nst == PlayerState::PS_WALL_GRAB)) nst = (isA || isD) ? PlayerState::PS_IDLE_TO_WALK : PlayerState::PS_WALK_TO_IDLE;
    if (m_state != nst) { m_currentFrame = 0; m_state = nst; }
    if (!air && (m_state == PlayerState::PS_WALK || m_state == PlayerState::PS_RUN) && !m_isSlowMo) { static bool lfr = m_isFacingRight; if (!m_wasMoving || (m_isFacingRight != lfr)) { for (int i = 0; i < 3; i++) EffectManager::AddDustCloudVFX(m_x + (m_isFacingRight ? 0 : m_colW) + (m_isFacingRight ? m_dustOffsetX[i] : -m_dustOffsetX[i]), m_y + m_colH + m_dustOffsetY[i], m_isFacingRight, ct); } lfr = m_isFacingRight; }
    m_wasMoving = !air && (m_state == PlayerState::PS_WALK || m_state == PlayerState::PS_RUN);
    if (m_state == PlayerState::PS_ROLL && !air && !m_isSlowMo) { if (m_currentFrame != m_lastRollFrame) { int cnts[] = { 1, 1, 2, 2, 3, 4 }; int safeFrameForCnt = (m_currentFrame < 5) ? m_currentFrame : 5; int c = cnts[safeFrameForCnt]; for (int i = 0; i < c; i++) EffectManager::AddDustCloudVFX(m_x + (m_isFacingRight ? 0 : m_colW) + (float)(rand() % 21 - 10), m_y + m_colH + (float)(rand() % 11 - 5) + 2.0f, m_isFacingRight, ct); m_lastRollFrame = m_currentFrame; } } else m_lastRollFrame = -1;
    if (m_state == PlayerState::PS_WALL_SLIDE && m_vy > 0.0f && !m_isSlowMo) { static DWORD lwdt = 0; if (ct - lwdt >= 150) { for (int i = 0; i < 2; i++) EffectManager::AddDustCloudVFX(m_x + (m_wallDir == 1 ? m_colW : 0) + (float)(rand() % 11 - 5), m_y + m_colH + (float)(rand() % 11 - 5), m_wallDir == -1, ct); lwdt = ct; } }
    bool leap = false; if (m_state == PlayerState::PS_ATTACK) { float dx = m_attackTargetX - m_x, dy = m_attackTargetY - m_y; if (sqrt(dx * dx + dy * dy) > 1.0f) { if (!m_hasLeapedInAir && m_isAttackClicked && m_currentFrame == 0) { leap = true; m_hasLeapedInAir = true; } } if (m_currentFrame >= 1) m_isAttackClicked = false; }
    if (leap || m_state == PlayerState::PS_ROLL || m_state == PlayerState::PS_WALL_FLIP || m_state == PlayerState::PS_ATTACK || m_isSlowMo) { 
        DWORD iv = m_isSlowMo ? (DWORD)30 : (DWORD)1; 
        if (ct - m_lastAfterImageTime >= iv) { 
            for (int i = (int)m_afterImages.size() - 1; i > 0; i--) m_afterImages[i] = m_afterImages[i - 1]; 
            AfterImageData ad;
            ad.x = m_x; ad.y = m_y; ad.state = m_state; ad.frame = m_currentFrame; 
            ad.isFacingRight = m_isFacingRight; ad.attackAngle = m_attackAngle; ad.active = true;
            m_afterImages[0] = ad;
            m_lastAfterImageTime = ct; 
        } 
    }
    else { if (ct - m_lastAfterImageTime >= (DWORD)1) { for (int i = (int)m_afterImages.size() - 1; i > 0; i--) m_afterImages[i] = m_afterImages[i - 1]; m_afterImages[0].active = false; m_lastAfterImageTime = ct; } }
}

void Player::UpdateAnimation() {
    static DWORD lt_ani = GetTickCount(); DWORD ct = GetTickCount(); DWORD d = 150;
    switch (m_state) {
    case PlayerState::PS_IDLE: d = 150; break; 
    case PlayerState::PS_IDLE_TO_WALK: d = 60; break; 
    case PlayerState::PS_WALK: d = 80; break; 
    case PlayerState::PS_WALK_TO_IDLE: d = 60; break; 
    case PlayerState::PS_RUN: d = 80; break; 
    case PlayerState::PS_JUMP_UP: case PlayerState::PS_FALL: d = 100; break; 
    case PlayerState::PS_PREVDOWN: case PlayerState::PS_DOWN: case PlayerState::PS_POSTDOWN: d = 80; break; 
    case PlayerState::PS_ROLL: d = 50; break; 
    case PlayerState::PS_ATTACK: d = 40; break; 
    case PlayerState::PS_WALL_GRAB: d = 80; break; 
    case PlayerState::PS_WALL_SLIDE: d = 100; break; 
    case PlayerState::PS_WALL_FLIP: d = 40; break; 
    case PlayerState::PS_DOOR_KICK: d = 80; break; 
    case PlayerState::PS_DOOR_KICK_FULL: d = 80; break;
    case PlayerState::PS_DEAD_FLY_BEGIN: d = 60; break;
    case PlayerState::PS_DEAD_FLY_LOOP: d = 60; break;
    case PlayerState::PS_DEAD_GROUND: d = 60; break;
    }
    if (m_isSlowMo) d = (DWORD)(d / 0.3f);
    if (ct - lt_ani >= d) {
        m_currentFrame++; lt_ani = ct;
        if (m_state == PlayerState::PS_IDLE && m_currentFrame >= 11) m_currentFrame = 0;
        if (m_state == PlayerState::PS_IDLE_TO_WALK && m_currentFrame >= 4) m_currentFrame = 3;
        if (m_state == PlayerState::PS_WALK && m_currentFrame >= 10) m_currentFrame = 0;
        if (m_state == PlayerState::PS_WALK_TO_IDLE && m_currentFrame >= 5) m_currentFrame = 4;
        if (m_state == PlayerState::PS_RUN && m_currentFrame >= 10) m_currentFrame = 0;
        if (m_state == PlayerState::PS_JUMP_UP && m_currentFrame >= 4) m_currentFrame = 3;
        if (m_state == PlayerState::PS_FALL && m_currentFrame >= 4) m_currentFrame = 3;
        if (m_state == PlayerState::PS_DOWN && m_currentFrame >= 1) m_currentFrame = 0;
        if (m_state == PlayerState::PS_ATTACK && m_currentFrame >= 5) m_currentFrame = 5;
        if (m_state == PlayerState::PS_WALL_GRAB && m_currentFrame >= 2) m_currentFrame = 1;
        if (m_state == PlayerState::PS_WALL_SLIDE && m_currentFrame >= 1) m_currentFrame = 0;
        if (m_state == PlayerState::PS_WALL_FLIP && m_currentFrame >= 11) m_currentFrame = 10;
        if (m_state == PlayerState::PS_DOOR_KICK && m_currentFrame >= 6) m_currentFrame = 5;
        if (m_state == PlayerState::PS_DOOR_KICK_FULL && m_currentFrame >= 10) m_currentFrame = 9;
        if (m_state == PlayerState::PS_DEAD_FLY_BEGIN && m_currentFrame >= 2) { m_state = PlayerState::PS_DEAD_FLY_LOOP; m_currentFrame = 0; }
        if (m_state == PlayerState::PS_DEAD_FLY_LOOP && m_currentFrame >= 4) m_currentFrame = 0;
        if (m_state == PlayerState::PS_DEAD_GROUND && m_currentFrame >= 6) m_currentFrame = 5;
    }
}

void Player::Render(HDC hMemDC, Gdiplus::Graphics* g, float camX, float camY, float mapScale, float playerScale, float g_renderMapScale, float g_mapOffsetX, float g_mapOffsetY, bool g_isFullMapView, bool g_showDebugRect, float stageTimer) {
    if (!hMemDC || !g) return;
    int mapW = StageManager::GetMapWidth();
    int mapH = StageManager::GetMapHeight();
    float pFS = mapScale;
    std::map<CImage*, Gdiplus::Bitmap*> bmpCache;

    for (int i = (int)m_afterImages.size() - 1; i >= -1; i--) {
        CImage* img = NULL; float px, py; PlayerState s; int f; bool fac; float a;
        if (i >= 0) { 
            if (!m_afterImages[i].active) continue; 
            px = m_afterImages[i].x; py = m_afterImages[i].y; s = m_afterImages[i].state; f = m_afterImages[i].frame; fac = m_afterImages[i].isFacingRight; a = m_afterImages[i].attackAngle; 
        } else { 
            px = m_x; py = m_y; s = m_state; f = m_currentFrame; fac = m_isFacingRight; a = m_attackAngle; 
        }
        
        int safeF = (f < 0) ? 0 : f;
        switch (s) {
        case PlayerState::PS_IDLE: img = &imgIdle[(safeF > 10) ? 10 : safeF]; break; 
        case PlayerState::PS_IDLE_TO_WALK: img = &imgIdleToWalk[(safeF > 3) ? 3 : safeF]; break; 
        case PlayerState::PS_WALK: img = &imgWalk[(safeF > 9) ? 9 : safeF]; break; 
        case PlayerState::PS_WALK_TO_IDLE: img = &imgWalkToIdle[(safeF > 4) ? 4 : safeF]; break; 
        case PlayerState::PS_RUN: img = &imgRun[(safeF > 9) ? 9 : safeF]; break; 
        case PlayerState::PS_JUMP_UP: img = &imgJumpUp[(safeF > 3) ? 3 : safeF]; break; 
        case PlayerState::PS_FALL: img = &imgFall[(safeF > 3) ? 3 : safeF]; break; 
        case PlayerState::PS_PREVDOWN: img = &imgPrevDown[(safeF > 1) ? 1 : safeF]; break; 
        case PlayerState::PS_DOWN: img = &imgDown[0]; break; 
        case PlayerState::PS_POSTDOWN: img = &imgPostDown[(safeF > 1) ? 1 : safeF]; break; 
        case PlayerState::PS_ROLL: img = &imgRoll[(safeF > 5) ? 5 : safeF]; break; 
        case PlayerState::PS_ATTACK: img = &imgAttack[(safeF > 6) ? 6 : safeF]; break; 
        case PlayerState::PS_WALL_GRAB: img = &imgWallGrab[(safeF > 1) ? 1 : safeF]; break; 
        case PlayerState::PS_WALL_SLIDE: img = &imgWallSlide[0]; break; 
        case PlayerState::PS_WALL_FLIP: img = &imgWallFlip[(safeF > 10) ? 10 : safeF]; break; 
        case PlayerState::PS_DOOR_KICK: img = &imgDoorKick[(safeF > 5) ? 5 : safeF]; break; 
        case PlayerState::PS_DOOR_KICK_FULL: img = &imgDoorKickFull[(safeF > 9) ? 9 : safeF]; break;
        case PlayerState::PS_DEAD_FLY_BEGIN: img = &imgHurtFlyBegin[(safeF > 1) ? 1 : safeF]; break;
        case PlayerState::PS_DEAD_FLY_LOOP: img = &imgHurtFlyLoop[(safeF > 3) ? 3 : safeF]; break; 
        case PlayerState::PS_DEAD_GROUND: img = &imgHurtGround[(safeF > 5) ? 5 : safeF]; break; 
        case PlayerState::PS_PIT_DEATH: img = NULL; break; 
        }

        if (img && !img->IsNull() && img->IsDIBSection() && img->GetWidth() > 0 && img->GetHeight() > 0) {
            float vx, vy; 
            if (g_isFullMapView) { 
                float fsW = 1280.0f / (float)(mapW > 0 ? mapW : 1);
                float fsH = 720.0f / (float)(mapH > 0 ? mapH : 1);
                float fs = (fsW < fsH) ? fsW : fsH;
                vx = px * fs + (1280.0f - mapW * fs) / 2.0f; 
                vy = py * fs + (720.0f - mapH * fs) / 2.0f; 
                pFS = fs; 
            } else { 
                vx = (px - camX) * mapScale; 
                vy = (py - camY) * mapScale; 
                pFS = mapScale; 
            }
            float sw = img->GetWidth() * playerScale * pFS;
            float sh = img->GetHeight() * playerScale * pFS;
            float dx = vx + (40.0f * pFS) / 2.0f - (sw / 2.0f);
            float dy = vy + (64.0f * pFS) - sh;
            if (sw <= 0 || sh <= 0) continue;

            if (i == -1 && !m_isSlowMo && stageTimer > 3.0f && fac) {
                img->Draw(hMemDC, (int)dx, (int)dy, (int)sw, (int)sh);
            } else {
                if (bmpCache.find(img) == bmpCache.end()) {
                    void* bits = img->GetBits(); 
                    if (bits) {
                        bmpCache[img] = new Gdiplus::Bitmap(img->GetWidth(), img->GetHeight(), img->GetPitch(), PixelFormat32bppARGB, (BYTE*)bits);
                    }
                }
                Gdiplus::Bitmap* pBmp = bmpCache[img]; 
                if (pBmp) {
                    Gdiplus::ImageAttributes at; 
                    Gdiplus::ColorMatrix mat;
                    if (i == -1) { 
                        if (m_isSlowMo) {
                            float fm[25] = { 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,1,1,1,0, 0,0,0,0,1 };
                            memcpy(&mat, fm, sizeof(mat));
                        } else if (stageTimer <= 3.0f && !IsDeathAnimationFinished()) {
                            float alphaRatio = 1.0f - (stageTimer / 3.0f); 
                            if (alphaRatio < 0) alphaRatio = 0; if (alphaRatio > 1.0f) alphaRatio = 1.0f;
                            float r = 1.0f, g = 1.0f - alphaRatio * 0.8f, b = 1.0f - alphaRatio * 0.2f;
                            float fm[25] = { r,0,0,0,0, 0,g,0,0,0, 0,0,b,0,0, 0,0,0,1,0, 0,0,0,0,1 };
                            memcpy(&mat, fm, sizeof(mat));
                        } else {
                            float fm[25] = { 1,0,0,0,0, 0,1,0,0,0, 0,0,1,0,0, 0,0,0,1,0, 0,0,0,0,1 };
                            memcpy(&mat, fm, sizeof(mat));
                        }
                    } else {
                        float al = (1.0f - ((float)i / (float)m_afterImages.size())); 
                        if (m_isSlowMo) { 
                            al *= 0.4f; 
                            float fm[25] = { 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,1,1,al,0, 0,0,0,0,1 };
                            memcpy(&mat, fm, sizeof(mat));
                        } else { 
                            if (i % 2 == 0) { 
                                al *= 0.8f; 
                                float fm[25] = { 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,1,1,al,0, 0,0,0,0,1 };
                                memcpy(&mat, fm, sizeof(mat));
                            } else { 
                                al *= 0.6f; 
                                float fm[25] = { 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 1,0,1,al,0, 0,0,0,0,1 };
                                memcpy(&mat, fm, sizeof(mat));
                            } 
                        }
                    }
                    at.SetColorMatrix(&mat, Gdiplus::ColorMatrixFlagsDefault, Gdiplus::ColorAdjustTypeBitmap);
                    if (fac) {
                        g->DrawImage(pBmp, Gdiplus::RectF(dx, dy, sw, sh), 0, 0, (float)img->GetWidth(), (float)img->GetHeight(), Gdiplus::UnitPixel, &at);
                    } else { 
                        g->ScaleTransform(-1.0f, 1.0f); 
                        g->TranslateTransform(-(dx * 2 + sw), 0); 
                        g->DrawImage(pBmp, Gdiplus::RectF(dx, dy, sw, sh), 0, 0, (float)img->GetWidth(), (float)img->GetHeight(), Gdiplus::UnitPixel, &at); 
                        g->ResetTransform(); 
                    }
                }
            }

            if (i == -1 && s == PlayerState::PS_ATTACK && f < 5) {
                int sIdx = (safeF > 4) ? 4 : safeF;
                CImage* si = &imgSlashFX[sIdx]; 
                if (si && !si->IsNull()) {
                    if (si->IsDIBSection()) {
                        void* sbits = si->GetBits(); 
                        if (sbits) {
                            Gdiplus::Bitmap gs(si->GetWidth(), si->GetHeight(), si->GetPitch(), PixelFormat32bppARGB, (BYTE*)sbits);
                            Gdiplus::ImageAttributes sat; 
                            Gdiplus::ColorMatrix smat;
                            float fm[25] = { 1,0,0,0,0, 0,1,0,0,0, 0,0,1,0,0, 0,0,0,1,0, 0,0,0,0,1 };
                            memcpy(&smat, fm, sizeof(smat));
                            sat.SetColorMatrix(&smat);
                            g->TranslateTransform(vx + (40.0f * pFS) / 2.0f, vy + (64.0f * pFS) / 2.0f); 
                            g->RotateTransform(a * 180.0f / 3.14159f);
                            g->DrawImage(&gs, Gdiplus::RectF(-(si->GetWidth() * playerScale * pFS) / 2.0f, -(si->GetHeight() * playerScale * pFS) / 2.0f, (float)(si->GetWidth() * playerScale * pFS), (float)(si->GetHeight() * playerScale * pFS)), 0, 0, (float)si->GetWidth(), (float)si->GetHeight(), Gdiplus::UnitPixel, &sat); 
                            g->ResetTransform();
                        }
                    }
                }
            }
        }
    }

    if (!bmpCache.empty()) {
        std::map<CImage*, Gdiplus::Bitmap*>::iterator it_render;
        for (it_render = bmpCache.begin(); it_render != bmpCache.end(); ++it_render) {
            delete it_render->second;
        }
        bmpCache.clear();
    }

    // 아이템 획득 팝업 렌더링
    if (m_itemPopupTimer > 0) {
        // 모든 아이템에 대해 인덱스 1 이미지를 사용
        int imgIndex = 1;
        
        CImage& icon = Item::GetItemImage(m_popupItemType, imgIndex);
        if (!icon.IsNull()) {
            float vx, vy;
            if (g_isFullMapView) {
                float fsW = 1280.0f / (float)(mapW > 0 ? mapW : 1);
                float fsH = 720.0f / (float)(mapH > 0 ? mapH : 1);
                float fs = (fsW < fsH) ? fsW : fsH;
                vx = m_x * fs + (1280.0f - mapW * fs) / 2.0f;
                vy = m_y * fs + (720.0f - mapH * fs) / 2.0f;
                pFS = fs;
            } else {
                vx = (m_x - camX) * mapScale;
                vy = (m_y - camY) * mapScale;
                pFS = mapScale;
            }

            int iw = (int)(icon.GetWidth() * pFS * 1.2f);
            int ih = (int)(icon.GetHeight() * pFS * 1.2f);
            int ix = (int)(vx + (40.0f * pFS) / 2.0f - iw / 2.0f);
            
            // 둥실거리는 효과와 서서히 위로 올라가는 효과
            float upOffset = (1.0f - m_itemPopupTimer) * 30.0f; 
            int iy = (int)(vy - ih - 20.0f * pFS - upOffset);

            void* bits = icon.GetBits();
            if (bits) {
                Gdiplus::Bitmap bmp(icon.GetWidth(), icon.GetHeight(), icon.GetPitch(), PixelFormat32bppARGB, (BYTE*)bits);
                Gdiplus::ImageAttributes at;
                float alpha = (m_itemPopupTimer > 0.8f) ? 1.0f : m_itemPopupTimer / 0.8f;
                Gdiplus::ColorMatrix mat = {
                    1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                    0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
                    0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                    0.0f, 0.0f, 0.0f, alpha, 0.0f,
                    0.0f, 0.0f, 0.0f, 0.0f, 1.0f
                };
                at.SetColorMatrix(&mat);
                g->DrawImage(&bmp, Gdiplus::RectF((float)ix, (float)iy, (float)iw, (float)ih), 0, 0, (float)icon.GetWidth(), (float)icon.GetHeight(), Gdiplus::UnitPixel, &at);
            }
        }
    }

    if (g_showDebugRect) {
        float vx, vy; 
        if (g_isFullMapView) { 
            float fsW = 1280.0f / (float)(mapW > 0 ? mapW : 1);
            float fsH = 720.0f / (float)(mapH > 0 ? mapH : 1);
            float fs = (fsW < fsH) ? fsW : fsH;
            vx = m_x * fs + (1280.0f - mapW * fs) / 2.0f; 
            vy = m_y * fs + (720.0f - mapH * fs) / 2.0f; 
            pFS = fs; 
        } else { 
            vx = (m_x - camX) * mapScale; 
            vy = (m_y - camY) * mapScale; 
            pFS = mapScale; 
        }
        HBRUSH gb = CreateSolidBrush(RGB(0, 255, 0)); 
        RECT pr;
        pr.left = (int)vx; pr.top = (int)vy; pr.right = (int)(vx + 40.0f * pFS); pr.bottom = (int)(vy + 64.0f * pFS);
        FrameRect(hMemDC, &pr, gb); 
        DeleteObject(gb);
        
        if (m_state == PlayerState::PS_ATTACK) {
            float hw = 80.0f * pFS;
            float hh = 60.0f * pFS;
            float hx = vx + (40.0f * pFS) / 2.0f + m_attackDirX * 40.0f * pFS - hw / 2.0f;
            float hy = vy + (64.0f * pFS) / 2.0f + m_attackDirY * 40.0f * pFS - hh / 2.0f;
            HBRUSH rb = CreateSolidBrush(RGB(255, 0, 0)); 
            RECT ar;
            ar.left = (int)hx; ar.top = (int)hy; ar.right = (int)(hx + hw); ar.bottom = (int)(hy + hh);
            FrameRect(hMemDC, &ar, rb); 
            DeleteObject(rb);
        }
    }
}

void Player::SetState(PlayerState state) { 
    if (m_state != state) { 
        m_state = state; 
        m_currentFrame = 0; 
        if (state == PlayerState::PS_DEAD) {
            m_state = PlayerState::PS_DEAD_FLY_BEGIN;
            // Removed default vx/vy setting to let OnTakeDamage handle it
        }
    } 
}

bool Player::IsDead() const { 
    return m_state == PlayerState::PS_DEAD || m_state == PlayerState::PS_DEAD_FLY_BEGIN || 
           m_state == PlayerState::PS_DEAD_FLY_LOOP || m_state == PlayerState::PS_DEAD_GROUND || 
           m_state == PlayerState::PS_PIT_DEATH; 
}

bool Player::IsDeathAnimationFinished() const { 
    if (m_state == PlayerState::PS_PIT_DEATH) {
        int ml = StageManager::GetMap().IsNull() ? 720 : StageManager::GetMap().GetHeight();
        return m_y > ml + 100.0f; 
    }
    return m_state == PlayerState::PS_DEAD_GROUND && m_currentFrame >= 5; 
}

void Player::PickUpItem(Item* item) {
    if (m_pHeldItem) {
        // 기존 아이템 버리기 (삭제 또는 맵에 드롭)
        delete m_pHeldItem;
        m_pHeldItem = nullptr;
    }
    m_pHeldItem = item;
    item->OnPickUp();

    // 아이템 획득 팝업 설정
    m_itemPopupTimer = 1.0f; // 1초 동안 표시
    m_popupItemType = item->GetType();
}

#include "../Core/Input.h"
#include "../SceneAndMap/Camera.h"
// ... (rest of includes)

void Player::ThrowItem(int mouseX, int mouseY, float camX, float camY, float rs, float ox, float oy, bool fv) {
    if (!m_pHeldItem) return;
    
    // 마우스 위치 계산 (카메라 오프셋 고려)
    float wx = (mouseX - ox) / rs, wy = (mouseY - oy) / rs;
    if (!fv) { wx += camX; wy += camY; }
    
    // 던지는 시작 위치 (플레이어 중심에서 약간 오프셋을 주어 즉시 충돌 방지)
    float spawnX = m_x + m_colW / 2.0f + (m_isFacingRight ? 20.0f : -20.0f);
    float spawnY = m_y + m_colH / 2.0f;
    
    float dirX = wx - spawnX;
    float dirY = wy - spawnY;
    float dist = std::sqrt(dirX * dirX + dirY * dirY);
    
    float vx = 0, vy = 0;
    if (dist > 1.0f) {
        vx = (dirX / dist) * 25.0f;
        vy = (dirY / dist) * 25.0f;
    } else {
        vx = m_isFacingRight ? 25.0f : -25.0f;
        vy = -8.0f;
    }
    
    // 먼저 아이템을 리스트에 추가
    auto items = StageManager::GetCurrentItems();
    if (items) {
        items->push_back(*m_pHeldItem);
        // 리스트에 추가된 아이템의 상태를 직접 수정하여 복사 문제 방지
        items->back().OnThrow(spawnX, spawnY, vx, vy);
    }
    
    delete m_pHeldItem; 
    m_pHeldItem = nullptr;
}

void Player::OnTakeDamage(float damage, float kvx, float kvy) {
    if (!m_isGodMode && !IsDead() && m_state != PlayerState::PS_ROLL) {
        if (m_pHeldItem) {
            // Drop item when taking damage
            m_pHeldItem = nullptr;
        }
        SetState(PlayerState::PS_DEAD);
        if (kvx != 0.0f || kvy != 0.0f) {
            m_vx = kvx;
            m_vy = kvy;
        }
        m_bloodDistance = 0.0f;
    }
}

void Player::AddReplayEvent(ReplayEvent type, int idx) {
    if (!m_snapshots.empty()) {
        m_snapshots.back().events.push_back({ type, idx });
    }
}

void Player::ClearAfterImages() {
    for (size_t i = 0; i < m_afterImages.size(); i++) m_afterImages[i].active = false;
}

void Player::StartRewind(int speed) { 
    m_isRewinding = true; 
    m_rewindSpeed = speed; 
    m_isSlowMo = false; 
    m_batteryLevel = m_batteryMax; 
    ClearAfterImages();
}

void Player::RenderSilhouette(Gdiplus::Graphics* g, float camX, float camY, float mapScale) {
    if (!EffectManager::IsInsideSmoke(m_x + m_colW / 2.0f, m_y + m_colH / 2.0f)) return;
    CImage* img = nullptr; int safeF = (m_currentFrame < 0) ? 0 : m_currentFrame;
    switch (m_state) {
        case PlayerState::PS_IDLE: img = &imgIdle[(safeF > 10) ? 10 : safeF]; break; case PlayerState::PS_IDLE_TO_WALK: img = &imgIdleToWalk[(safeF > 3) ? 3 : safeF]; break; case PlayerState::PS_WALK: img = &imgWalk[(safeF > 9) ? 9 : safeF]; break; case PlayerState::PS_WALK_TO_IDLE: img = &imgWalkToIdle[(safeF > 4) ? 4 : safeF]; break; case PlayerState::PS_RUN: img = &imgRun[(safeF > 9) ? 9 : safeF]; break; case PlayerState::PS_JUMP_UP: img = &imgJumpUp[(safeF > 3) ? 3 : safeF]; break; case PlayerState::PS_FALL: img = &imgFall[(safeF > 3) ? 3 : safeF]; break; case PlayerState::PS_ROLL: img = &imgRoll[(safeF > 5) ? 5 : safeF]; break; case PlayerState::PS_ATTACK: img = &imgAttack[(safeF > 6) ? 6 : safeF]; break; case PlayerState::PS_DEAD_FLY_BEGIN: img = &imgHurtFlyBegin[(safeF > 1) ? 1 : safeF]; break; case PlayerState::PS_DEAD_FLY_LOOP: img = &imgHurtFlyLoop[(safeF > 3) ? 3 : safeF]; break; case PlayerState::PS_DEAD_GROUND: img = &imgHurtGround[(safeF > 5) ? 5 : safeF]; break; default: img = &imgIdle[0]; break;
    }
    if (img && !img->IsNull()) {
        float vx = (m_x - camX) * mapScale, vy = (m_y - camY) * mapScale; float sw = img->GetWidth() * 2.0f * mapScale, sh = img->GetHeight() * 2.0f * mapScale; float dx = vx + (40.0f * mapScale) / 2.0f - (sw / 2.0f), dy = vy + (64.0f * mapScale) - sh;
        Gdiplus::Bitmap bmp(img->GetWidth(), img->GetHeight(), img->GetPitch(), PixelFormat32bppARGB, (BYTE*)img->GetBits()); Gdiplus::ImageAttributes attr; Gdiplus::ColorMatrix cm = { 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0.5f,0, 0,0,0,0,1.0f }; attr.SetColorMatrix(&cm);
        if (m_isFacingRight) g->DrawImage(&bmp, Gdiplus::RectF(dx, dy, sw, sh), 0, 0, (float)img->GetWidth(), (float)img->GetHeight(), Gdiplus::UnitPixel, &attr);
        else { g->ScaleTransform(-1.0f, 1.0f); g->TranslateTransform(-(dx * 2 + sw), 0); g->DrawImage(&bmp, Gdiplus::RectF(dx, dy, sw, sh), 0, 0, (float)img->GetWidth(), (float)img->GetHeight(), Gdiplus::UnitPixel, &attr); g->ResetTransform(); }
    }
}
