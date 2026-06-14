#include "Enemy.h"
#include "Player.h"
#include "Physics.h"
#include "Bullet.h"
#include "../SceneAndMap/Camera.h"
#include "../Effects/EffectManager.h"
#include "../Core/SoundManager.h"
#include <gdiplus.h>
#include <cmath>
#include <map>

CImage Enemy::m_ImgExclaim[2];
CImage Enemy::m_imgFire2[6];

Enemy::Enemy(float startX, float startY, EnemyType type, float patrolRange) {
    m_startX = startX; m_startY = startY; m_x = startX; m_y = startY;
    m_vx = 2.0f; m_vy = 0.0f; m_colW = 40.0f; m_colH = 60.0f;
    m_isAlive = true; m_isFacingLeft = false; m_Type = type; m_State = EnemyState::ES_IDLE;
    m_CurrentFrame = 0; m_LastTime = GetTickCount(); m_friction = 0.96f; m_knockbackVx = 0.0f;
    m_isImmortal = false; m_patternTimer = GetTickCount(); m_isWaiting = false; m_walkDistance = 0.0f;
    m_patrolRange = patrolRange;
    m_isPlayerDetected = false; m_alertStartTime = 0; m_exclaimFrame = 0;
    m_bloodDistance = 0.0f;
    m_lastBleedTime = 0;
    m_isBurning = false;
    m_burnTimer = 0.0f;
}
Enemy::~Enemy() {}

void Enemy::LoadCommonAssets() {
    if (!m_imgFire2[0].IsNull()) return;
    TCHAR path[256];
    for (int i = 0; i < 6; i++) {
        wsprintf(path, TEXT("assets/spr_fire_2/spr_fire_2_%d.png"), i);
        m_imgFire2[i].Load(path);
    }
    wsprintf(path, TEXT("assets/enemy/spr_enemy_follow/spr_enemy_follow_%d.png"), 0); m_ImgExclaim[0].Load(path);
    wsprintf(path, TEXT("assets/enemy/spr_enemy_follow/spr_enemy_follow_%d.png"), 1); m_ImgExclaim[1].Load(path);
}

void Enemy::RenderBurningEffect(HDC hdc, float mapScale, float camX, float camY) {
    // Burning visual effect (fire2) disabled per user request.
}

bool Enemy::IsPlayerInCone(float px, float py, float pw, float ph) {
    float pcx = px + pw / 2.0f, pcy = py + ph / 2.0f, dx = pcx - (m_x + m_colW / 2.0f), dy = pcy - (m_y + m_colH / 2.0f), dist = (float)sqrt(dx * dx + dy * dy);
    if (dist > m_detectRange) return false;
    float angle = atan2(dy, dx) * 180.0f / 3.14159f, absAngle = (float)fabs(angle);
    return m_isFacingLeft ? (absAngle >= 180.0f - m_detectAngle) : (absAngle <= m_detectAngle);
}

void Enemy::UpdateDetection(float px, float py, float pw, float ph, float ts) {
    if (!m_isAlive) return;
    if (!m_isPlayerDetected) { if (IsPlayerInCone(px, py, pw, ph)) { m_isPlayerDetected = true; m_alertStartTime = GetTickCount(); m_exclaimFrame = 0; m_State = EnemyState::ES_ALERT; } }
    else { if (m_exclaimFrame == 0 && GetTickCount() - m_alertStartTime > (DWORD)(100.0f / ts)) m_exclaimFrame = 1; }
}

void Enemy::RenderExclaim(HDC hdc, float camX, float camY, float mapScale) {
    if (!m_isPlayerDetected || !m_isAlive) return;
    if (!m_ImgExclaim[m_exclaimFrame].IsNull()) {
        int ew = (int)(m_ImgExclaim[m_exclaimFrame].GetWidth() * 2.0f * mapScale), eh = (int)(m_ImgExclaim[m_exclaimFrame].GetHeight() * 2.0f * mapScale);
        int ex = (int)((m_x + m_colW / 2.0f - camX) * mapScale) - ew / 2, ey = (int)((m_y - camY) * mapScale) - eh - 10;
        m_ImgExclaim[m_exclaimFrame].Draw(hdc, ex, ey, ew, eh);
    }
}

void Enemy::RenderDetectionRange(HDC hdc, float camX, float camY, float mapScale) {
    if (!m_isAlive) return;
    int ex = (int)((m_x + m_colW / 2.0f - camX) * mapScale), ey = (int)((m_y + m_colH / 2.0f - camY) * mapScale), r = (int)(m_detectRange * mapScale);
    HPEN hRedPen = CreatePen(PS_SOLID, 1, RGB(255, 0, 0)); HPEN hOldPen = (HPEN)SelectObject(hdc, hRedPen);
    HBRUSH hRedBrush = CreateSolidBrush(RGB(255, 0, 0)); HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hRedBrush);
    auto DrawCone = [&](float centralAngle) {
        float startAngle = (centralAngle - m_detectAngle) * 3.14159f / 180.0f, endAngle = (centralAngle + m_detectAngle) * 3.14159f / 180.0f;
        int x1 = ex + (int)(100 * cos(endAngle)), y1 = ey + (int)(100 * sin(endAngle)), x2 = ex + (int)(100 * cos(startAngle)), y2 = ey + (int)(100 * sin(startAngle));
        Pie(hdc, ex - r, ey - r, ex + r, ey + r, x1, y1, x2, y2);
    };
    int oldMode = SetROP2(hdc, R2_COPYPEN); DrawCone(m_isFacingLeft ? 180.0f : 0.0f); SetROP2(hdc, oldMode);
    SelectObject(hdc, hOldBrush); SelectObject(hdc, hOldPen); DeleteObject(hRedPen); DeleteObject(hRedBrush);
}

void Enemy::RenderDebug(HDC hdc, float camX, float camY, float mapScale) {
    int sx = (int)((m_x - camX) * mapScale), sy = (int)((m_y - camY) * mapScale);
    HBRUSH redBrush = CreateSolidBrush(RGB(255, 0, 0)); RECT rect = { sx, sy, sx + (int)(m_colW * mapScale), sy + (int)(m_colH * mapScale) };
    FrameRect(hdc, &rect, redBrush); DeleteObject(redBrush); RenderDetectionRange(hdc, camX, camY, mapScale);
}

void Enemy::ReleaseAll() { 
    Gangster::Release(); Grunt::Release(); Pomp::Release(); ShieldCop::Release(); 
    if (!m_ImgExclaim[0].IsNull()) m_ImgExclaim[0].Destroy(); if (!m_ImgExclaim[1].IsNull()) m_ImgExclaim[1].Destroy();
    for (int i = 0; i < 6; i++) m_imgFire2[i].Destroy();
}

void Enemy::Reset() {
    m_x = m_startX; m_y = m_startY; m_vx = 2.0f; m_vy = 0.0f; m_isAlive = true; m_isFacingLeft = false; m_State = EnemyState::ES_IDLE;
    m_CurrentFrame = 0; m_LastTime = GetTickCount(); m_knockbackVx = 0.0f; m_patternTimer = GetTickCount(); m_isWaiting = false; m_walkDistance = 0.0f; m_isPlayerDetected = false; m_bloodDistance = 0.0f; m_alertStartTime = 0; m_exclaimFrame = 0;
    m_isBurning = false; m_burnTimer = 0.0f;
}

bool Enemy::OnTakeDamage(float kvx, float kvy, DeathCause cause) {
    if (!m_isImmortal) {
        m_isAlive = false; m_State = EnemyState::ES_DEAD; m_vx = kvx; m_vy = kvy; m_CurrentFrame = 0;
        if (cause == DeathCause::FIRE) { 
            m_isBurning = true; 
            m_burnTimer = 2.0f; 
            if (rand() % 2 == 0) SoundManager::Play("explosion1"); 
            else SoundManager::Play("explosion2");
        }
        switch (cause) {
        case DeathCause::BOTTLE: SoundManager::Play("SFX_ENEMY_DIE_BOTTLE"); break;
        case DeathCause::BULLET: SoundManager::Play("SFX_ENEMY_DIE_BULLET"); break;
        case DeathCause::KNIFE:  SoundManager::Play("SFX_ENEMY_DIE_KNIFE");  break;
        case DeathCause::SWORD: default: if (rand() % 2 == 0) SoundManager::Play("SFX_ENEMY_DIE_SWORD1"); else SoundManager::Play("SFX_ENEMY_DIE_SWORD2"); break;
        }
        SoundManager::Play("SFX_BLOODSPLAT");
        float length = (float)sqrt(m_vx * m_vx + m_vy * m_vy);
        if (length > 0) {
            float nvx = m_vx / length, nvy = m_vy / length, perpX = -nvy, perpY = nvx; DWORD ct = GetTickCount();
            for (int i = 0; i < 12; i++) {
                float speed = 3.0f + (rand() % 40) / 10.0f, pvx1 = m_vx * 0.5f + perpX * speed, pvy1 = m_vy * 0.5f + perpY * speed;
                EffectManager::AddBloodSplatter(m_x + m_colW / 2.0f, m_y + m_colH / 2.0f, pvx1, pvy1, atan2(pvy1, pvx1), ct);
                float pvx2 = m_vx * 0.5f - perpX * speed, pvy2 = m_vy * 0.5f - perpY * speed;
                EffectManager::AddBloodSplatter(m_x + m_colW / 2.0f, m_y + m_colH / 2.0f, pvx2, pvy2, atan2(pvy2, pvx2), ct);
            }
        }
        if (!IsMapTransparent((int)(m_x + m_colW / 2.0f), (int)(m_y + m_colH / 2.0f))) EffectManager::AddMapBlood(m_x + m_colW / 2.0f, m_y + m_colH / 2.0f, 0, rand() % 9);
        m_bloodDistance = 0.0f;
    }
    return false;
}
void Enemy::ApplyKnockback(float vx) { m_knockbackVx = vx; m_vx = vx; }

