#include "Player.h"
#include "Physics.h"
#include "../SceneAndMap/StageManager.h"
#include "../Effects/EffectManager.h"
#include <objidl.h>
#include <gdiplus.h>

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
    m_state = PlayerState::IDLE; m_isJumping = false; m_isFacingRight = true;
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
}

Player::~Player() {}

void Player::Init() {
    TCHAR path[256];
    for (int i = 0; i < 11; i++) { wsprintf(path, TEXT("assets/idle/%d.png"), i); imgIdle[i].Load(path); }
    for (int i = 0; i < 10; i++) { wsprintf(path, TEXT("assets/walk/%d.png"), i); imgWalk[i].Load(path); }
    for (int i = 0; i < 10; i++) { wsprintf(path, TEXT("assets/run/%d.png"), i); imgRun[i].Load(path); }
    for (int i = 0; i < 4; i++) { wsprintf(path, TEXT("assets/jump/%d.png"), i); imgJumpUp[i].Load(path); }
    for (int i = 0; i < 4; i++) { wsprintf(path, TEXT("assets/fall/%d.png"), i); imgFall[i].Load(path); }
    for (int i = 0; i < 2; i++) { wsprintf(path, TEXT("assets/prevdown/%d.png"), i); imgPrevDown[i].Load(path); }
    for (int i = 0; i < 1; i++) { wsprintf(path, TEXT("assets/down/%d.png"), i); imgDown[i].Load(path); }
    for (int i = 0; i < 2; i++) { wsprintf(path, TEXT("assets/postdown/%d.png"), i); imgPostDown[i].Load(path); }
    for (int i = 0; i < 6; i++) { wsprintf(path, TEXT("assets/roll/%d.png"), i); imgRoll[i].Load(path); }
    for (int i = 0; i < 2; i++) { wsprintf(path, TEXT("assets/wallgrab/%d.png"), i); imgWallGrab[i].Load(path); }
    for (int i = 0; i < 1; i++) { wsprintf(path, TEXT("assets/wallslide/%d.png"), i); imgWallSlide[i].Load(path); }
    for (int i = 0; i < 11; i++) { wsprintf(path, TEXT("assets/wallflip/%d.png"), i); imgWallFlip[i].Load(path); }
    for (int i = 0; i < 4; i++) { wsprintf(path, TEXT("assets/idletowalk/%d.png"), i); imgIdleToWalk[i].Load(path); }
    for (int i = 0; i < 5; i++) { wsprintf(path, TEXT("assets/walktoidle/%d.png"), i); imgWalkToIdle[i].Load(path); }
    for (int i = 0; i < 7; i++) { wsprintf(path, TEXT("assets/attack/%d.png"), i); imgAttack[i].Load(path); }
    for (int i = 0; i < 5; i++) { wsprintf(path, TEXT("assets/slash/%d.png"), i); imgSlashFX[i].Load(path); }
    for (int i = 0; i < 6; i++) { wsprintf(path, TEXT("assets/spr_doorbreak/%d.png"), i); imgDoorKick[i].Load(path); }
    for (int i = 0; i < 10; i++) { wsprintf(path, TEXT("assets/spr_doorbreak_full/%d.png"), i); imgDoorKickFull[i].Load(path); }
}