bool Enemy::CheckDoorCollision(float nx, float ny, float nw, float nh) {
    auto doors = StageManager::GetCurrentDoors(); if (!doors) return false;
    for (const auto& d : *doors) { if (d.IsClosed() && nx < d.GetX() + d.GetW() && nx + nw > d.GetX() && ny < d.GetY() + d.GetH() && ny + nh > d.GetY()) return true; }
    return false;
}

void Enemy::Update(float ts, const Player& player) {
    DWORD ct = GetTickCount(); float oldX = m_x, oldY = m_y;
    if (m_isBurning) { m_burnTimer -= ts; if (m_burnTimer <= 0) m_isBurning = false; }
    if (m_isAlive && m_vx != 0.0f) {
        float nx = m_x + m_vx * ts, checkX = (m_vx > 0) ? (nx + m_colW) : nx, checkY = m_y + m_colH + 1.0f; int colType = GetCollisionType((int)checkX, (int)checkY);
        bool canMove = true; if (colType == 0) { if (m_isPlayerDetected && player.GetY() > m_y + 20.0f) canMove = true; else { m_vx = 0.0f; m_isWaiting = true; m_patternTimer = ct; canMove = false; } }
        if (canMove) {
            if (CheckMapCollision(nx, m_y, m_colW, m_colH) || CheckDoorCollision(nx, m_y, m_colW, m_colH)) {
                bool stepped = false; for (int i = 1; i <= 16; i++) { if (!CheckMapCollision(nx, m_y - i, m_colW, m_colH) && !CheckDoorCollision(nx, m_y - i, m_colW, m_colH)) { m_x = nx; m_y -= (float)i; stepped = true; break; } }
                if (!stepped) { m_vx = -m_vx; m_isFacingLeft = (m_vx < 0); }
            } else m_x = nx;
        }
    } else if (!m_isAlive && m_vx != 0.0f) {
        float moveDist = m_vx * ts; int steps = (int)(fabs(moveDist) / 4.0f) + 1; float stepDist = moveDist / steps;
        for (int s = 0; s < steps; s++) {
            float nx = m_x + stepDist;
            if (CheckMapCollision(nx, m_y, m_colW, m_colH) || CheckDoorCollision(nx, m_y, m_colW, m_colH)) {
                m_vx = -m_vx * 0.5f; ResolveMapCollision(nx, m_y, m_colW, m_colH, m_x, m_y); m_x = nx; SoundManager::Play("SFX_HITFLOOR"); break; }
            else m_x = nx;
        }
    }
    if (std::isnan(m_vx) || std::isinf(m_vx)) m_vx = 0.0f; if (std::isnan(m_vy) || std::isinf(m_vy)) m_vy = 0.0f;
    float gravity = m_isAlive ? 1.5f : 1.0f; m_vy += gravity * ts; if (m_vy > 30.0f) m_vy = 30.0f;
    if (m_vy != 0.0f) {
        float ny = m_y + m_vy * ts; bool floorHit = false, ceilingHit = false;
        if (m_vy > 0.0f) {
            int safetyCounter = 0; for (float sy = m_y; sy <= ny && safetyCounter++ < 500; sy += 1.0f) {
                int tl = GetCollisionType((int)(m_x + 2.0f), (int)(sy + m_colH)), tc = GetCollisionType((int)(m_x + m_colW / 2.0f), (int)(sy + m_colH)), tr = GetCollisionType((int)(m_x + m_colW - 2.0f), (int)(sy + m_colH));
                bool hit = (tl == 1 || tl == 3 || tc == 1 || tc == 3 || tr == 1 || tr == 3);
                if (!hit && (tl == 2 || tc == 2 || tr == 2) && m_y + m_colH <= sy + m_colH) hit = true;
                if (hit) { m_y = sy; floorHit = true; break; }
            }
        } else if (m_vy < 0.0f) {
            int safetyCounter = 0; for (float sy = m_y; sy >= ny && safetyCounter++ < 500; sy -= 1.0f) {
                int tl = GetCollisionType((int)(m_x + 2.0f), (int)(sy)), tc = GetCollisionType((int)(m_x + m_colW / 2.0f), (int)(sy)), tr = GetCollisionType((int)(m_x + m_colW - 2.0f), (int)(sy));
                if (tl == 1 || tl == 3 || tc == 1 || tc == 3 || tr == 1 || tr == 3) { m_y = sy + 1.0f; ceilingHit = true; break; }
            }
        }
        if (floorHit) { if (!m_isAlive && fabs(m_vy) > 2.0f) { m_vy = -m_vy * 0.3f; SoundManager::Play("SFX_HITFLOOR"); } else m_vy = 0.0f; if (!m_isAlive) m_vx *= m_friction; }
        else if (ceilingHit) { m_vy = -m_vy * 0.3f; if (!m_isAlive) m_vx *= 0.99f; }
        else { m_y = ny; if (!m_isAlive) m_vx *= 0.99f; if (!m_isAlive && m_vy == 0.0f) m_vy = 0.001f; }
    }
    if (!m_isAlive && (m_vx != 0.0f || m_vy != 0.0f)) {
        float dx = m_x - oldX, dy = m_y - oldY, distMoved = (float)sqrt(dx * dx + dy * dy);
        if (distMoved > 0.1f) {
            static float particleAccumulator = 0.0f; particleAccumulator += distMoved;
            if (particleAccumulator >= 30.0f) {
                particleAccumulator -= 30.0f; float angle = atan2(-m_vy, -m_vx);
                for (int i = 0; i < 3; i++) {
                    float speed = 2.0f + (rand() % 30) / 10.0f, spread = (rand() % 20 - 10) / 10.0f, pvx = -m_vx * 0.2f + cos(angle + spread) * speed, pvy = -m_vy * 0.2f + sin(angle + spread) * speed;
                    EffectManager::AddBloodSplatter(m_x + m_colW / 2.0f, m_y + m_colH / 2.0f, pvx, pvy, atan2(pvy, pvx), ct);
                }
            }
        }
    }
    UpdateDetection(player.GetX(), player.GetY(), player.GetColW(), player.GetColH(), ts);
}

// Gangster
CImage Gangster::m_ImgIdle_R[8], Gangster::m_ImgIdle_L[8], Gangster::m_ImgWalk_R[8], Gangster::m_ImgWalk_L[8], Gangster::m_ImgAim_R[4], Gangster::m_ImgAim_L[4], Gangster::m_ImgTurn_R[6], Gangster::m_ImgTurn_L[6], Gangster::m_ImgFall_R[12], Gangster::m_ImgFall_L[12], Gangster::m_ImgHurtFly_R[2], Gangster::m_ImgHurtFly_L[2], Gangster::m_ImgHurtGround_R[14], Gangster::m_ImgHurtGround_L[14], Gangster::m_ImgRun_R[10], Gangster::m_ImgRun_L[10], Gangster::m_ImgGun_R[2], Gangster::m_ImgGun_L[2], Gangster::m_ImgArm[2];
Gangster::Gangster(float x, float y) : Enemy(x, y, EnemyType::GANGSTER) { m_ActionState = GangsterAction::GA_NONE; }
Gangster::~Gangster() {}
void Gangster::Reset() { Enemy::Reset(); m_ActionState = GangsterAction::GA_NONE; m_aimAngle = 0.0f; }
bool Gangster::OnTakeDamage(float kvx, float kvy, DeathCause cause) { if (m_isImmortal) return false; Enemy::OnTakeDamage(kvx, kvy, cause); m_ActionState = GangsterAction::GA_HURT_FLY; return false; }
void Gangster::Init() { if (!m_ImgIdle_R[0].IsNull()) return; TCHAR path[256]; for (int i = 0; i < 8; i++) { wsprintf(path, TEXT("assets/enemy/spr_gangsteridle/%d.png"), i); m_ImgIdle_R[i].Load(path); m_ImgIdle_L[i].Load(path); } for (int i = 0; i < 8; i++) { wsprintf(path, TEXT("assets/enemy/spr_gangsterwalk/%d.png"), i); m_ImgWalk_R[i].Load(path); m_ImgWalk_L[i].Load(path); } for (int i = 0; i < 4; i++) { wsprintf(path, TEXT("assets/enemy/spr_gangster_aim/%d.png"), i); m_ImgAim_R[i].Load(path); m_ImgAim_L[i].Load(path); } for (int i = 0; i < 6; i++) { wsprintf(path, TEXT("assets/enemy/spr_gangsterturn/%d.png"), i); m_ImgTurn_R[i].Load(path); m_ImgTurn_L[i].Load(path); } for (int i = 0; i < 12; i++) { wsprintf(path, TEXT("assets/enemy/spr_gangsterfall/%d.png"), i); m_ImgFall_R[i].Load(path); m_ImgFall_L[i].Load(path); } for (int i = 0; i < 2; i++) { wsprintf(path, TEXT("assets/enemy/spr_gangsterhurtfly/%d.png"), i); m_ImgHurtFly_R[i].Load(path); m_ImgHurtFly_L[i].Load(path); } for (int i = 0; i < 14; i++) { wsprintf(path, TEXT("assets/enemy/spr_gangsterhurtground/%d.png"), i); m_ImgHurtGround_R[i].Load(path); m_ImgHurtGround_L[i].Load(path); } for (int i = 0; i < 10; i++) { wsprintf(path, TEXT("assets/enemy/spr_gangsterrun/%d.png"), i); m_ImgRun_R[i].Load(path); m_ImgRun_L[i].Load(path); } for (int i = 0; i < 2; i++) { wsprintf(path, TEXT("assets/enemy/spr_gangstergun/%d.png"), i); m_ImgGun_R[i].Load(path); m_ImgGun_L[i].Load(path); } for (int i = 0; i < 2; i++) { wsprintf(path, TEXT("assets/enemy/spr_arm/%d.png"), i); m_ImgArm[i].Load(path); } }
void Gangster::Release() { for (int i = 0; i < 8; i++) { m_ImgIdle_R[i].Destroy(); m_ImgIdle_L[i].Destroy(); } for (int i = 0; i < 8; i++) { m_ImgWalk_R[i].Destroy(); m_ImgWalk_L[i].Destroy(); } for (int i = 0; i < 4; i++) { m_ImgAim_R[i].Destroy(); m_ImgAim_L[i].Destroy(); } for (int i = 0; i < 6; i++) { m_ImgTurn_R[i].Destroy(); m_ImgTurn_L[i].Destroy(); } for (int i = 0; i < 12; i++) { m_ImgFall_R[i].Destroy(); m_ImgFall_L[i].Destroy(); } for (int i = 0; i < 2; i++) { m_ImgHurtFly_R[i].Destroy(); m_ImgHurtFly_L[i].Destroy(); } for (int i = 0; i < 14; i++) { m_ImgHurtGround_R[i].Destroy(); m_ImgHurtGround_L[i].Destroy(); } for (int i = 0; i < 10; i++) { m_ImgRun_R[i].Destroy(); m_ImgRun_L[i].Destroy(); } for (int i = 0; i < 2; i++) { m_ImgGun_R[i].Destroy(); m_ImgGun_L[i].Destroy(); } for (int i = 0; i < 2; i++) { m_ImgArm[i].Destroy(); } }
void Gangster::Update(float ts, const Player& player) {
    if (!m_isAlive) { if (m_ActionState == GangsterAction::GA_HURT_FLY && m_vy == 0) { m_ActionState = GangsterAction::GA_HURT_GROUND; m_vx = 0; m_CurrentFrame = 0; } if (GetTickCount() - m_LastTime >= 100) { if (m_ActionState == GangsterAction::GA_HURT_GROUND) { if (m_CurrentFrame < 13) m_CurrentFrame++; } else m_CurrentFrame++; m_LastTime = GetTickCount(); } Enemy::Update(ts, player); return; }
    Enemy::Update(ts, player); DWORD ct = GetTickCount();
    if (m_isPlayerDetected) {
        float dx = player.GetX() - m_x, dy = player.GetY() - m_y; m_aimAngle = atan2(dy, dx) * 180.0f / 3.14159f; bool nextFacingLeft = (dx < 0);
        if (m_isFacingLeft != nextFacingLeft && m_ActionState != GangsterAction::GA_TURN) { m_ActionState = GangsterAction::GA_TURN; m_CurrentFrame = 0; m_patternTimer = ct; }
        m_isFacingLeft = nextFacingLeft;
        if (m_ActionState == GangsterAction::GA_TURN) { m_vx = 0; if (ct - m_LastTime >= (DWORD)(50.0f / ts)) { m_CurrentFrame++; m_LastTime = ct; if (m_CurrentFrame >= 6) { m_ActionState = GangsterAction::GA_NONE; } } }
        else if (fabs(dx) > 450.0f) { m_ActionState = GangsterAction::GA_RUN; m_State = EnemyState::ES_WALK; m_vx = m_isFacingLeft ? -8.0f : 8.0f; }
        else if (m_ActionState == GangsterAction::GA_NONE || m_ActionState == GangsterAction::GA_RUN) { m_State = EnemyState::ES_IDLE; m_vx = 0; m_ActionState = GangsterAction::GA_AIM; m_CurrentFrame = 0; m_patternTimer = ct - 500; m_LastTime = ct; }
        else if (m_ActionState == GangsterAction::GA_AIM) {
            m_vx = 0; m_State = EnemyState::ES_IDLE;
            if (ct - m_patternTimer > (DWORD)(1000.0f / ts)) {
                float rad = m_aimAngle * 3.14159f / 180.0f, speed = 20.0f, bvx = speed * cos(rad), bvy = speed * sin(rad); if (!m_isFacingLeft && bvx < 0) bvx = -bvx; else if (m_isFacingLeft && bvx > 0) bvx = -bvx;
                float fireX = m_x + m_colW / 2.0f + (m_isFacingLeft ? -25.0f : 25.0f), fireY = m_y + m_colH - 35.0f;
                Bullet::AddBullet(fireX, fireY, bvx, bvy, m_x + m_colW / 2.0f, m_y + m_colH / 2.0f); SoundManager::Play("SFX_GUNFIRE"); EffectManager::AddGunSparkVFX(fireX, fireY, m_aimAngle, ct); m_patternTimer = ct;
            }
        }
    } else {
        if (m_isWaiting) { m_vx = 0.0f; m_State = EnemyState::ES_IDLE; if (ct - m_patternTimer >= (DWORD)(1000.0f / ts)) { m_isWaiting = false; m_patternTimer = ct; m_walkDistance = 0.0f; m_ActionState = GangsterAction::GA_TURN; m_CurrentFrame = 0; m_isFacingLeft = !m_isFacingLeft; } }
        else { 
            if (m_ActionState == GangsterAction::GA_TURN) { if (ct - m_LastTime >= (DWORD)(100.0f / ts)) { m_CurrentFrame++; m_LastTime = ct; if (m_CurrentFrame >= 6) { m_ActionState = GangsterAction::GA_NONE; m_vx = m_isFacingLeft ? -2.0f : 2.0f; } } }
            else { m_State = EnemyState::ES_WALK; m_vx = m_isFacingLeft ? -2.0f : 2.0f; float nx = m_x + m_vx * ts; bool oor = (m_isFacingLeft && nx < m_startX - m_patrolRange / 2.0f) || (!m_isFacingLeft && nx > m_startX + m_patrolRange / 2.0f); if (!CheckMapCollision(nx, m_y, m_colW, m_colH) && !oor) { m_walkDistance += (float)fabs(m_vx * ts); } else { m_isWaiting = true; m_patternTimer = ct; } }
        }
    }
    if (m_vy > 5.0f) m_State = EnemyState::ES_FALL; else if (m_vy == 0.0f && m_State == EnemyState::ES_FALL) m_State = EnemyState::ES_IDLE;
    if (ct - m_LastTime >= (DWORD)(100.0f / ts)) { m_CurrentFrame++; m_LastTime = ct; }
}
void Gangster::Render(HDC hdc, Gdiplus::Graphics* g, float camX, float camY, float mapScale, bool showDebugRect, bool isSlowMo) {
    if (!m_isAlive && m_ActionState != GangsterAction::GA_HURT_FLY && m_ActionState != GangsterAction::GA_HURT_GROUND) return;
    RenderExclaim(hdc, camX, camY, mapScale); int sx = (int)((m_x - camX) * mapScale), sy = (int)((m_y - camY) * mapScale);
    CImage *imgBody = nullptr, *imgGun = nullptr, *imgArm = nullptr; float es = 1.8f, msX = 1.0f, msY = 1.0f;
    if (m_isFacingLeft) {
        imgArm = &m_ImgArm[1]; if (m_ActionState == GangsterAction::GA_HURT_FLY) imgBody = &m_ImgHurtFly_L[m_CurrentFrame % 2]; else if (m_ActionState == GangsterAction::GA_HURT_GROUND || m_State == EnemyState::ES_DEAD) imgBody = &m_ImgHurtGround_L[m_CurrentFrame % 14]; else if (m_State == EnemyState::ES_FALL) imgBody = &m_ImgFall_L[m_CurrentFrame % 12];
        else { switch (m_ActionState) { case GangsterAction::GA_NONE: if (m_State == EnemyState::ES_IDLE || m_State == EnemyState::ES_ALERT) { imgBody = &m_ImgIdle_L[m_CurrentFrame % 8]; msX = 1.1f; msY = 1.1f; } else if (m_State == EnemyState::ES_WALK) imgBody = &m_ImgWalk_L[m_CurrentFrame % 8]; break; case GangsterAction::GA_AIM: imgBody = &m_ImgAim_L[m_CurrentFrame % 4]; imgGun = &m_ImgGun_L[0]; break; case GangsterAction::GA_FIRE: imgBody = &m_ImgAim_L[3]; imgGun = &m_ImgGun_L[m_CurrentFrame == 2 ? 1 : 0]; break; case GangsterAction::GA_TURN: imgBody = &m_ImgTurn_L[m_CurrentFrame % 6]; break; case GangsterAction::GA_RUN: imgBody = &m_ImgRun_L[m_CurrentFrame % 10]; break; default: imgBody = &m_ImgIdle_L[0]; break; } }
    } else {
        imgArm = &m_ImgArm[0]; if (m_ActionState == GangsterAction::GA_HURT_FLY) imgBody = &m_ImgHurtFly_R[m_CurrentFrame % 2]; else if (m_ActionState == GangsterAction::GA_HURT_GROUND || m_State == EnemyState::ES_DEAD) imgBody = &m_ImgHurtGround_R[m_CurrentFrame % 14]; else if (m_State == EnemyState::ES_FALL) imgBody = &m_ImgFall_R[m_CurrentFrame % 12];
        else { switch (m_ActionState) { case GangsterAction::GA_NONE: if (m_State == EnemyState::ES_IDLE || m_State == EnemyState::ES_ALERT) { imgBody = &m_ImgIdle_R[m_CurrentFrame % 8]; msX = 1.1f; msY = 1.1f; } else if (m_State == EnemyState::ES_WALK) imgBody = &m_ImgWalk_R[m_CurrentFrame % 8]; break; case GangsterAction::GA_AIM: imgBody = &m_ImgAim_R[m_CurrentFrame % 4]; imgGun = &m_ImgGun_R[0]; break; case GangsterAction::GA_FIRE: imgBody = &m_ImgAim_R[3]; imgGun = &m_ImgGun_R[m_CurrentFrame == 2 ? 1 : 0]; break; case GangsterAction::GA_TURN: imgBody = &m_ImgTurn_R[m_CurrentFrame % 6]; break; case GangsterAction::GA_RUN: imgBody = &m_ImgRun_R[m_CurrentFrame % 10]; break; default: imgBody = &m_ImgIdle_R[0]; break; } }
    }
    auto Draw = [&](CImage* im, float sX, float sY, float oX = 0, float oY = 0, bool rotate = false) {
        if (!im || im->IsNull()) return;
        int fw = (int)(im->GetWidth() * es * sX * mapScale), fh = (int)(im->GetHeight() * es * sY * mapScale), dx = sx + (int)(m_colW * mapScale / 2) - (fw / 2) + (int)(oX * mapScale), dy = sy + (int)(m_colH * mapScale) - fh + (int)(oY * mapScale);
        int om = SetGraphicsMode(hdc, GM_ADVANCED); XFORM xo; GetWorldTransform(hdc, &xo);
        if (rotate) {
            float angle = m_aimAngle, pivotX = m_isFacingLeft ? (float)(dx + fw * 0.7f) : (float)(dx + fw * 0.3f), pivotY = (float)(dy + fh * 0.5f); if (m_isFacingLeft && (im == &m_ImgArm[1] || im == &m_ImgGun_L[0] || im == &m_ImgGun_L[1])) angle -= 180.0f;
            float rad = angle * 3.14159f / 180.0f, cosA = cos(rad), sinA = sin(rad); XFORM rot = { cosA, sinA, -sinA, cosA, pivotX - pivotX * cosA + pivotY * sinA, pivotY - pivotX * sinA - pivotY * cosA }, combined; CombineTransform(&combined, &rot, &xo); SetWorldTransform(hdc, &combined);
        } else if (m_isFacingLeft) {
            XFORM flip = { -1.0f, 0.0f, 0.0f, 1.0f, (float)(2 * dx + fw), 0.0f }, combined; CombineTransform(&combined, &flip, &xo); SetWorldTransform(hdc, &combined);
        }
        im->Draw(hdc, dx, dy, fw, fh); SetWorldTransform(hdc, &xo); SetGraphicsMode(hdc, om);
    };
    Draw(imgBody, msX, msY); if (imgGun) Draw(imgGun, 1.0f, 1.0f, (m_isFacingLeft ? -25.0f : 25.0f), -35.0f, true);
    if (imgArm && (m_ActionState == GangsterAction::GA_AIM || m_ActionState == GangsterAction::GA_FIRE)) Draw(imgArm, 1.0f, 1.0f, (m_isFacingLeft ? -5.0f : 5.0f), -30.0f, true);
    RenderBurningEffect(hdc, mapScale, camX, camY); if (showDebugRect) RenderDebug(hdc, camX, camY, mapScale);
}