void Player::Update(int mouseX, int mouseY, float camX, float camY, float rs, float ox, float oy, bool fv) {
    DWORD ct = GetTickCount();
    if (m_isRewinding) {
        if (m_history.empty()) { m_isRewinding = false; return; }
        for (int i = 0; i < m_rewindSpeed; i++) {
            if (m_history.empty()) break;
            RewindData d = m_history.back(); m_history.pop_back();
            m_x = d.x; m_y = d.y; m_state = d.state; m_currentFrame = d.frame;
            m_isFacingRight = d.isFacingRight; m_attackAngle = d.attackAngle;
        }
        return;
    }
    m_history.push_back({ m_x, m_y, m_state, m_currentFrame, m_isFacingRight, m_attackAngle });
    if (m_history.size() > (size_t)m_maxHistorySize) m_history.erase(m_history.begin());

    bool isW = GetAsyncKeyState('W') & 0x8000, isA = GetAsyncKeyState('A') & 0x8000, isS = GetAsyncKeyState('S') & 0x8000, isD = GetAsyncKeyState('D') & 0x8000, isJ = (GetAsyncKeyState('W') & 0x8000) || (GetAsyncKeyState(VK_SPACE) & 0x8000);
    int twd = 0; if (CheckSpecificCollision(m_x - 3.0f, m_y, m_colW, m_colH, 3)) twd = -1; else if (CheckSpecificCollision(m_x + 3.0f, m_y, m_colW, m_colH, 3)) twd = 1;
    bool isShift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
    static DWORD lt = ct; float dT = (ct - lt) / 1000.0f; lt = ct;
    if (isShift) { if (m_canSlowMo && m_batteryLevel >= 1.0f) { if (!m_isSlowMo) { m_isSlowMo = true; m_slowMoStartTime = ct; } } if (m_isSlowMo) { m_batteryLevel -= (11.0f / 6.5f) * dT; if (m_batteryLevel <= 0.0f) { m_batteryLevel = 0.0f; m_isSlowMo = false; m_canSlowMo = false; } } }
    else { m_isSlowMo = false; m_canSlowMo = true; }
    if (!m_isSlowMo && m_batteryLevel < 11.0f) { m_batteryLevel += (11.0f / 11.0f) * dT; if (m_batteryLevel > 11.0f) m_batteryLevel = 11.0f; }
    float ts = m_isSlowMo ? 0.3f : 1.0f;
    if (ts != m_lastTimeScale) { float f = ts / m_lastTimeScale; m_vx *= f; m_vy *= f; m_lastTimeScale = ts; }
    float cAcc = 0.6f * ts * ts, cFri = 0.3f * ts * ts, cDSp = 25.0f * ts, cWSp = 12.0f * ts;
    float cJP = -11.0f * ts, cWJP_Y = -11.0f * ts, cWJP_X = 14.0f * (m_isSlowMo ? ts : 1.0f);
    bool air = m_isJumping || (m_vy != 0.0f); if (!isJ) m_canJump = true;
    if (isJ && m_canJump) {
        if (m_state == PlayerState::WALL_GRAB || m_state == PlayerState::WALL_SLIDE) {
            m_state = PlayerState::WALL_FLIP; m_currentFrame = 0; m_vy = cWJP_Y; m_vx = (m_wallDir == 1) ? -cWJP_X : cWJP_X; m_isFacingRight = (m_wallDir == -1); m_isJumping = true; m_canJump = false; m_x += (m_wallDir == 1) ? -2.0f : 2.0f; twd = 0; m_jumpHoldTimer = ct;
            if (!m_isSlowMo) EffectManager::AddJumpCloudVFX(m_x + ((m_wallDir == 1) ? m_colW + 2.0f : -2.0f), m_y + m_colH / 2.0f, ct, (m_wallDir == 1) ? -1.5708f : 1.5708f);
        } else if (!air && m_state != PlayerState::ROLL && m_state != PlayerState::ATTACK && m_state != PlayerState::WALL_FLIP && m_state != PlayerState::DOOR_KICK && m_state != PlayerState::DOOR_KICK_FULL) {
            m_vy = cJP; m_isJumping = true; m_canJump = false; m_jumpHoldTimer = ct; if (!m_isSlowMo) EffectManager::AddJumpCloudVFX(m_x + m_colW / 2.0f, m_y + m_colH, ct);
        }
    }
    bool curL = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0; static bool prL = false;
    float wx = (mouseX - ox) / rs, wy = (mouseY - oy) / rs; if (!fv) { wx += camX; wy += camY; }
    if (curL && !prL && m_state != PlayerState::ATTACK && m_state != PlayerState::PREVDOWN && m_state != PlayerState::DOWN && m_state != PlayerState::DOOR_KICK && m_state != PlayerState::DOOR_KICK_FULL) {
        if (ct - m_lastAttackTime >= (DWORD)g_playerAttackCooldown) {
            m_state = PlayerState::ATTACK; m_currentFrame = 0; m_lastAttackTime = ct; m_isAttackClicked = true; if (!air) m_hasLeapedInAir = false;
            float dx = wx - (m_x + m_colW / 2.0f), dy = wy - (m_y + m_colH / 2.0f), dist = sqrt(dx * dx + dy * dy);
            m_attackAngle = atan2(dy, dx); m_isFacingRight = (dx >= 0);
            if (dist > 0) { m_attackDirX = dx / dist; m_attackDirY = dy / dist; } else { m_attackDirX = 1.0f; m_attackDirY = 0.0f; }
            m_dashDirX = m_attackDirX; m_dashDirY = m_attackDirY;
            if (dy < 0) { if (m_canAirYDash) m_canAirYDash = false; else { m_dashDirY = 0.0f; m_dashDirX = (dx >= 0) ? 1.0f : -1.0f; } }
            float dD = (std::min)(dist, m_dashRadius); m_attackTargetX = m_x + m_dashDirX * dD; m_attackTargetY = m_y + m_dashDirY * dD;
        }
    }
    prL = curL;
    if (air && twd != 0 && m_state != PlayerState::ATTACK && m_state != PlayerState::ROLL && m_state != PlayerState::DOOR_KICK && m_state != PlayerState::DOOR_KICK_FULL) {
        if (m_state != PlayerState::WALL_GRAB && m_state != PlayerState::WALL_SLIDE) {
            if (((twd == -1 && isA) || (twd == 1 && isD)) || (m_state == PlayerState::WALL_FLIP && twd != m_wallDir)) { m_state = PlayerState::WALL_GRAB; m_currentFrame = 0; m_wallGrabTime = ct; m_wallDir = twd; m_isFacingRight = (m_wallDir == 1); m_canAirYDash = true; }
        } else { if ((m_wallDir == 1 && isA) || (m_wallDir == -1 && isD) || twd != m_wallDir) m_state = PlayerState::FALL; }
    } else if (m_state == PlayerState::WALL_GRAB || m_state == PlayerState::WALL_SLIDE) { if (!air) m_state = PlayerState::IDLE; else m_state = PlayerState::FALL; }
    float tvx = 0.0f;
    if (m_state == PlayerState::ATTACK) {
        float dx = m_attackTargetX - m_x, dy = m_attackTargetY - m_y; float dtt = sqrt(dx * dx + dy * dy);
        if (dtt > cDSp) { if (!CheckMapCollision(m_x + m_dashDirX * cDSp, m_y, m_colW, m_colH)) m_x += m_dashDirX * cDSp; if (!CheckMapCollision(m_x, m_y + m_dashDirY * cDSp, m_colW, m_colH)) m_y += m_dashDirY * cDSp; }
        else { m_x = m_attackTargetX; m_y = m_attackTargetY; }
        m_vy = 0.0f; m_vx = 0.0f;
    } else if (m_state == PlayerState::ROLL) m_vx = m_isFacingRight ? 15.0f * ts : -15.0f * ts;
    else if (m_state == PlayerState::WALL_GRAB || m_state == PlayerState::WALL_SLIDE || m_state == PlayerState::DOOR_KICK || m_state == PlayerState::DOOR_KICK_FULL) m_vx = 0.0f;
    else if (m_state == PlayerState::IDLE_TO_WALK) { if (isA) { tvx = -1.0f * ts; m_isFacingRight = false; } if (isD) { tvx = 1.0f * ts; m_isFacingRight = true; } }
    else if (m_state != PlayerState::WALK_TO_IDLE && m_state != PlayerState::PREVDOWN && m_state != PlayerState::DOWN && m_state != PlayerState::POSTDOWN) { if (isA) { tvx = -cWSp; m_isFacingRight = false; } if (isD) { tvx = cWSp; m_isFacingRight = true; } }
    if (tvx != 0.0f && m_state != PlayerState::ROLL && m_state != PlayerState::ATTACK && m_state != PlayerState::WALL_GRAB && m_state != PlayerState::WALL_SLIDE && m_state != PlayerState::WALL_FLIP) m_vx += (tvx - m_vx) * cAcc;
    else if (m_state != PlayerState::ROLL && m_state != PlayerState::ATTACK && m_state != PlayerState::WALL_GRAB && m_state != PlayerState::WALL_SLIDE && m_state != PlayerState::WALL_FLIP) { m_vx += (0.0f - m_vx) * cFri; if (fabs(m_vx) < 0.1f) m_vx = 0.0f; }
    if (m_vx != 0.0f && m_state != PlayerState::ATTACK && m_state != PlayerState::WALL_GRAB && m_state != PlayerState::WALL_SLIDE) {
        float nx = m_x + m_vx; if (!CheckMapCollision(nx, m_y, m_colW, m_colH - 5)) m_x = nx;
        else { bool step = false; for (int i = 1; i <= 15; i++) if (!CheckMapCollision(nx, m_y - i, m_colW, m_colH - 5)) { m_x = nx; m_y -= i; step = true; break; }
            if (!step) { float si = (m_vx > 0) ? 1.0f : -1.0f; int f = 0; while (!CheckMapCollision(m_x + si, m_y, m_colW, m_colH - 5) && f++ < (int)fabs(m_vx) + 2) m_x += si; if (m_state != PlayerState::WALL_FLIP) m_vx = 0.0f; }
        }
    }
    if (isS && !air && m_state != PlayerState::ROLL && m_state != PlayerState::ATTACK && m_state != PlayerState::WALL_GRAB && m_state != PlayerState::WALL_SLIDE && m_state != PlayerState::WALL_FLIP) {
        if (GetCollisionType((int)m_x, (int)(m_y + m_colH + 1)) == 2 || GetCollisionType((int)(m_x + m_colW / 2), (int)(m_y + m_colH + 1)) == 2 || GetCollisionType((int)(m_x + m_colW), (int)(m_y + m_colH + 1)) == 2) { m_y += 4.0f; m_isJumping = true; m_vy = 1.0f; }
    }
    if (m_state != PlayerState::ATTACK) {
        if (m_state == PlayerState::WALL_GRAB) { m_vy = 0.0f; if (ct - m_wallGrabTime >= 150) { m_state = PlayerState::WALL_SLIDE; m_currentFrame = 0; } }
        else {
            bool jhv = isJ && (ct - m_jumpHoldTimer < (DWORD)200); float cg = (jhv && m_vy < 0.0f) ? 0.45f * ts * ts : 1.0f * ts * ts; m_vy += cg;
            float mf = 30.0f * ts; if (m_state == PlayerState::WALL_SLIDE && m_vy >= 0.0f) { mf = isS ? 12.0f * ts : 2.5f * ts; m_vy = mf; } else if (m_vy > mf) m_vy = mf;
        }
        float ny = m_y + m_vy;
        if (m_vy > 0) {
            bool hf = false; auto cf = [&](float ty) { int tl = GetCollisionType((int)m_x, (int)(ty + m_colH)), tc = GetCollisionType((int)(m_x + m_colW / 2), (int)(ty + m_colH)), tr = GetCollisionType((int)(m_x + m_colW), (int)(ty + m_colH)); if (tl == 1 || tl == 3 || tc == 1 || tc == 3 || tr == 1 || tr == 3) return true; if (tl == 2 || tc == 2 || tr == 2) { if (m_y + m_colH <= ty + m_colH) return true; } return false; };
            for (float sy = m_y; sy <= ny; sy += 1.0f) if (cf(sy)) { m_y = sy; hf = true; break; }
            if (hf) { if (air && !m_isSlowMo) EffectManager::AddLandCloudVFX(m_x + m_colW / 2.0f, m_y + m_colH, ct); m_isJumping = false; m_vy = 0; m_canAirYDash = true; m_hasLeapedInAir = false; } else { m_y = ny; if (!cf(m_y + 1.0f)) m_isJumping = true; else { m_isJumping = false; m_canAirYDash = true; } }
        } else if (m_vy < 0) {
            auto ch = [&](float ty) { return (GetCollisionType((int)(m_x + 2.0f), (int)ty) % 2 != 0 || GetCollisionType((int)(m_x + m_colW / 2.0f), (int)ty) % 2 != 0 || GetCollisionType((int)(m_x + m_colW - 2.0f), (int)ty) % 2 != 0); };
            bool hc = false; for (float sy = m_y; sy >= ny; sy -= 1.0f) if (ch(sy)) { m_y = sy; hc = true; break; }
            if (hc) m_vy = 0; else m_y = ny;
        }
        int ml = StageManager::GetMap().IsNull() ? 720 : StageManager::GetMap().GetHeight(); if (m_y + m_colH > ml - 20) { m_y = (float)ml - m_colH - 20.0f; m_isJumping = false; m_vy = 0; }
    }
    PlayerState nst = m_state;
    if (m_state == PlayerState::ATTACK && m_currentFrame >= 5) nst = air ? PlayerState::FALL : PlayerState::IDLE;
    else if (m_state == PlayerState::DOOR_KICK && m_currentFrame >= 5) nst = air ? PlayerState::FALL : PlayerState::IDLE;
    else if (m_state == PlayerState::DOOR_KICK_FULL && m_currentFrame >= 9) nst = air ? PlayerState::FALL : PlayerState::IDLE;
    else if (m_state == PlayerState::WALL_FLIP && m_currentFrame >= 10) nst = air ? (m_vy < 0.0f ? PlayerState::JUMP_UP : PlayerState::FALL) : PlayerState::IDLE;
    else if (m_state == PlayerState::ROLL && m_currentFrame >= 6) nst = (isA || isD) ? PlayerState::WALK : (isS ? PlayerState::DOWN : PlayerState::IDLE);
    else if (m_state == PlayerState::PREVDOWN && m_currentFrame >= 2) nst = PlayerState::DOWN;
    else if (m_state == PlayerState::POSTDOWN && m_currentFrame >= 2) nst = PlayerState::IDLE;
    if (!isS) { m_canRoll = true; if (nst == PlayerState::PREVDOWN || nst == PlayerState::DOWN) nst = PlayerState::POSTDOWN; }
    else {
        if (m_canRoll && (isA || isD) && !air && nst != PlayerState::ROLL && nst != PlayerState::ATTACK && nst != PlayerState::WALL_GRAB && nst != PlayerState::WALL_SLIDE && nst != PlayerState::WALL_FLIP && nst != PlayerState::DOOR_KICK && nst != PlayerState::DOOR_KICK_FULL) { nst = PlayerState::ROLL; m_canRoll = false; m_isFacingRight = isD; }
        else if (!m_canRoll && (isA || isD) && !air && nst != PlayerState::ROLL && nst != PlayerState::ATTACK && nst != PlayerState::WALL_GRAB && nst != PlayerState::WALL_SLIDE && nst != PlayerState::WALL_FLIP && nst != PlayerState::DOOR_KICK && nst != PlayerState::DOOR_KICK_FULL) nst = PlayerState::WALK;
        else if (!(isA || isD) && !air && nst != PlayerState::ROLL && nst != PlayerState::ATTACK && nst != PlayerState::PREVDOWN && nst != PlayerState::DOWN && nst != PlayerState::POSTDOWN && nst != PlayerState::WALL_GRAB && nst != PlayerState::WALL_SLIDE && nst != PlayerState::WALL_FLIP && nst != PlayerState::DOOR_KICK && nst != PlayerState::DOOR_KICK_FULL) nst = PlayerState::PREVDOWN;
        else if (!(isA || isD) && nst == PlayerState::POSTDOWN) nst = PlayerState::PREVDOWN;
    }
    if (nst != PlayerState::ROLL && nst != PlayerState::ATTACK && nst != PlayerState::PREVDOWN && nst != PlayerState::DOWN && nst != PlayerState::POSTDOWN && nst != PlayerState::WALL_GRAB && nst != PlayerState::WALL_SLIDE && nst != PlayerState::WALL_FLIP && nst != PlayerState::DOOR_KICK && nst != PlayerState::DOOR_KICK_FULL) {
        if (air) nst = (m_vy < 0.0f) ? PlayerState::JUMP_UP : PlayerState::FALL;
        else { if (isA || isD) nst = (m_state == PlayerState::IDLE_TO_WALK) ? (m_currentFrame >= 3 ? PlayerState::WALK : PlayerState::IDLE_TO_WALK) : ((m_state == PlayerState::WALK || m_state == PlayerState::RUN) ? PlayerState::WALK : PlayerState::IDLE_TO_WALK);
            else nst = (m_state == PlayerState::WALK_TO_IDLE) ? (m_currentFrame >= 4 ? PlayerState::IDLE : PlayerState::WALK_TO_IDLE) : ((m_state == PlayerState::WALK || m_state == PlayerState::RUN || m_state == PlayerState::IDLE_TO_WALK || m_state == PlayerState::FALL) ? PlayerState::WALK_TO_IDLE : PlayerState::IDLE);
        }
    } else if (!air && (nst == PlayerState::WALL_FLIP || nst == PlayerState::WALL_SLIDE || nst == PlayerState::WALL_GRAB)) nst = (isA || isD) ? PlayerState::IDLE_TO_WALK : PlayerState::WALK_TO_IDLE;
    if (m_state != nst) { m_currentFrame = 0; m_state = nst; }
    if (!air && (m_state == PlayerState::WALK || m_state == PlayerState::RUN) && !m_isSlowMo) { static bool lfr = m_isFacingRight; if (!m_wasMoving || (m_isFacingRight != lfr)) { for (int i = 0; i < 3; i++) EffectManager::AddDustCloudVFX(m_x + (m_isFacingRight ? 0 : m_colW) + (m_isFacingRight ? m_dustOffsetX[i] : -m_dustOffsetX[i]), m_y + m_colH + m_dustOffsetY[i], m_isFacingRight, ct); } lfr = m_isFacingRight; }
    m_wasMoving = !air && (m_state == PlayerState::WALK || m_state == PlayerState::RUN);
    if (m_state == PlayerState::ROLL && !air && !m_isSlowMo) { if (m_currentFrame != m_lastRollFrame) { int cnts[] = { 1, 1, 2, 2, 3, 4 }; int c = cnts[(std::min)(m_currentFrame, 5)]; for (int i = 0; i < c; i++) EffectManager::AddDustCloudVFX(m_x + (m_isFacingRight ? 0 : m_colW) + (float)(rand() % 21 - 10), m_y + m_colH + (float)(rand() % 11 - 5) + 2.0f, m_isFacingRight, ct); m_lastRollFrame = m_currentFrame; } } else m_lastRollFrame = -1;
    if (m_state == PlayerState::WALL_SLIDE && m_vy > 0.0f && !m_isSlowMo) { static DWORD lwdt = 0; if (ct - lwdt >= 150) { for (int i = 0; i < 2; i++) EffectManager::AddDustCloudVFX(m_x + (m_wallDir == 1 ? m_colW : 0) + (float)(rand() % 11 - 5), m_y + m_colH + (float)(rand() % 11 - 5), m_wallDir == -1, ct); lwdt = ct; } }
    bool leap = false; if (m_state == PlayerState::ATTACK) { float dx = m_attackTargetX - m_x, dy = m_attackTargetY - m_y; if (sqrt(dx * dx + dy * dy) > 1.0f) { if (!m_hasLeapedInAir && m_isAttackClicked && m_currentFrame == 0) { leap = true; m_hasLeapedInAir = true; } } if (m_currentFrame >= 1) m_isAttackClicked = false; }
    if (leap || m_state == PlayerState::ROLL || m_state == PlayerState::WALL_FLIP || m_state == PlayerState::ATTACK || m_isSlowMo) { DWORD iv = m_isSlowMo ? (DWORD)30 : (DWORD)1; if (ct - m_lastAfterImageTime >= iv) { for (int i = (int)m_afterImages.size() - 1; i > 0; i--) m_afterImages[i] = m_afterImages[i - 1]; m_afterImages[0] = { m_x, m_y, m_state, m_currentFrame, m_isFacingRight, m_attackAngle, true }; m_lastAfterImageTime = ct; } }
    else { if (ct - m_lastAfterImageTime >= (DWORD)1) { for (int i = (int)m_afterImages.size() - 1; i > 0; i--) m_afterImages[i] = m_afterImages[i - 1]; m_afterImages[0].active = false; m_lastAfterImageTime = ct; } }

}