// Grunt
CImage Grunt::m_ImgIdle_R[8], Grunt::m_ImgIdle_L[8], Grunt::m_ImgWalk_R[10], Grunt::m_ImgWalk_L[10], Grunt::m_ImgAttack_R[8], Grunt::m_ImgAttack_L[8], Grunt::m_ImgSlash_R[5], Grunt::m_ImgSlash_L[5], Grunt::m_ImgTurn_R[8], Grunt::m_ImgTurn_L[8], Grunt::m_ImgFall_R[13], Grunt::m_ImgFall_L[13], Grunt::m_ImgHurtFly_R[2], Grunt::m_ImgHurtFly_L[2], Grunt::m_ImgHurtGround_R[16], Grunt::m_ImgHurtGround_L[16], Grunt::m_ImgRun_R[10], Grunt::m_ImgRun_L[10];
Grunt::Grunt(float x, float y) : Enemy(x, y, EnemyType::GRUNT) { m_ActionState = GruntAction::GR_NONE; }
Grunt::~Grunt() {}
void Grunt::Reset() { Enemy::Reset(); m_ActionState = GruntAction::GR_NONE; }
bool Grunt::OnTakeDamage(float kvx, float kvy, DeathCause cause) { if (m_isImmortal) return false; Enemy::OnTakeDamage(kvx, kvy, cause); m_ActionState = GruntAction::GR_HURT_FLY; return false; }
void Grunt::Init() { if (!m_ImgIdle_R[0].IsNull()) return; TCHAR path[256]; for (int i = 0; i < 8; i++) { wsprintf(path, TEXT("assets/enemy/spr_grunt_idle/%d.png"), i); m_ImgIdle_R[i].Load(path); m_ImgIdle_L[i].Load(path); } for (int i = 0; i < 10; i++) { wsprintf(path, TEXT("assets/enemy/spr_grunt_walk/%d.png"), i); m_ImgWalk_R[i].Load(path); m_ImgWalk_L[i].Load(path); } for (int i = 0; i < 8; i++) { wsprintf(path, TEXT("assets/enemy/spr_grunt_attack/%d.png"), i); m_ImgAttack_R[i].Load(path); m_ImgAttack_L[i].Load(path); } for (int i = 0; i < 5; i++) { wsprintf(path, TEXT("assets/enemy/spr_gruntslash/%d.png"), i); m_ImgSlash_R[i].Load(path); m_ImgSlash_L[i].Load(path); } for (int i = 0; i < 8; i++) { wsprintf(path, TEXT("assets/enemy/spr_grunt_turn/%d.png"), i); m_ImgTurn_R[i].Load(path); m_ImgTurn_L[i].Load(path); } for (int i = 0; i < 13; i++) { wsprintf(path, TEXT("assets/enemy/spr_grunt_fall/%d.png"), i); m_ImgFall_R[i].Load(path); m_ImgFall_L[i].Load(path); } for (int i = 0; i < 2; i++) { wsprintf(path, TEXT("assets/enemy/spr_grunt_hurtfly/%d.png"), i); m_ImgHurtFly_R[i].Load(path); m_ImgHurtFly_L[i].Load(path); } for (int i = 0; i < 16; i++) { wsprintf(path, TEXT("assets/enemy/spr_grunt_hurtground/%d.png"), i); m_ImgHurtGround_R[i].Load(path); m_ImgHurtGround_L[i].Load(path); } for (int i = 0; i < 10; i++) { wsprintf(path, TEXT("assets/enemy/spr_grunt_run/%d.png"), i); m_ImgRun_R[i].Load(path); m_ImgRun_L[i].Load(path); } }
void Grunt::Release() { for (int i = 0; i < 8; i++) { m_ImgIdle_R[i].Destroy(); m_ImgIdle_L[i].Destroy(); } for (int i = 0; i < 10; i++) { m_ImgWalk_R[i].Destroy(); m_ImgWalk_L[i].Destroy(); } for (int i = 0; i < 8; i++) { m_ImgAttack_R[i].Destroy(); m_ImgAttack_L[i].Destroy(); } for (int i = 0; i < 5; i++) { m_ImgSlash_R[i].Destroy(); m_ImgSlash_L[i].Destroy(); } for (int i = 0; i < 8; i++) { m_ImgTurn_R[i].Destroy(); m_ImgTurn_L[i].Destroy(); } for (int i = 0; i < 13; i++) { m_ImgFall_R[i].Destroy(); m_ImgFall_L[i].Destroy(); } for (int i = 0; i < 2; i++) { m_ImgHurtFly_R[i].Destroy(); m_ImgHurtFly_L[i].Destroy(); } for (int i = 0; i < 16; i++) { m_ImgHurtGround_R[i].Destroy(); m_ImgHurtGround_L[i].Destroy(); } for (int i = 0; i < 10; i++) { m_ImgRun_R[i].Destroy(); m_ImgRun_L[i].Destroy(); } }
void Grunt::Update(float ts, const Player& player) {
    if (!m_isAlive) { if (m_ActionState == GruntAction::GR_HURT_FLY && m_vy == 0) { m_ActionState = GruntAction::GR_HURT_GROUND; m_vx = 0; m_CurrentFrame = 0; } if (GetTickCount() - m_LastTime >= 100) { if (m_ActionState == GruntAction::GR_HURT_GROUND) { if (m_CurrentFrame < 15) m_CurrentFrame++; } else m_CurrentFrame++; m_LastTime = GetTickCount(); } Enemy::Update(ts, player); return; }
    Enemy::Update(ts, player); DWORD ct = GetTickCount();
    if (m_isPlayerDetected) {
        float dx = player.GetX() - m_x; bool nextFacingLeft = (dx < 0);
        if (m_isFacingLeft != nextFacingLeft && m_ActionState != GruntAction::GR_TURN) { m_ActionState = GruntAction::GR_TURN; m_CurrentFrame = 0; m_patternTimer = ct; }
        m_isFacingLeft = nextFacingLeft;
        if (m_ActionState == GruntAction::GR_TURN) { m_vx = 0; if (ct - m_LastTime >= (DWORD)(100.0f / ts)) { m_CurrentFrame++; m_LastTime = ct; if (m_CurrentFrame >= 8) { m_ActionState = GruntAction::GR_NONE; } } }
        else if (m_ActionState == GruntAction::GR_ATTACK) {
            m_vx = 0; m_State = EnemyState::ES_IDLE; if (ct - m_LastTime >= (DWORD)(100.0f / ts)) {
                m_CurrentFrame++; m_LastTime = ct;
                if (m_CurrentFrame == 5) {
                    float ex = m_x + m_colW / 2.0f, ey = m_y + m_colH / 2.0f, px = player.GetX() + player.GetColW() / 2.0f, py = player.GetY() + player.GetColH() / 2.0f;
                    float dist = (float)sqrt((px - ex) * (px - ex) + (py - ey) * (py - ey)), angle = atan2(py - ey, px - ex) * 180.0f / 3.14159f;
                    if (dist < 100.0f && (m_isFacingLeft ? (fabs(angle) >= 180.0f - m_detectAngle) : (fabs(angle) <= m_detectAngle))) { const_cast<Player&>(player).OnTakeDamage(1.0f, m_isFacingLeft ? -8.0f : 8.0f, -6.0f, ex, ey); }
                }
                if (m_CurrentFrame >= 8) { m_ActionState = GruntAction::GR_NONE; m_CurrentFrame = 0; m_patternTimer = ct; }
            }
        }
        else if (fabs(dx) > 60.0f) { m_ActionState = GruntAction::GR_RUN; m_State = EnemyState::ES_WALK; m_vx = m_isFacingLeft ? -9.0f : 9.0f; }
        else { if (m_ActionState == GruntAction::GR_RUN) m_ActionState = GruntAction::GR_NONE; m_State = EnemyState::ES_IDLE; m_vx = 0; if (ct - m_patternTimer > 500) { m_ActionState = GruntAction::GR_ATTACK; m_CurrentFrame = 0; m_patternTimer = ct; SoundManager::Play("SFX_PUNCH"); } }
    } else {
        if (m_isWaiting) { m_vx = 0.0f; m_State = EnemyState::ES_IDLE; if (ct - m_patternTimer >= (DWORD)(1000.0f / ts)) { m_isWaiting = false; m_patternTimer = ct; m_walkDistance = 0.0f; m_ActionState = GruntAction::GR_TURN; m_CurrentFrame = 0; m_isFacingLeft = !m_isFacingLeft; } }
        else { 
            if (m_ActionState == GruntAction::GR_TURN) { if (ct - m_LastTime >= (DWORD)(100.0f / ts)) { m_CurrentFrame++; m_LastTime = ct; if (m_CurrentFrame >= 8) { m_ActionState = GruntAction::GR_NONE; m_vx = m_isFacingLeft ? -2.5f : 2.5f; } } }
            else { m_State = EnemyState::ES_WALK; m_vx = m_isFacingLeft ? -2.5f : 2.5f; float nx = m_x + m_vx * ts; bool oor = (m_isFacingLeft && nx < m_startX - m_patrolRange / 2.0f) || (!m_isFacingLeft && nx > m_startX + m_patrolRange / 2.0f); if (!CheckMapCollision(nx, m_y, m_colW, m_colH) && !oor) { m_walkDistance += (float)fabs(m_vx * ts); } else { m_isWaiting = true; m_patternTimer = ct; } }
        }
    }
    if (m_vy > 5.0f) m_State = EnemyState::ES_FALL; else if (m_vy == 0.0f && m_State == EnemyState::ES_FALL) m_State = EnemyState::ES_IDLE;
    if (ct - m_LastTime >= (DWORD)(100.0f / ts)) { m_CurrentFrame++; m_LastTime = ct; }
}
void Grunt::Render(HDC hdc, Gdiplus::Graphics* g, float camX, float camY, float mapScale, bool showDebugRect, bool isSlowMo) {
    if (!m_isAlive && m_ActionState != GruntAction::GR_HURT_FLY && m_ActionState != GruntAction::GR_HURT_GROUND) return;
    RenderExclaim(hdc, camX, camY, mapScale); int sx = (int)((m_x - camX) * mapScale), sy = (int)((m_y - camY) * mapScale);
    float es = 1.8f, msX = 1.0f, msY = 1.0f; CImage *img = nullptr, *imgSlash = nullptr;
    if (m_isFacingLeft) { 
        if (m_ActionState == GruntAction::GR_HURT_FLY) img = &m_ImgHurtFly_L[m_CurrentFrame % 2]; 
        else if (m_ActionState == GruntAction::GR_HURT_GROUND || m_State == EnemyState::ES_DEAD) img = &m_ImgHurtGround_L[m_CurrentFrame % 16]; 
        else if (m_State == EnemyState::ES_FALL) img = &m_ImgFall_L[m_CurrentFrame % 13]; 
        else {
            switch (m_ActionState) {
                case GruntAction::GR_NONE: if (m_State == EnemyState::ES_IDLE || m_State == EnemyState::ES_ALERT) { img = &m_ImgIdle_L[m_CurrentFrame % 8]; msX = 1.1f; msY = 1.1f; } else if (m_State == EnemyState::ES_WALK) img = &m_ImgWalk_L[m_CurrentFrame % 10]; break;
                case GruntAction::GR_ATTACK: img = &m_ImgAttack_L[m_CurrentFrame % 8]; if (m_CurrentFrame >= 3 && m_CurrentFrame <= 7) imgSlash = &m_ImgSlash_L[m_CurrentFrame - 3]; break;
                case GruntAction::GR_SLASH: img = &m_ImgSlash_L[m_CurrentFrame % 5]; break;
                case GruntAction::GR_TURN: img = &m_ImgTurn_L[m_CurrentFrame % 8]; break;
                case GruntAction::GR_RUN: img = &m_ImgRun_L[m_CurrentFrame % 10]; break;
            }
        }
    } else { 
        if (m_ActionState == GruntAction::GR_HURT_FLY) img = &m_ImgHurtFly_R[m_CurrentFrame % 2]; 
        else if (m_ActionState == GruntAction::GR_HURT_GROUND || m_State == EnemyState::ES_DEAD) img = &m_ImgHurtGround_R[m_CurrentFrame % 16]; 
        else if (m_State == EnemyState::ES_FALL) img = &m_ImgFall_R[m_CurrentFrame % 13]; 
        else {
            switch (m_ActionState) {
                case GruntAction::GR_NONE: if (m_State == EnemyState::ES_IDLE || m_State == EnemyState::ES_ALERT) { img = &m_ImgIdle_R[m_CurrentFrame % 8]; msX = 1.1f; msY = 1.1f; } else if (m_State == EnemyState::ES_WALK) img = &m_ImgWalk_R[m_CurrentFrame % 10]; break;
                case GruntAction::GR_ATTACK: img = &m_ImgAttack_R[m_CurrentFrame % 8]; if (m_CurrentFrame >= 3 && m_CurrentFrame <= 7) imgSlash = &m_ImgSlash_R[m_CurrentFrame - 3]; break;
                case GruntAction::GR_SLASH: img = &m_ImgSlash_R[m_CurrentFrame % 5]; break;
                case GruntAction::GR_TURN: img = &m_ImgTurn_R[m_CurrentFrame % 8]; break;
                case GruntAction::GR_RUN: img = &m_ImgRun_R[m_CurrentFrame % 10]; break;
            }
        }
    }
    auto DrawImg = [&](CImage* im, float sX, float sY) {
        if (!im || im->IsNull()) return; int fw = (int)(im->GetWidth() * es * sX * mapScale), fh = (int)(im->GetHeight() * es * sY * mapScale);
        int fy = sy + (int)(m_colH * mapScale) - fh, dx = sx + (int)(m_colW * mapScale / 2) - (fw / 2);
        if (m_isFacingLeft) { int om = SetGraphicsMode(hdc, GM_ADVANCED); XFORM xo; GetWorldTransform(hdc, &xo); XFORM xl = { -1.0f, 0.0f, 0.0f, 1.0f, (float)(2 * dx + fw), 0.0f }; SetWorldTransform(hdc, &xl); im->Draw(hdc, dx, fy, fw, fh); SetWorldTransform(hdc, &xo); SetGraphicsMode(hdc, om); }
        else im->Draw(hdc, dx, fy, fw, fh);
    };
    DrawImg(img, msX, msY); if (imgSlash) DrawImg(imgSlash, 1.0f, 1.0f);
    RenderBurningEffect(hdc, mapScale, camX, camY); if (showDebugRect) RenderDebug(hdc, camX, camY, mapScale);
}

// Pomp
CImage Pomp::m_ImgIdle_R[8], Pomp::m_ImgIdle_L[8], Pomp::m_ImgWalk_R[10], Pomp::m_ImgWalk_L[10], Pomp::m_ImgAttack_R[6], Pomp::m_ImgAttack_L[6], Pomp::m_ImgBoxIdle_R[10], Pomp::m_ImgBoxIdle_L[10], Pomp::m_ImgBoxHit_R[14], Pomp::m_ImgBoxHit_L[14], Pomp::m_ImgTurn_R[6], Pomp::m_ImgTurn_L[6], Pomp::m_ImgFall_R[13], Pomp::m_ImgFall_L[13], Pomp::m_ImgHurtFly_R[2], Pomp::m_ImgHurtFly_L[2], Pomp::m_ImgHurtGround_R[15], Pomp::m_ImgHurtGround_L[15], Pomp::m_ImgRun_R[10], Pomp::m_ImgRun_L[10];
Pomp::Pomp(float x, float y) : Enemy(x, y, EnemyType::POMP) { m_ActionState = PompAction::PA_NONE; }
Pomp::~Pomp() {}
void Pomp::Reset() { Enemy::Reset(); m_ActionState = PompAction::PA_NONE; }
bool Pomp::OnTakeDamage(float kvx, float kvy, DeathCause cause) { if (m_isImmortal) return false; Enemy::OnTakeDamage(kvx, kvy, cause); m_ActionState = PompAction::PA_HURT_FLY; return false; }
void Pomp::Init() { if (!m_ImgIdle_R[0].IsNull()) return; TCHAR path[256]; for (int i = 0; i < 8; i++) { wsprintf(path, TEXT("assets/enemy/spr_pomp_idle/%d.png"), i); m_ImgIdle_R[i].Load(path); m_ImgIdle_L[i].Load(path); } for (int i = 0; i < 10; i++) { wsprintf(path, TEXT("assets/enemy/spr_pomp_walk/%d.png"), i); m_ImgWalk_R[i].Load(path); m_ImgWalk_L[i].Load(path); } for (int i = 0; i < 6; i++) { wsprintf(path, TEXT("assets/enemy/spr_pomp_attack/%d.png"), i); m_ImgAttack_R[i].Load(path); m_ImgAttack_L[i].Load(path); } for (int i = 0; i < 10; i++) { wsprintf(path, TEXT("assets/enemy/spr_pomp_box_idle/%d.png"), i); m_ImgBoxIdle_R[i].Load(path); m_ImgBoxIdle_L[i].Load(path); } for (int i = 0; i < 14; i++) { wsprintf(path, TEXT("assets/enemy/spr_pomp_box_hit/%d.png"), i); m_ImgBoxHit_R[i].Load(path); m_ImgBoxHit_L[i].Load(path); } for (int i = 0; i < 6; i++) { wsprintf(path, TEXT("assets/enemy/spr_pomp_turn/%d.png"), i); m_ImgTurn_R[i].Load(path); m_ImgTurn_L[i].Load(path); } for (int i = 0; i < 13; i++) { wsprintf(path, TEXT("assets/enemy/spr_pomp_fall/%d.png"), i); m_ImgFall_R[i].Load(path); m_ImgFall_L[i].Load(path); } for (int i = 0; i < 2; i++) { wsprintf(path, TEXT("assets/enemy/spr_pomp_hurtfly/%d.png"), i); m_ImgHurtFly_R[i].Load(path); m_ImgHurtFly_L[i].Load(path); } for (int i = 0; i < 15; i++) { wsprintf(path, TEXT("assets/enemy/spr_pomp_hurtground/%d.png"), i); m_ImgHurtGround_R[i].Load(path); m_ImgHurtGround_L[i].Load(path); } for (int i = 0; i < 10; i++) { wsprintf(path, TEXT("assets/enemy/spr_pomp_run/%d.png"), i); m_ImgRun_R[i].Destroy(); m_ImgRun_L[i].Destroy(); m_ImgRun_R[i].Load(path); m_ImgRun_L[i].Load(path); } }
void Pomp::Release() { for (int i = 0; i < 8; i++) { m_ImgIdle_R[i].Destroy(); m_ImgIdle_L[i].Destroy(); } for (int i = 0; i < 6; i++) { m_ImgAttack_R[i].Destroy(); m_ImgAttack_L[i].Destroy(); } for (int i = 0; i < 10; i++) { m_ImgBoxIdle_R[i].Destroy(); m_ImgBoxIdle_L[i].Destroy(); } for (int i = 0; i < 14; i++) { m_ImgBoxHit_R[i].Destroy(); m_ImgBoxHit_L[i].Destroy(); } for (int i = 0; i < 6; i++) { m_ImgTurn_R[i].Destroy(); m_ImgTurn_L[i].Destroy(); } for (int i = 0; i < 13; i++) { m_ImgFall_R[i].Destroy(); m_ImgFall_L[i].Destroy(); } for (int i = 0; i < 2; i++) { m_ImgHurtFly_R[i].Destroy(); m_ImgHurtFly_L[i].Destroy(); } for (int i = 0; i < 15; i++) { m_ImgHurtGround_R[i].Destroy(); m_ImgHurtGround_L[i].Destroy(); } for (int i = 0; i < 10; i++) { m_ImgRun_R[i].Destroy(); m_ImgRun_L[i].Destroy(); } }
void Pomp::Update(float ts, const Player& player) {
    if (!m_isAlive) { if (m_ActionState == PompAction::PA_HURT_FLY && m_vy == 0) { m_ActionState = PompAction::PA_HURT_GROUND; m_vx = 0; m_CurrentFrame = 0; } if (GetTickCount() - m_LastTime >= 100) { if (m_ActionState == PompAction::PA_HURT_GROUND) { if (m_CurrentFrame < 14) m_CurrentFrame++; } else m_CurrentFrame++; m_LastTime = GetTickCount(); } Enemy::Update(ts, player); return; }
    Enemy::Update(ts, player); DWORD ct = GetTickCount();
    if (m_isPlayerDetected) {
        float dx = player.GetX() - m_x; bool nextFacingLeft = (dx < 0);
        if (m_isFacingLeft != nextFacingLeft && m_ActionState != PompAction::PA_TURN) { m_ActionState = PompAction::PA_TURN; m_CurrentFrame = 0; m_patternTimer = ct; }
        m_isFacingLeft = nextFacingLeft;
        if (m_ActionState == PompAction::PA_TURN) { m_vx = 0; if (ct - m_LastTime >= (DWORD)(100.0f / ts)) { m_CurrentFrame++; m_LastTime = ct; if (m_CurrentFrame >= 6) { m_ActionState = PompAction::PA_NONE; } } }
        else if (m_ActionState == PompAction::PA_ATTACK) {
            m_vx = 0; m_State = EnemyState::ES_IDLE; if (ct - m_LastTime >= (DWORD)(100.0f / ts)) {
                m_CurrentFrame++; m_LastTime = ct;
                if (m_CurrentFrame == 3) {
                    float ex = m_x + m_colW / 2.0f, ey = m_y + m_colH / 2.0f, px = player.GetX() + player.GetColW() / 2.0f, py = player.GetY() + player.GetColH() / 2.0f;
                    float dist = (float)sqrt((px - ex) * (px - ex) + (py - ey) * (py - ey)), angle = atan2(py - ey, px - ex) * 180.0f / 3.14159f;
                    if (dist < 80.0f && (m_isFacingLeft ? (fabs(angle) >= 180.0f - m_detectAngle) : (fabs(angle) <= m_detectAngle))) { const_cast<Player&>(player).OnTakeDamage(1.0f, m_isFacingLeft ? -8.0f : 8.0f, -6.0f, ex, ey); }
                }
                if (m_CurrentFrame >= 6) { m_ActionState = PompAction::PA_NONE; m_CurrentFrame = 0; m_patternTimer = ct; }
            }
        }
        else if (fabs(dx) > 50.0f) { m_ActionState = PompAction::PA_RUN; m_State = EnemyState::ES_WALK; m_vx = m_isFacingLeft ? -10.0f : 10.0f; }
        else { if (m_ActionState == PompAction::PA_RUN) m_ActionState = PompAction::PA_NONE; m_State = EnemyState::ES_IDLE; m_vx = 0; if (ct - m_patternTimer > (DWORD)(600.0f / ts)) { m_ActionState = PompAction::PA_ATTACK; m_CurrentFrame = 0; m_patternTimer = ct; m_LastTime = ct; SoundManager::Play("SFX_SWING"); } }
    } else {
        if (m_isWaiting) { m_vx = 0.0f; m_State = EnemyState::ES_IDLE; if (ct - m_patternTimer >= (DWORD)(1000.0f / ts)) { m_isWaiting = false; m_patternTimer = ct; m_walkDistance = 0.0f; m_ActionState = PompAction::PA_TURN; m_CurrentFrame = 0; m_isFacingLeft = !m_isFacingLeft; } }
        else { 
            if (m_ActionState == PompAction::PA_TURN) { if (ct - m_LastTime >= (DWORD)(100.0f / ts)) { m_CurrentFrame++; m_LastTime = ct; if (m_CurrentFrame >= 6) { m_ActionState = PompAction::PA_NONE; m_vx = m_isFacingLeft ? -3.0f : 3.0f; } } }
            else { m_State = EnemyState::ES_WALK; m_vx = m_isFacingLeft ? -3.0f : 3.0f; float nx = m_x + m_vx * ts; bool oor = (m_isFacingLeft && nx < m_startX - m_patrolRange / 2.0f) || (!m_isFacingLeft && nx > m_startX + m_patrolRange / 2.0f); if (!CheckMapCollision(nx, m_y, m_colW, m_colH) && !oor) { m_walkDistance += (float)fabs(m_vx * ts); } else { m_isWaiting = true; m_patternTimer = ct; } }
        }
    }
    if (m_vy > 5.0f) m_State = EnemyState::ES_FALL; else if (m_vy == 0.0f && m_State == EnemyState::ES_FALL) m_State = EnemyState::ES_IDLE;
    if (ct - m_LastTime >= (DWORD)(100.0f / ts)) { m_CurrentFrame++; m_LastTime = ct; }
}
void Pomp::Render(HDC hdc, Gdiplus::Graphics* g, float camX, float camY, float mapScale, bool showDebugRect, bool isSlowMo) {
    if (!m_isAlive && m_ActionState != PompAction::PA_HURT_FLY && m_ActionState != PompAction::PA_HURT_GROUND) return;
    RenderExclaim(hdc, camX, camY, mapScale); int sx = (int)((m_x - camX) * mapScale), sy = (int)((m_y - camY) * mapScale);
    float es = 1.8f, msX = 1.0f, msY = 1.0f; CImage* img = nullptr;
    if (m_isFacingLeft) { 
        if (m_ActionState == PompAction::PA_HURT_FLY) img = &m_ImgHurtFly_L[m_CurrentFrame % 2]; else if (m_ActionState == PompAction::PA_HURT_GROUND || m_State == EnemyState::ES_DEAD) img = &m_ImgHurtGround_L[m_CurrentFrame % 15]; else if (m_State == EnemyState::ES_FALL) img = &m_ImgFall_L[m_CurrentFrame % 13];
        else { switch (m_ActionState) { case PompAction::PA_NONE: if (m_State == EnemyState::ES_IDLE || m_State == EnemyState::ES_ALERT) { img = &m_ImgIdle_L[m_CurrentFrame % 8]; msX = 1.1f; msY = 1.1f; } else if (m_State == EnemyState::ES_WALK) img = &m_ImgWalk_L[m_CurrentFrame % 10]; break; case PompAction::PA_ATTACK: img = &m_ImgAttack_L[m_CurrentFrame % 6]; break; case PompAction::PA_BOX_IDLE: img = &m_ImgBoxIdle_L[m_CurrentFrame % 10]; break; case PompAction::PA_BOX_HIT: img = &m_ImgBoxHit_L[m_CurrentFrame % 14]; break; case PompAction::PA_TURN: img = &m_ImgTurn_L[m_CurrentFrame % 6]; break; case PompAction::PA_RUN: img = &m_ImgRun_L[m_CurrentFrame % 10]; break; } }
    } else { 
        if (m_ActionState == PompAction::PA_HURT_FLY) img = &m_ImgHurtFly_R[m_CurrentFrame % 2]; else if (m_ActionState == PompAction::PA_HURT_GROUND || m_State == EnemyState::ES_DEAD) img = &m_ImgHurtGround_R[m_CurrentFrame % 15]; else if (m_State == EnemyState::ES_FALL) img = &m_ImgFall_R[m_CurrentFrame % 13];
        else { switch (m_ActionState) { case PompAction::PA_NONE: if (m_State == EnemyState::ES_IDLE || m_State == EnemyState::ES_ALERT) { img = &m_ImgIdle_R[m_CurrentFrame % 8]; msX = 1.1f; msY = 1.1f; } else if (m_State == EnemyState::ES_WALK) img = &m_ImgWalk_R[m_CurrentFrame % 10]; break; case PompAction::PA_ATTACK: img = &m_ImgAttack_R[m_CurrentFrame % 6]; break; case PompAction::PA_BOX_IDLE: img = &m_ImgBoxIdle_R[m_CurrentFrame % 10]; break; case PompAction::PA_BOX_HIT: img = &m_ImgBoxHit_R[m_CurrentFrame % 14]; break; case PompAction::PA_TURN: img = &m_ImgTurn_R[m_CurrentFrame % 6]; break; case PompAction::PA_RUN: img = &m_ImgRun_R[m_CurrentFrame % 10]; break; } }
    }
    if (img && !img->IsNull()) {
        int fw = (int)(img->GetWidth() * es * msX * mapScale), fh = (int)(img->GetHeight() * es * msY * mapScale), fy = sy + (int)(m_colH * mapScale) - fh, dx = sx + (int)(m_colW * mapScale / 2) - (fw / 2);
        if (m_isFacingLeft) { int om = SetGraphicsMode(hdc, GM_ADVANCED); XFORM xo; GetWorldTransform(hdc, &xo); XFORM xl = { -1.0f, 0.0f, 0.0f, 1.0f, (float)(2 * dx + fw), 0.0f }; SetWorldTransform(hdc, &xl); img->Draw(hdc, dx, fy, fw, fh); SetWorldTransform(hdc, &xo); SetGraphicsMode(hdc, om); }
        else img->Draw(hdc, dx, fy, fw, fh);
    }
    RenderBurningEffect(hdc, mapScale, camX, camY); if (showDebugRect) RenderDebug(hdc, camX, camY, mapScale);
}