void Player::UpdateAnimation() {
    static DWORD lt = GetTickCount(); DWORD ct = GetTickCount(); DWORD d = 150;
    switch (m_state) {
    case PlayerState::IDLE: d = 150; break; case PlayerState::IDLE_TO_WALK: d = 60; break; case PlayerState::WALK: d = 80; break; case PlayerState::WALK_TO_IDLE: d = 60; break; case PlayerState::RUN: d = 80; break; case PlayerState::JUMP_UP: case PlayerState::FALL: d = 100; break; case PlayerState::PREVDOWN: case PlayerState::DOWN: case PlayerState::POSTDOWN: d = 80; break; case PlayerState::ROLL: d = 50; break; case PlayerState::ATTACK: d = 40; break; case PlayerState::WALL_GRAB: d = 80; break; case PlayerState::WALL_SLIDE: d = 100; break; case PlayerState::WALL_FLIP: d = 40; break; case PlayerState::DOOR_KICK: d = 40; break; case PlayerState::DOOR_KICK_FULL: d = 40; break;
    }
    if (m_isSlowMo) d = (DWORD)(d / 0.3f);
    if (ct - lt >= d) {
        m_currentFrame++; lt = ct;
        if (m_state == PlayerState::IDLE && m_currentFrame >= 11) m_currentFrame = 0;
        if (m_state == PlayerState::IDLE_TO_WALK && m_currentFrame >= 4) m_currentFrame = 3;
        if (m_state == PlayerState::WALK && m_currentFrame >= 10) m_currentFrame = 0;
        if (m_state == PlayerState::WALK_TO_IDLE && m_currentFrame >= 5) m_currentFrame = 4;
        if (m_state == PlayerState::RUN && m_currentFrame >= 10) m_currentFrame = 0;
        if (m_state == PlayerState::JUMP_UP && m_currentFrame >= 4) m_currentFrame = 3;
        if (m_state == PlayerState::FALL && m_currentFrame >= 4) m_currentFrame = 3;
        if (m_state == PlayerState::DOWN && m_currentFrame >= 1) m_currentFrame = 0;
        if (m_state == PlayerState::ATTACK && m_currentFrame >= 5) m_currentFrame = 5;
        if (m_state == PlayerState::WALL_GRAB && m_currentFrame >= 2) m_currentFrame = 1;
        if (m_state == PlayerState::WALL_SLIDE && m_currentFrame >= 1) m_currentFrame = 0;
        if (m_state == PlayerState::WALL_FLIP && m_currentFrame >= 11) m_currentFrame = 10;
        if (m_state == PlayerState::DOOR_KICK && m_currentFrame >= 6) m_currentFrame = 5;
        if (m_state == PlayerState::DOOR_KICK_FULL && m_currentFrame >= 10) m_currentFrame = 9;
    }
}