// ShieldCop
CImage ShieldCop::m_ImgIdle_R[6], ShieldCop::m_ImgIdle_L[6], ShieldCop::m_ImgWalk_R[10], ShieldCop::m_ImgWalk_L[10], ShieldCop::m_ImgRun_R[10], ShieldCop::m_ImgRun_L[10], ShieldCop::m_ImgTurn_R[8], ShieldCop::m_ImgTurn_L[8], ShieldCop::m_ImgAim_R[19], ShieldCop::m_ImgAim_L[19], ShieldCop::m_ImgBash_R[6], ShieldCop::m_ImgBash_L[6], ShieldCop::m_ImgKnockback_R[2], ShieldCop::m_ImgKnockback_L[2], ShieldCop::m_ImgTragedyDie_R[15], ShieldCop::m_ImgTragedyDie_L[15];
ShieldCop::ShieldCop(float x, float y) : Enemy(x, y, EnemyType::SHIELDCOP) { m_ActionState = ShieldCopAction::SA_NONE; }
ShieldCop::~ShieldCop() {}
void ShieldCop::Reset() { Enemy::Reset(); m_ActionState = ShieldCopAction::SA_NONE; }
bool ShieldCop::OnTakeDamage(float kvx, float kvy, DeathCause cause) { if (m_isImmortal) return false; Enemy::OnTakeDamage(kvx, kvy, cause); m_ActionState = ShieldCopAction::SA_HURT_FLY; return false; }
void ShieldCop::Init() { if (!m_ImgIdle_R[0].IsNull()) return; TCHAR path[256]; for (int i = 0; i < 6; i++) { wsprintf(path, TEXT("assets/enemy/spr_shieldcop_idle/%d.png"), i); m_ImgIdle_R[i].Load(path); m_ImgIdle_L[i].Load(path); } for (int i = 0; i < 10; i++) { wsprintf(path, TEXT("assets/enemy/spr_shieldcop_walk/%d.png"), i); m_ImgWalk_R[i].Load(path); m_ImgWalk_L[i].Load(path); } for (int i = 0; i < 10; i++) { wsprintf(path, TEXT("assets/enemy/spr_shieldcop_run/%d.png"), i); m_ImgRun_R[i].Load(path); m_ImgRun_L[i].Load(path); } for (int i = 0; i < 8; i++) { wsprintf(path, TEXT("assets/enemy/spr_shieldcop_turn/%d.png"), i); m_ImgTurn_R[i].Load(path); m_ImgTurn_L[i].Load(path); } for (int i = 0; i < 19; i++) { wsprintf(path, TEXT("assets/enemy/spr_shieldcop_aim/%d.png"), i); m_ImgAim_R[i].Load(path); m_ImgAim_L[i].Load(path); } for (int i = 0; i < 6; i++) { wsprintf(path, TEXT("assets/enemy/spr_shieldcop_bash/%d.png"), i); m_ImgBash_R[i].Load(path); m_ImgBash_L[i].Load(path); } for (int i = 0; i < 2; i++) { wsprintf(path, TEXT("assets/enemy/spr_shieldcop_knockback/%d.png"), i); m_ImgKnockback_R[i].Load(path); m_ImgKnockback_L[i].Load(path); } for (int i = 0; i < 15; i++) { wsprintf(path, TEXT("assets/enemy/spr_shieldcop_tragedy_die_1/%d.png"), i); m_ImgTragedyDie_R[i].Load(path); m_ImgTragedyDie_L[i].Load(path); } }
void ShieldCop::Release() { for (int i = 0; i < 6; i++) { m_ImgIdle_R[i].Destroy(); m_ImgIdle_L[i].Destroy(); } for (int i = 0; i < 10; i++) { m_ImgRun_R[i].Destroy(); m_ImgRun_L[i].Destroy(); } for (int i = 0; i < 8; i++) { m_ImgTurn_R[i].Destroy(); m_ImgTurn_L[i].Destroy(); } for (int i = 0; i < 19; i++) { m_ImgAim_R[i].Destroy(); m_ImgAim_L[i].Destroy(); } for (int i = 0; i < 6; i++) { m_ImgBash_R[i].Destroy(); m_ImgBash_L[i].Destroy(); } for (int i = 0; i < 2; i++) { m_ImgKnockback_R[i].Destroy(); m_ImgKnockback_L[i].Destroy(); } for (int i = 0; i < 15; i++) { m_ImgTragedyDie_R[i].Destroy(); m_ImgTragedyDie_L[i].Destroy(); } }
void ShieldCop::Update(float ts, const Player& player) {
    if (!m_isAlive) { if (m_ActionState == ShieldCopAction::SA_HURT_FLY && m_vy == 0) { m_ActionState = ShieldCopAction::SA_HURT_GROUND; m_vx = 0; m_CurrentFrame = 0; } if (GetTickCount() - m_LastTime >= 100) { if (m_ActionState == ShieldCopAction::SA_HURT_GROUND) { if (m_CurrentFrame < 14) m_CurrentFrame++; } else m_CurrentFrame++; m_LastTime = GetTickCount(); } Enemy::Update(ts, player); return; }
    Enemy::Update(ts, player); DWORD ct = GetTickCount();
    if (m_isPlayerDetected) {
        float dx = player.GetX() - m_x; bool nextFacingLeft = (dx < 0);
        if (m_isFacingLeft != nextFacingLeft && m_ActionState != ShieldCopAction::SA_TURN) { m_ActionState = ShieldCopAction::SA_TURN; m_CurrentFrame = 0; m_patternTimer = ct; }
        m_isFacingLeft = nextFacingLeft;
        if (m_ActionState == ShieldCopAction::SA_TURN) { m_vx = 0; if (ct - m_LastTime >= (DWORD)(100.0f / ts)) { m_CurrentFrame++; m_LastTime = ct; if (m_CurrentFrame >= 8) { m_ActionState = ShieldCopAction::SA_NONE; } } }
        else if (m_ActionState == ShieldCopAction::SA_BASH) {
            m_vx = 0; m_State = EnemyState::ES_IDLE; if (ct - m_LastTime >= (DWORD)(100.0f / ts)) {
                m_CurrentFrame++; m_LastTime = ct;
                if (m_CurrentFrame == 3) {
                    float ex = m_x + m_colW / 2.0f, ey = m_y + m_colH / 2.0f, px = player.GetX() + player.GetColW() / 2.0f, py = player.GetY() + player.GetColH() / 2.0f;
                    float dist = (float)sqrt((px - ex) * (px - ex) + (py - ey) * (py - ey)), angle = atan2(py - ey, px - ex) * 180.0f / 3.14159f;
                    if (dist < 70.0f && (m_isFacingLeft ? (fabs(angle) >= 180.0f - m_detectAngle) : (fabs(angle) <= m_detectAngle))) { const_cast<Player&>(player).OnTakeDamage(1.0f, m_isFacingLeft ? -8.0f : 8.0f, -6.0f, ex, ey); }
                }
                if (m_CurrentFrame >= 6) { m_ActionState = ShieldCopAction::SA_NONE; m_CurrentFrame = 0; m_patternTimer = ct; }
            }
        }
        else if (fabs(dx) > 40.0f) { m_ActionState = ShieldCopAction::SA_RUN; m_State = EnemyState::ES_WALK; m_vx = m_isFacingLeft ? -7.0f : 7.0f; }
        else { if (m_ActionState == ShieldCopAction::SA_RUN) m_ActionState = ShieldCopAction::SA_NONE; m_State = EnemyState::ES_IDLE; m_vx = 0; if (ct - m_patternTimer > 500) { m_ActionState = ShieldCopAction::SA_BASH; m_CurrentFrame = 0; m_patternTimer = ct; } }
    } else {
        if (m_isWaiting) { m_vx = 0.0f; m_State = EnemyState::ES_IDLE; if (ct - m_patternTimer >= (DWORD)(1200.0f / ts)) { m_isWaiting = false; m_patternTimer = ct; m_walkDistance = 0.0f; m_ActionState = ShieldCopAction::SA_TURN; m_CurrentFrame = 0; m_isFacingLeft = !m_isFacingLeft; } }
        else { 
            if (m_ActionState == ShieldCopAction::SA_TURN) { if (ct - m_LastTime >= (DWORD)(100.0f / ts)) { m_CurrentFrame++; m_LastTime = ct; if (m_CurrentFrame >= 8) { m_ActionState = ShieldCopAction::SA_NONE; m_vx = m_isFacingLeft ? -2.0f : 2.0f; } } }
            else { m_State = EnemyState::ES_WALK; m_vx = m_isFacingLeft ? -2.0f : 2.0f; float nx = m_x + m_vx * ts; bool oor = (m_isFacingLeft && nx < m_startX - m_patrolRange / 2.0f) || (!m_isFacingLeft && nx > m_startX + m_patrolRange / 2.0f); if (!CheckMapCollision(nx, m_y, m_colW, m_colH) && !oor) { m_walkDistance += (float)fabs(m_vx * ts); } else { m_isWaiting = true; m_patternTimer = ct; } }
        }
    }
    if (m_vy > 0.1f) m_State = EnemyState::ES_FALL; else if (m_vy == 0.0f && m_State == EnemyState::ES_FALL) m_State = EnemyState::ES_IDLE;
    if (ct - m_LastTime >= (DWORD)(100.0f / ts)) { m_CurrentFrame++; m_LastTime = ct; }
}
void ShieldCop::Render(HDC hdc, Gdiplus::Graphics* g, float camX, float camY, float mapScale, bool showDebugRect, bool isSlowMo) {
    if (!m_isAlive && m_ActionState != ShieldCopAction::SA_HURT_FLY && m_ActionState != ShieldCopAction::SA_HURT_GROUND) return;
    RenderExclaim(hdc, camX, camY, mapScale); int sx = (int)((m_x - camX) * mapScale), sy = (int)((m_y - camY) * mapScale);
    float es = 1.8f, msX = 1.0f, msY = 1.0f; CImage* img = nullptr;
    if (m_isFacingLeft) { 
        if (m_ActionState == ShieldCopAction::SA_HURT_FLY) img = &m_ImgKnockback_L[m_CurrentFrame % 2]; else if (m_ActionState == ShieldCopAction::SA_HURT_GROUND || m_State == EnemyState::ES_DEAD) img = &m_ImgTragedyDie_L[m_CurrentFrame % 15]; else if (m_State == EnemyState::ES_FALL) img = &m_ImgIdle_L[m_CurrentFrame % 6];
        else { switch (m_ActionState) { case ShieldCopAction::SA_NONE: if (m_State == EnemyState::ES_IDLE || m_State == EnemyState::ES_ALERT) { img = &m_ImgIdle_L[m_CurrentFrame % 6]; msX = 1.1f; msY = 1.1f; } else if (m_State == EnemyState::ES_WALK) img = &m_ImgWalk_L[m_CurrentFrame % 10]; break; case ShieldCopAction::SA_AIM: img = &m_ImgAim_L[m_CurrentFrame % 19]; break; case ShieldCopAction::SA_BASH: img = &m_ImgBash_L[m_CurrentFrame % 6]; break; case ShieldCopAction::SA_TURN: img = &m_ImgTurn_L[m_CurrentFrame % 8]; break; case ShieldCopAction::SA_RUN: img = &m_ImgRun_L[m_CurrentFrame % 10]; break; } }
    } else { 
        if (m_ActionState == ShieldCopAction::SA_HURT_FLY) img = &m_ImgKnockback_R[m_CurrentFrame % 2]; else if (m_ActionState == ShieldCopAction::SA_HURT_GROUND || m_State == EnemyState::ES_DEAD) img = &m_ImgTragedyDie_R[m_CurrentFrame % 15]; else if (m_State == EnemyState::ES_FALL) img = &m_ImgIdle_R[m_CurrentFrame % 6];
        else { switch (m_ActionState) { case ShieldCopAction::SA_NONE: if (m_State == EnemyState::ES_IDLE || m_State == EnemyState::ES_ALERT) { img = &m_ImgIdle_R[m_CurrentFrame % 6]; msX = 1.1f; msY = 1.1f; } else if (m_State == EnemyState::ES_WALK) img = &m_ImgWalk_R[m_CurrentFrame % 10]; break; case ShieldCopAction::SA_AIM: img = &m_ImgAim_R[m_CurrentFrame % 19]; break; case ShieldCopAction::SA_BASH: img = &m_ImgBash_R[m_CurrentFrame % 6]; break; case ShieldCopAction::SA_TURN: img = &m_ImgTurn_R[m_CurrentFrame % 8]; break; case ShieldCopAction::SA_RUN: img = &m_ImgRun_R[m_CurrentFrame % 10]; break; } }
    }
    if (img && !img->IsNull()) {
        int fw = (int)(img->GetWidth() * es * msX * mapScale), fh = (int)(img->GetHeight() * es * msY * mapScale), fy = sy + (int)(m_colH * mapScale) - fh, dx = sx + (int)(m_colW * mapScale / 2) - (fw / 2);
        if (m_isFacingLeft) { int om = SetGraphicsMode(hdc, GM_ADVANCED); XFORM xo; GetWorldTransform(hdc, &xo); XFORM xl = { -1.0f, 0.0f, 0.0f, 1.0f, (float)(2 * dx + fw), 0.0f }; SetWorldTransform(hdc, &xl); img->Draw(hdc, dx, fy, fw, fh); SetWorldTransform(hdc, &xo); SetGraphicsMode(hdc, om); }
        else img->Draw(hdc, dx, fy, fw, fh);
    }
    RenderBurningEffect(hdc, mapScale, camX, camY); if (showDebugRect) RenderDebug(hdc, camX, camY, mapScale);
}