void Player::Render(HDC hMemDC, float camX, float camY, float mapScale, float playerScale, float g_renderMapScale, float g_mapOffsetX, float g_mapOffsetY, bool g_isFullMapView, bool g_showDebugRect) {
    if (!hMemDC) return;
    int mapW = StageManager::GetMapWidth(), mapH = StageManager::GetMapHeight(); float pFS = mapScale;
    Gdiplus::Graphics graphics(hMemDC);
    for (int i = (int)m_afterImages.size() - 1; i >= -1; i--) {
        CImage* img = NULL; float px, py; PlayerState s; int f; bool fac; float a;
        if (i >= 0) { if (!m_afterImages[i].active) continue; px = m_afterImages[i].x; py = m_afterImages[i].y; s = m_afterImages[i].state; f = m_afterImages[i].frame; fac = m_afterImages[i].isFacingRight; a = m_afterImages[i].attackAngle; }
        else { px = m_x; py = m_y; s = m_state; f = m_currentFrame; fac = m_isFacingRight; a = m_attackAngle; }
        
        int safeF = (std::max)(0, f);
        switch (s) {
        case PlayerState::IDLE: img = &imgIdle[(std::min)(safeF, 10)]; break; 
        case PlayerState::IDLE_TO_WALK: img = &imgIdleToWalk[(std::min)(safeF, 3)]; break; 
        case PlayerState::WALK: img = &imgWalk[(std::min)(safeF, 9)]; break; 
        case PlayerState::WALK_TO_IDLE: img = &imgWalkToIdle[(std::min)(safeF, 4)]; break; 
        case PlayerState::RUN: img = &imgRun[(std::min)(safeF, 9)]; break; 
        case PlayerState::JUMP_UP: img = &imgJumpUp[(std::min)(safeF, 3)]; break; 
        case PlayerState::FALL: img = &imgFall[(std::min)(safeF, 3)]; break; 
        case PlayerState::PREVDOWN: img = &imgPrevDown[(std::min)(safeF, 1)]; break; 
        case PlayerState::DOWN: img = &imgDown[0]; break; 
        case PlayerState::POSTDOWN: img = &imgPostDown[(std::min)(safeF, 1)]; break; 
        case PlayerState::ROLL: img = &imgRoll[(std::min)(safeF, 5)]; break; 
        case PlayerState::ATTACK: img = &imgAttack[(std::min)(safeF, 6)]; break; 
        case PlayerState::WALL_GRAB: img = &imgWallGrab[(std::min)(safeF, 1)]; break; 
        case PlayerState::WALL_SLIDE: img = &imgWallSlide[0]; break; 
        case PlayerState::WALL_FLIP: img = &imgWallFlip[(std::min)(safeF, 10)]; break; 
        case PlayerState::DOOR_KICK: img = &imgDoorKick[(std::min)(safeF, 5)]; break; 
        case PlayerState::DOOR_KICK_FULL: img = &imgDoorKickFull[(std::min)(safeF, 9)]; break;
        }
        if (img && !img->IsNull() && img->IsDIBSection()) {
            float vx, vy; if (g_isFullMapView) { float fs = (std::min)(1280.0f / (float)(mapW > 0 ? mapW : 1), 720.0f / (float)(mapH > 0 ? mapH : 1)); vx = px * fs + (1280.0f - mapW * fs) / 2.0f; vy = py * fs + (720.0f - mapH * fs) / 2.0f; pFS = fs; }
            else { vx = (px - camX) * mapScale; vy = (py - camY) * mapScale; pFS = mapScale; }
            float sw = img->GetWidth() * playerScale * pFS, sh = img->GetHeight() * playerScale * pFS;
            float dx = vx + (40.0f * pFS) / 2.0f - (sw / 2.0f), dy = vy + (64.0f * pFS) - sh;
            Gdiplus::Bitmap gb(img->GetWidth(), img->GetHeight(), img->GetPitch(), PixelFormat32bppARGB, (BYTE*)img->GetBits());
            Gdiplus::ImageAttributes at; Gdiplus::ColorMatrix mat;
            if (i == -1) { if (m_isSlowMo) mat = { 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,1,1,1,0, 0,0,0,0,1 }; else mat = { 1,0,0,0,0, 0,1,0,0,0, 0,0,1,0,0, 0,0,0,1,0, 0,0,0,0,1 }; }
            else { float al = (1.0f - ((float)i / (float)m_afterImages.size())); if (m_isSlowMo) { al *= 0.4f; mat = { 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,1,1,al,0, 0,0,0,0,1 }; } else { if (i % 2 == 0) { al *= 0.8f; mat = { 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,1,1,al,0, 0,0,0,0,1 }; } else { al *= 0.6f; mat = { 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 1,0,1,al,0, 0,0,0,0,1 }; } } }
            at.SetColorMatrix(&mat, Gdiplus::ColorMatrixFlagsDefault, Gdiplus::ColorAdjustTypeBitmap);
            if (fac) graphics.DrawImage(&gb, Gdiplus::RectF(dx, dy, sw, sh), 0, 0, (float)img->GetWidth(), (float)img->GetHeight(), Gdiplus::UnitPixel, &at);
            else { graphics.ScaleTransform(-1.0f, 1.0f); graphics.TranslateTransform(-(dx * 2 + sw), 0); graphics.DrawImage(&gb, Gdiplus::RectF(dx, dy, sw, sh), 0, 0, (float)img->GetWidth(), (float)img->GetHeight(), Gdiplus::UnitPixel, &at); graphics.ResetTransform(); }
            if (i == -1 && s == PlayerState::ATTACK && f < 5) {
                CImage* si = &imgSlashFX[(std::min)(safeF, 4)]; if (si && !si->IsNull() && si->IsDIBSection()) {
                    int slw = (int)(si->GetWidth() * playerScale * pFS), slh = (int)(si->GetHeight() * playerScale * pFS);
                    Gdiplus::Bitmap gs(si->GetWidth(), si->GetHeight(), si->GetPitch(), PixelFormat32bppARGB, (BYTE*)si->GetBits());
                    graphics.TranslateTransform(vx + (40.0f * pFS) / 2.0f, vy + (64.0f * pFS) / 2.0f); graphics.RotateTransform(a * 180.0f / 3.14159f);
                    graphics.DrawImage(&gs, Gdiplus::RectF(-slw / 2.0f, -slh / 2.0f, (float)slw, (float)slh), 0, 0, (float)si->GetWidth(), (float)si->GetHeight(), Gdiplus::UnitPixel, &at);
                    graphics.ResetTransform();
                }
            }
        }

    }

    if (g_showDebugRect) {
        float vx, vy; if (g_isFullMapView) { float fs = (std::min)(1280.0f / (float)(mapW > 0 ? mapW : 1), 720.0f / (float)(mapH > 0 ? mapH : 1)); vx = m_x * fs + (1280.0f - mapW * fs) / 2.0f; vy = m_y * fs + (720.0f - mapH * fs) / 2.0f; pFS = fs; }
        else { vx = (m_x - camX) * mapScale; vy = (m_y - camY) * mapScale; pFS = mapScale; }
        HBRUSH gb = CreateSolidBrush(RGB(0, 255, 0)); RECT pr = { (int)vx, (int)vy, (int)(vx + 40.0f * pFS), (int)(vy + 64.0f * pFS) }; FrameRect(hMemDC, &pr, gb); DeleteObject(gb);
        if (m_state == PlayerState::ATTACK) {
            float hw = 80.0f * pFS, hh = 60.0f * pFS, hx = vx + (40.0f * pFS) / 2.0f + m_attackDirX * 40.0f * pFS - hw / 2.0f, hy = vy + (64.0f * pFS) / 2.0f + m_attackDirY * 40.0f * pFS - hh / 2.0f;
            HBRUSH rb = CreateSolidBrush(RGB(255, 0, 0)); RECT ar = { (int)hx, (int)hy, (int)(hx + hw), (int)(hy + hh) }; FrameRect(hMemDC, &ar, rb); DeleteObject(rb);
        }
    }
}

