#include "Enemy.h"
#include "Player.h"
#include "Physics.h"
#include "Bullet.h"
#include "../SceneAndMap/Camera.h"
#include "../Effects/EffectManager.h"
#include <gdiplus.h>
#include <map>

CImage Enemy::m_ImgExclaim[2];

Enemy::Enemy(float startX, float startY, EnemyType type, float patrolRange) {
    m_startX = startX; m_startY = startY; m_x = startX; m_y = startY;
    m_vx = 2.0f; m_vy = 0.0f; m_colW = 40.0f; m_colH = 60.0f;
    m_isAlive = true; m_isFacingLeft = false; m_Type = type; m_State = EnemyState::ES_IDLE;
    m_CurrentFrame = 0; m_LastTime = GetTickCount(); m_friction = 0.96f; m_knockbackVx = 0.0f;
    m_isImmortal = false; m_patternTimer = GetTickCount(); m_isWaiting = false; m_walkDistance = 0.0f;
    m_patrolRange = patrolRange;
    m_isPlayerDetected = false; m_alertStartTime = 0; m_exclaimFrame = 0;
    m_bloodDistance = 0.0f;
}
Enemy::~Enemy() {}

bool Enemy::IsPlayerInCone(float px, float py, float pw, float ph) {
    float ex = m_x + m_colW / 2.0f, ey = m_y + m_colH / 2.0f;
    float pcx = px + pw / 2.0f, pcy = py + ph / 2.0f;
    float dx = pcx - ex, dy = pcy - ey, dist = (float)sqrt(dx * dx + dy * dy);
    if (dist > m_detectRange) return false;

    float angle = atan2(dy, dx) * 180.0f / 3.14159f;
    float absAngle = (float)fabs(angle);
    if (m_isFacingLeft) {
        if (absAngle >= 180.0f - m_detectAngle) return true; // Left cone
    } else {
        if (absAngle <= m_detectAngle) return true; // Right cone
    }
    return false;
}

void Enemy::UpdateDetection(float px, float py, float pw, float ph, float ts) {
    if (!m_isAlive) return;
    if (!m_isPlayerDetected) {
        if (IsPlayerInCone(px, py, pw, ph)) {
            m_isPlayerDetected = true;
            m_alertStartTime = GetTickCount();
            m_exclaimFrame = 0;
            m_State = EnemyState::ES_ALERT;
        }
    } else {
        DWORD ct = GetTickCount();
        if (m_exclaimFrame == 0 && ct - m_alertStartTime > (DWORD)(100.0f / ts)) {
            m_exclaimFrame = 1;
        }
    }
}

void Enemy::RenderExclaim(HDC hdc, float camX, float camY, float mapScale) {
    if (!m_isPlayerDetected || !m_isAlive) return;
    if (m_ImgExclaim[m_exclaimFrame].IsNull()) {
        TCHAR p[256];
        wsprintf(p, TEXT("assets/enemy/spr_enemy_follow/spr_enemy_follow_%d.png"), 0); m_ImgExclaim[0].Load(p);
        wsprintf(p, TEXT("assets/enemy/spr_enemy_follow/spr_enemy_follow_%d.png"), 1); m_ImgExclaim[1].Load(p);
    }
    if (!m_ImgExclaim[m_exclaimFrame].IsNull()) {
        int ew = (int)(m_ImgExclaim[m_exclaimFrame].GetWidth() * 2.0f * mapScale);
        int eh = (int)(m_ImgExclaim[m_exclaimFrame].GetHeight() * 2.0f * mapScale);
        int ex = (int)((m_x + m_colW / 2.0f - camX) * mapScale) - ew / 2;
        int ey = (int)((m_y - camY) * mapScale) - eh - 10;
        m_ImgExclaim[m_exclaimFrame].Draw(hdc, ex, ey, ew, eh);
    }
}

void Enemy::RenderDetectionRange(HDC hdc, float camX, float camY, float mapScale) {
    if (!m_isAlive) return;

    int ex = (int)((m_x + m_colW / 2.0f - camX) * mapScale);
    int ey = (int)((m_y + m_colH / 2.0f - camY) * mapScale);
    int r = (int)(m_detectRange * mapScale);

    HPEN hRedPen = CreatePen(PS_SOLID, 1, RGB(255, 0, 0));
    HPEN hOldPen = (HPEN)SelectObject(hdc, hRedPen);
    HBRUSH hRedBrush = CreateSolidBrush(RGB(255, 0, 0));
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hRedBrush);

    auto DrawCone = [&](float centralAngle) {
        float startAngle = (centralAngle - m_detectAngle) * 3.14159f / 180.0f;
        float endAngle = (centralAngle + m_detectAngle) * 3.14159f / 180.0f;
        int x1 = ex + (int)(100 * cos(endAngle));
        int y1 = ey + (int)(100 * sin(endAngle));
        int x2 = ex + (int)(100 * cos(startAngle));
        int y2 = ey + (int)(100 * sin(startAngle));
        Pie(hdc, ex - r, ey - r, ex + r, ey + r, x1, y1, x2, y2);
    };

    int oldMode = SetROP2(hdc, R2_COPYPEN);
    DrawCone(m_isFacingLeft ? 180.0f : 0.0f);
    SetROP2(hdc, oldMode);

    SelectObject(hdc, hOldBrush);
    SelectObject(hdc, hOldPen);
    DeleteObject(hRedPen); DeleteObject(hRedBrush);
}

void Enemy::RenderDebug(HDC hdc, float camX, float camY, float mapScale) {
    int sx = (int)((m_x - camX) * mapScale), sy = (int)((m_y - camY) * mapScale);
    HBRUSH redBrush = CreateSolidBrush(RGB(255, 0, 0));
    RECT rect = { sx, sy, sx + (int)(m_colW * mapScale), sy + (int)(m_colH * mapScale) };
    FrameRect(hdc, &rect, redBrush); DeleteObject(redBrush);
    RenderDetectionRange(hdc, camX, camY, mapScale);
}

void Enemy::ReleaseAll() { 
    Gangster::Release(); Grunt::Release(); Pomp::Release(); ShieldCop::Release(); 
    if (!m_ImgExclaim[0].IsNull()) m_ImgExclaim[0].Destroy();
    if (!m_ImgExclaim[1].IsNull()) m_ImgExclaim[1].Destroy();
}

void Enemy::Reset() {
    m_x = m_startX; m_y = m_startY; m_vx = 2.0f; m_vy = 0.0f; m_isAlive = true; m_isFacingLeft = false; m_State = EnemyState::ES_IDLE;
    m_CurrentFrame = 0; m_LastTime = GetTickCount(); m_knockbackVx = 0.0f; m_patternTimer = GetTickCount(); m_isWaiting = false; m_walkDistance = 0.0f; m_isPlayerDetected = false;
    m_bloodDistance = 0.0f; m_alertStartTime = 0; m_exclaimFrame = 0;
}

void Enemy::OnTakeDamage(float kvx, float kvy) { if (!m_isImmortal) { m_isAlive = false; m_State = EnemyState::ES_DEAD; m_vx = kvx; m_vy = kvy; m_CurrentFrame = 0; m_bloodDistance = 0.0f; } }
void Enemy::ApplyKnockback(float vx) { m_knockbackVx = vx; m_vx = vx; }

void Enemy::Update(float ts, const Player& player) {
    if (m_vx != 0.0f) { 
        float nx = m_x + m_vx * ts; 
        if (!CheckMapCollision(nx, m_y, m_colW, m_colH)) {
            if (!m_isAlive) {
                m_bloodDistance += (float)fabs(m_vx * ts);
                if (m_bloodDistance >= 15.0f) {
                    m_bloodDistance -= 15.0f;
                    float length = (float)sqrt(m_vx * m_vx + m_vy * m_vy);
                    if (length > 0) {
                        float nvx = m_vx / length; float nvy = m_vy / length;
                        float perpX1 = -nvy; float perpY1 = nvx;
                        float perpX2 = nvy; float perpY2 = -nvx;
                        float angle1 = atan2(perpY1, perpX1); float angle2 = atan2(perpY2, perpX2);
                        float speed1 = 2.0f + (rand() % 30) / 10.0f; float speed2 = 2.0f + (rand() % 30) / 10.0f;
                        EffectManager::AddBloodSplatter(m_x + m_colW / 2.0f, m_y + m_colH / 2.0f, perpX1 * speed1 + ((rand() % 100) / 100.0f - 0.5f), perpY1 * speed1 + ((rand() % 100) / 100.0f - 0.5f), angle1, GetTickCount());
                        EffectManager::AddBloodSplatter(m_x + m_colW / 2.0f, m_y + m_colH / 2.0f, perpX2 * speed2 + ((rand() % 100) / 100.0f - 0.5f), perpY2 * speed2 + ((rand() % 100) / 100.0f - 0.5f), angle2, GetTickCount());
                    }
                }
            }
            m_x = nx; 
        } else {
            if (!m_isAlive) m_vx = -m_vx * 0.5f; // Bounce if dead
            else m_vx = 0.0f; 
        }
        if (!m_isAlive) m_vx *= m_friction; 
    }

    float gravity = m_isAlive ? 1.5f : 1.0f;
    m_vy += gravity * ts; if (m_vy > 30.0f) m_vy = 30.0f;
    
    float nextY = m_y + m_vy * ts; 
    if (CheckMapCollision(m_x, nextY, m_colW, m_colH)) {
        if (!m_isAlive && fabs(m_vy) > 2.0f) m_vy = -m_vy * 0.3f; // Bounce
        else m_vy = 0.0f;
    } else {
        m_y = nextY;
    }

    UpdateDetection(player.GetX(), player.GetY(), player.GetColW(), player.GetColH(), ts);
}

// Gangster
CImage Gangster::m_ImgIdle_R[8], Gangster::m_ImgIdle_L[8], Gangster::m_ImgWalk_R[8], Gangster::m_ImgWalk_L[8], Gangster::m_ImgAim_R[4], Gangster::m_ImgAim_L[4], Gangster::m_ImgTurn_R[6], Gangster::m_ImgTurn_L[6], Gangster::m_ImgFall_R[12], Gangster::m_ImgFall_L[12], Gangster::m_ImgHurtFly_R[2], Gangster::m_ImgHurtFly_L[2], Gangster::m_ImgHurtGround_R[14], Gangster::m_ImgHurtGround_L[14], Gangster::m_ImgRun_R[10], Gangster::m_ImgRun_L[10], Gangster::m_ImgGun_R[2], Gangster::m_ImgGun_L[2], Gangster::m_ImgArm[2];
Gangster::Gangster(float x, float y) : Enemy(x, y, EnemyType::GANGSTER) { m_ActionState = GangsterAction::GA_NONE; }
Gangster::~Gangster() {}
void Gangster::Reset() { Enemy::Reset(); m_ActionState = GangsterAction::GA_NONE; }
void Gangster::OnTakeDamage(float kvx, float kvy) { if (m_isImmortal) return; Enemy::OnTakeDamage(kvx, kvy); m_ActionState = GangsterAction::GA_HURT_FLY; }
void Gangster::Init() { if (!m_ImgIdle_R[0].IsNull()) return; TCHAR p[256]; for (int i = 0; i < 8; i++) { wsprintf(p, TEXT("assets/enemy/spr_gangsteridle/%d.png"), i); m_ImgIdle_R[i].Load(p); m_ImgIdle_L[i].Load(p); } for (int i = 0; i < 8; i++) { wsprintf(p, TEXT("assets/enemy/spr_gangsterwalk/%d.png"), i); m_ImgWalk_R[i].Load(p); m_ImgWalk_L[i].Load(p); } for (int i = 0; i < 4; i++) { wsprintf(p, TEXT("assets/enemy/spr_gangster_aim/%d.png"), i); m_ImgAim_R[i].Load(p); m_ImgAim_L[i].Load(p); } for (int i = 0; i < 6; i++) { wsprintf(p, TEXT("assets/enemy/spr_gangsterturn/%d.png"), i); m_ImgTurn_R[i].Load(p); m_ImgTurn_L[i].Load(p); } for (int i = 0; i < 12; i++) { wsprintf(p, TEXT("assets/enemy/spr_gangsterfall/%d.png"), i); m_ImgFall_R[i].Load(p); m_ImgFall_L[i].Load(p); } for (int i = 0; i < 2; i++) { wsprintf(p, TEXT("assets/enemy/spr_gangsterhurtfly/%d.png"), i); m_ImgHurtFly_R[i].Load(p); m_ImgHurtFly_L[i].Load(p); } for (int i = 0; i < 14; i++) { wsprintf(p, TEXT("assets/enemy/spr_gangsterhurtground/%d.png"), i); m_ImgHurtGround_R[i].Load(p); m_ImgHurtGround_L[i].Load(p); } for (int i = 0; i < 10; i++) { wsprintf(p, TEXT("assets/enemy/spr_gangsterrun/%d.png"), i); m_ImgRun_R[i].Load(p); m_ImgRun_L[i].Load(p); } for (int i = 0; i < 2; i++) { wsprintf(p, TEXT("assets/enemy/spr_gangstergun/%d.png"), i); m_ImgGun_R[i].Load(p); m_ImgGun_L[i].Load(p); } for (int i = 0; i < 2; i++) { wsprintf(p, TEXT("assets/enemy/spr_arm/%d.png"), i); m_ImgArm[i].Load(p); } }
void Gangster::Release() { for (int i = 0; i < 8; i++) { m_ImgIdle_R[i].Destroy(); m_ImgIdle_L[i].Destroy(); } for (int i = 0; i < 8; i++) { m_ImgWalk_R[i].Destroy(); m_ImgWalk_L[i].Destroy(); } for (int i = 0; i < 4; i++) { m_ImgAim_R[i].Destroy(); m_ImgAim_L[i].Destroy(); } for (int i = 0; i < 6; i++) { m_ImgTurn_R[i].Destroy(); m_ImgTurn_L[i].Destroy(); } for (int i = 0; i < 12; i++) { m_ImgFall_R[i].Destroy(); m_ImgFall_L[i].Destroy(); } for (int i = 0; i < 2; i++) { m_ImgHurtFly_R[i].Destroy(); m_ImgHurtFly_L[i].Destroy(); } for (int i = 0; i < 14; i++) { m_ImgHurtGround_R[i].Destroy(); m_ImgHurtGround_L[i].Destroy(); } for (int i = 0; i < 10; i++) { m_ImgRun_R[i].Destroy(); m_ImgRun_L[i].Destroy(); } for (int i = 0; i < 2; i++) { m_ImgGun_R[i].Destroy(); m_ImgGun_L[i].Destroy(); } for (int i = 0; i < 2; i++) { m_ImgArm[i].Destroy(); } }
void Gangster::Update(float ts, const Player& player) {
    if (!m_isAlive) { if (m_ActionState == GangsterAction::GA_HURT_FLY && m_vy == 0) { m_ActionState = GangsterAction::GA_HURT_GROUND; m_vx = 0; m_CurrentFrame = 0; } if (GetTickCount() - m_LastTime >= 100) { if (m_ActionState == GangsterAction::GA_HURT_GROUND) { if (m_CurrentFrame < 13) m_CurrentFrame++; } else m_CurrentFrame++; m_LastTime = GetTickCount(); } Enemy::Update(ts, player); return; }
    Enemy::Update(ts, player); DWORD ct = GetTickCount();
    if (m_isPlayerDetected) {
        float dx = player.GetX() - m_x; m_isFacingLeft = (dx < 0);
        if (m_ActionState == GangsterAction::GA_NONE || m_ActionState == GangsterAction::GA_RUN) {
            if (fabs(dx) > 250.0f) { m_State = EnemyState::ES_WALK; m_vx = m_isFacingLeft ? -4.0f : 4.0f; }
            else { m_State = EnemyState::ES_IDLE; m_vx = 0; m_ActionState = GangsterAction::GA_AIM; m_CurrentFrame = 0; m_patternTimer = ct; m_LastTime = ct; }
        } else if (m_ActionState == GangsterAction::GA_AIM) {
            m_vx = 0; m_State = EnemyState::ES_IDLE;
            if (ct - m_LastTime >= (DWORD)(100.0f / ts)) {
                if (m_CurrentFrame < 3) m_CurrentFrame++;
                else { if (ct - m_patternTimer > (DWORD)(800.0f / ts)) { m_ActionState = GangsterAction::GA_FIRE; m_CurrentFrame = 0; m_patternTimer = ct; } }
                m_LastTime = ct;
            }
        } else if (m_ActionState == GangsterAction::GA_FIRE) {
            m_vx = 0; m_State = EnemyState::ES_IDLE;
            if (ct - m_LastTime >= (DWORD)(80.0f / ts)) {
                if (m_CurrentFrame < 5) { m_CurrentFrame++; if (m_CurrentFrame == 2) { Bullet::AddBullet(m_x + (m_isFacingLeft ? -15.0f : 55.0f), m_y + 30.0f, m_isFacingLeft ? -20.0f : 20.0f, 0.0f); } }
                else { m_ActionState = GangsterAction::GA_NONE; m_patternTimer = ct; m_CurrentFrame = 0; }
                m_LastTime = ct;
            }
        }
    } else {
        if (m_isWaiting) { m_vx = 0.0f; m_State = EnemyState::ES_IDLE; if (ct - m_patternTimer >= (DWORD)(1000.0f / ts)) { m_isWaiting = false; m_patternTimer = ct; m_walkDistance = 0.0f; m_isFacingLeft = !m_isFacingLeft; m_vx = m_isFacingLeft ? -2.0f : 2.0f; } }
        else { m_State = EnemyState::ES_WALK; m_vx = m_isFacingLeft ? -2.0f : 2.0f; float nx = m_x + m_vx * ts; bool oor = (m_isFacingLeft && nx < m_startX - m_patrolRange / 2.0f) || (!m_isFacingLeft && nx > m_startX + m_patrolRange / 2.0f); if (!CheckMapCollision(nx, m_y, m_colW, m_colH) && !oor) { m_walkDistance += (float)fabs(m_vx * ts); } else { m_isWaiting = true; m_patternTimer = ct; } }
    }
    if (m_vy > 5.0f) m_State = EnemyState::ES_FALL; else if (m_vy == 0.0f && m_State == EnemyState::ES_FALL) m_State = EnemyState::ES_IDLE;
    if (ct - m_LastTime >= (DWORD)(100.0f / ts)) { m_CurrentFrame++; m_LastTime = ct; }
}
void Gangster::Render(HDC hdc, Gdiplus::Graphics* g, float camX, float camY, float mapScale, bool showDebugRect, bool isSlowMo) {
    if (!m_isAlive && m_ActionState != GangsterAction::GA_HURT_FLY && m_ActionState != GangsterAction::GA_HURT_GROUND) return;
    RenderExclaim(hdc, camX, camY, mapScale); int sx = (int)((m_x - camX) * mapScale), sy = (int)((m_y - camY) * mapScale);
    CImage *imgBody = nullptr, *imgGun = nullptr, *imgArm = nullptr; float msX = 1.0f, msY = 1.0f, es = 1.8f;
    if (m_isFacingLeft) {
        imgArm = &m_ImgArm[1];
        if (m_ActionState == GangsterAction::GA_HURT_FLY) { imgBody = &m_ImgHurtFly_L[m_CurrentFrame % 2]; imgArm = nullptr; }
        else if (m_ActionState == GangsterAction::GA_HURT_GROUND || m_State == EnemyState::ES_DEAD) { imgBody = &m_ImgHurtGround_L[m_CurrentFrame % 14]; imgArm = nullptr; }
        else if (m_State == EnemyState::ES_FALL) { imgBody = &m_ImgFall_L[m_CurrentFrame % 12]; imgArm = nullptr; }
        else {
            switch (m_ActionState) {
            case GangsterAction::GA_NONE: if (m_State == EnemyState::ES_IDLE || m_State == EnemyState::ES_ALERT) { imgBody = &m_ImgIdle_L[m_CurrentFrame % 8]; msX = 1.1f; msY = 1.1f; } else if (m_State == EnemyState::ES_WALK) imgBody = &m_ImgWalk_L[m_CurrentFrame % 8]; break;
            case GangsterAction::GA_AIM: imgBody = &m_ImgAim_L[m_CurrentFrame % 4]; imgGun = &m_ImgGun_L[0]; break;
            case GangsterAction::GA_FIRE: imgBody = &m_ImgAim_L[3]; imgGun = &m_ImgGun_L[m_CurrentFrame == 2 ? 1 : 0]; break;
            case GangsterAction::GA_TURN: imgBody = &m_ImgTurn_L[m_CurrentFrame % 6]; break;
            case GangsterAction::GA_RUN: imgBody = &m_ImgRun_L[m_CurrentFrame % 10]; break;
            }
        }
    } else {
        imgArm = &m_ImgArm[0];
        if (m_ActionState == GangsterAction::GA_HURT_FLY) { imgBody = &m_ImgHurtFly_R[m_CurrentFrame % 2]; imgArm = nullptr; }
        else if (m_ActionState == GangsterAction::GA_HURT_GROUND || m_State == EnemyState::ES_DEAD) { imgBody = &m_ImgHurtGround_R[m_CurrentFrame % 14]; imgArm = nullptr; }
        else if (m_State == EnemyState::ES_FALL) { imgBody = &m_ImgFall_R[m_CurrentFrame % 12]; imgArm = nullptr; }
        else {
            switch (m_ActionState) {
            case GangsterAction::GA_NONE: if (m_State == EnemyState::ES_IDLE || m_State == EnemyState::ES_ALERT) { imgBody = &m_ImgIdle_R[m_CurrentFrame % 8]; msX = 1.1f; msY = 1.1f; } else if (m_State == EnemyState::ES_WALK) imgBody = &m_ImgWalk_R[m_CurrentFrame % 8]; break;
            case GangsterAction::GA_AIM: imgBody = &m_ImgAim_R[m_CurrentFrame % 4]; imgGun = &m_ImgGun_R[0]; break;
            case GangsterAction::GA_FIRE: imgBody = &m_ImgAim_R[3]; imgGun = &m_ImgGun_R[m_CurrentFrame == 2 ? 1 : 0]; break;
            case GangsterAction::GA_TURN: imgBody = &m_ImgTurn_R[m_CurrentFrame % 6]; break;
            case GangsterAction::GA_RUN: imgBody = &m_ImgRun_R[m_CurrentFrame % 10]; break;
            }
        }
    }
    auto DrawImg = [&](CImage* im, float sX, float sY, float oX = 0, float oY = 0, bool forceNoFlip = false) {
        if (!im || im->IsNull()) return; int fw = (int)(im->GetWidth() * es * sX * mapScale), fh = (int)(im->GetHeight() * es * sY * mapScale);
        int fy = sy + (int)(m_colH * mapScale) - fh + (int)(oY * mapScale), dx = sx + (int)(m_colW * mapScale / 2) - (fw / 2) + (int)(oX * mapScale);
        if (m_isFacingLeft && !forceNoFlip) { int om = SetGraphicsMode(hdc, GM_ADVANCED); XFORM xo; GetWorldTransform(hdc, &xo); XFORM xl = { -1.0f, 0.0f, 0.0f, 1.0f, (float)(2 * dx + fw), 0.0f }; SetWorldTransform(hdc, &xl); im->Draw(hdc, dx, fy, fw, fh); SetWorldTransform(hdc, &xo); SetGraphicsMode(hdc, om); }
        else im->Draw(hdc, dx, fy, fw, fh);
    };
    DrawImg(imgBody, msX, msY); float gX = m_isFacingLeft ? -10.0f : 10.0f, gY = -30.0f;
    if (imgArm && (m_ActionState == GangsterAction::GA_AIM || m_ActionState == GangsterAction::GA_FIRE)) DrawImg(imgArm, 1.0f, 1.0f, gX, gY, true);
    if (imgGun) DrawImg(imgGun, 1.0f, 1.0f, gX, gY);
    if (showDebugRect) RenderDebug(hdc, camX, camY, mapScale);
}

// Grunt
CImage Grunt::m_ImgIdle_R[8], Grunt::m_ImgIdle_L[8], Grunt::m_ImgWalk_R[10], Grunt::m_ImgWalk_L[10], Grunt::m_ImgAttack_R[8], Grunt::m_ImgAttack_L[8], Grunt::m_ImgSlash_R[5], Grunt::m_ImgSlash_L[5], Grunt::m_ImgTurn_R[8], Grunt::m_ImgTurn_L[8], Grunt::m_ImgFall_R[13], Grunt::m_ImgFall_L[13], Grunt::m_ImgHurtFly_R[2], Grunt::m_ImgHurtFly_L[2], Grunt::m_ImgHurtGround_R[16], Grunt::m_ImgHurtGround_L[16], Grunt::m_ImgRun_R[10], Grunt::m_ImgRun_L[10];
Grunt::Grunt(float x, float y) : Enemy(x, y, EnemyType::GRUNT) { m_ActionState = GruntAction::GR_NONE; }
Grunt::~Grunt() {}
void Grunt::Reset() { Enemy::Reset(); m_ActionState = GruntAction::GR_NONE; }
void Grunt::OnTakeDamage(float kvx, float kvy) { if (m_isImmortal) return; Enemy::OnTakeDamage(kvx, kvy); m_ActionState = GruntAction::GR_HURT_FLY; }
void Grunt::Init() { if (!m_ImgIdle_R[0].IsNull()) return; TCHAR path[256]; for (int i = 0; i < 8; i++) { wsprintf(path, TEXT("assets/enemy/spr_grunt_idle/%d.png"), i); m_ImgIdle_R[i].Load(path); m_ImgIdle_L[i].Load(path); } for (int i = 0; i < 10; i++) { wsprintf(path, TEXT("assets/enemy/spr_grunt_walk/%d.png"), i); m_ImgWalk_R[i].Load(path); m_ImgWalk_L[i].Load(path); } for (int i = 0; i < 8; i++) { wsprintf(path, TEXT("assets/enemy/spr_grunt_attack/%d.png"), i); m_ImgAttack_R[i].Load(path); m_ImgAttack_L[i].Load(path); } for (int i = 0; i < 5; i++) { wsprintf(path, TEXT("assets/enemy/spr_gruntslash/%d.png"), i); m_ImgSlash_R[i].Load(path); m_ImgSlash_L[i].Load(path); } for (int i = 0; i < 8; i++) { wsprintf(path, TEXT("assets/enemy/spr_grunt_turn/%d.png"), i); m_ImgTurn_R[i].Load(path); m_ImgTurn_L[i].Load(path); } for (int i = 0; i < 13; i++) { wsprintf(path, TEXT("assets/enemy/spr_grunt_fall/%d.png"), i); m_ImgFall_R[i].Load(path); m_ImgFall_L[i].Load(path); } for (int i = 0; i < 2; i++) { wsprintf(path, TEXT("assets/enemy/spr_grunt_hurtfly/%d.png"), i); m_ImgHurtFly_R[i].Load(path); m_ImgHurtFly_L[i].Load(path); } for (int i = 0; i < 16; i++) { wsprintf(path, TEXT("assets/enemy/spr_grunt_hurtground/%d.png"), i); m_ImgHurtGround_R[i].Load(path); m_ImgHurtGround_L[i].Load(path); } for (int i = 0; i < 10; i++) { wsprintf(path, TEXT("assets/enemy/spr_grunt_run/%d.png"), i); m_ImgRun_R[i].Load(path); m_ImgRun_L[i].Load(path); } }
void Grunt::Release() { for (int i = 0; i < 8; i++) { m_ImgIdle_R[i].Destroy(); m_ImgIdle_L[i].Destroy(); } for (int i = 0; i < 10; i++) { m_ImgWalk_R[i].Destroy(); m_ImgWalk_L[i].Destroy(); } for (int i = 0; i < 8; i++) { m_ImgAttack_R[i].Destroy(); m_ImgAttack_L[i].Destroy(); } for (int i = 0; i < 5; i++) { m_ImgSlash_R[i].Destroy(); m_ImgSlash_L[i].Destroy(); } for (int i = 0; i < 8; i++) { m_ImgTurn_R[i].Destroy(); m_ImgTurn_L[i].Destroy(); } for (int i = 0; i < 13; i++) { m_ImgFall_R[i].Destroy(); m_ImgFall_L[i].Destroy(); } for (int i = 0; i < 2; i++) { m_ImgHurtFly_R[i].Destroy(); m_ImgHurtFly_L[i].Destroy(); } for (int i = 0; i < 16; i++) { m_ImgHurtGround_R[i].Destroy(); m_ImgHurtGround_L[i].Destroy(); } for (int i = 0; i < 10; i++) { m_ImgRun_R[i].Destroy(); m_ImgRun_L[i].Destroy(); } }
void Grunt::Update(float ts, const Player& player) {
    if (!m_isAlive) { if (m_ActionState == GruntAction::GR_HURT_FLY && m_vy == 0) { m_ActionState = GruntAction::GR_HURT_GROUND; m_vx = 0; m_CurrentFrame = 0; } if (GetTickCount() - m_LastTime >= 100) { if (m_ActionState == GruntAction::GR_HURT_GROUND) { if (m_CurrentFrame < 15) m_CurrentFrame++; } else m_CurrentFrame++; m_LastTime = GetTickCount(); } Enemy::Update(ts, player); return; }
    Enemy::Update(ts, player); DWORD ct = GetTickCount();
    if (m_isPlayerDetected) {
        float dx = player.GetX() - m_x; m_isFacingLeft = (dx < 0);
        if (m_ActionState == GruntAction::GR_ATTACK) {
            m_vx = 0; m_State = EnemyState::ES_IDLE; if (ct - m_LastTime >= (DWORD)(100.0f / ts)) {
                m_CurrentFrame++; m_LastTime = ct;
                if (m_CurrentFrame == 5) {
                    float ex = m_x + m_colW / 2.0f, ey = m_y + m_colH / 2.0f;
                    float px = player.GetX() + player.GetColW() / 2.0f, py = player.GetY() + player.GetColH() / 2.0f;
                    float dist = (float)sqrt((px - ex) * (px - ex) + (py - ey) * (py - ey));
                    float angle = atan2(py - ey, px - ex) * 180.0f / 3.14159f;
                    float absAngle = (float)fabs(angle);
                    if (dist < 100.0f && (m_isFacingLeft ? (absAngle >= 180.0f - m_detectAngle) : (absAngle <= m_detectAngle))) const_cast<Player&>(player).OnTakeDamage(1.0f);
                }
                if (m_CurrentFrame >= 8) { m_ActionState = GruntAction::GR_NONE; m_CurrentFrame = 0; m_patternTimer = ct; }
            }
        } else if (fabs(dx) > 60.0f) { m_State = EnemyState::ES_WALK; m_vx = m_isFacingLeft ? -5.0f : 5.0f; }
        else { m_State = EnemyState::ES_IDLE; m_vx = 0; if (ct - m_patternTimer > (DWORD)(800.0f / ts)) { m_ActionState = GruntAction::GR_ATTACK; m_CurrentFrame = 0; m_patternTimer = ct; m_LastTime = ct; } }
    } else {
        if (m_isWaiting) { m_vx = 0.0f; m_State = EnemyState::ES_IDLE; if (ct - m_patternTimer >= (DWORD)(1000.0f / ts)) { m_isWaiting = false; m_patternTimer = ct; m_walkDistance = 0.0f; m_isFacingLeft = !m_isFacingLeft; m_vx = m_isFacingLeft ? -2.0f : 2.0f; } }
        else { m_State = EnemyState::ES_WALK; m_vx = m_isFacingLeft ? -2.5f : 2.5f; float nx = m_x + m_vx * ts; bool oor = (m_isFacingLeft && nx < m_startX - m_patrolRange / 2.0f) || (!m_isFacingLeft && nx > m_startX + m_patrolRange / 2.0f); if (!CheckMapCollision(nx, m_y, m_colW, m_colH) && !oor) { m_walkDistance += (float)fabs(m_vx * ts); } else { m_isWaiting = true; m_patternTimer = ct; } }
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
    if (showDebugRect) RenderDebug(hdc, camX, camY, mapScale);
}

// Pomp
CImage Pomp::m_ImgIdle_R[8], Pomp::m_ImgIdle_L[8], Pomp::m_ImgWalk_R[10], Pomp::m_ImgWalk_L[10], Pomp::m_ImgAttack_R[6], Pomp::m_ImgAttack_L[6], Pomp::m_ImgBoxIdle_R[10], Pomp::m_ImgBoxIdle_L[10], Pomp::m_ImgBoxHit_R[14], Pomp::m_ImgBoxHit_L[14], Pomp::m_ImgTurn_R[6], Pomp::m_ImgTurn_L[6], Pomp::m_ImgFall_R[13], Pomp::m_ImgFall_L[13], Pomp::m_ImgHurtFly_R[2], Pomp::m_ImgHurtFly_L[2], Pomp::m_ImgHurtGround_R[15], Pomp::m_ImgHurtGround_L[15], Pomp::m_ImgRun_R[10], Pomp::m_ImgRun_L[10];
Pomp::Pomp(float x, float y) : Enemy(x, y, EnemyType::POMP) { m_ActionState = PompAction::PA_NONE; }
Pomp::~Pomp() {}
void Pomp::Reset() { Enemy::Reset(); m_ActionState = PompAction::PA_NONE; }
void Pomp::OnTakeDamage(float kvx, float kvy) { if (m_isImmortal) return; Enemy::OnTakeDamage(kvx, kvy); m_ActionState = PompAction::PA_HURT_FLY; }
void Pomp::Init() { if (!m_ImgIdle_R[0].IsNull()) return; TCHAR path[256]; for (int i = 0; i < 8; i++) { wsprintf(path, TEXT("assets/enemy/spr_pomp_idle/%d.png"), i); m_ImgIdle_R[i].Load(path); m_ImgIdle_L[i].Load(path); } for (int i = 0; i < 10; i++) { wsprintf(path, TEXT("assets/enemy/spr_pomp_walk/%d.png"), i); m_ImgWalk_R[i].Load(path); m_ImgWalk_L[i].Load(path); } for (int i = 0; i < 6; i++) { wsprintf(path, TEXT("assets/enemy/spr_pomp_attack/%d.png"), i); m_ImgAttack_R[i].Load(path); m_ImgAttack_L[i].Load(path); } for (int i = 0; i < 10; i++) { wsprintf(path, TEXT("assets/enemy/spr_pomp_box_idle/%d.png"), i); m_ImgBoxIdle_R[i].Load(path); m_ImgBoxIdle_L[i].Load(path); } for (int i = 0; i < 14; i++) { wsprintf(path, TEXT("assets/enemy/spr_pomp_box_hit/%d.png"), i); m_ImgBoxHit_R[i].Load(path); m_ImgBoxHit_L[i].Load(path); } for (int i = 0; i < 6; i++) { wsprintf(path, TEXT("assets/enemy/spr_pomp_turn/%d.png"), i); m_ImgTurn_R[i].Load(path); m_ImgTurn_L[i].Load(path); } for (int i = 0; i < 13; i++) { wsprintf(path, TEXT("assets/enemy/spr_pomp_fall/%d.png"), i); m_ImgFall_R[i].Load(path); m_ImgFall_L[i].Load(path); } for (int i = 0; i < 2; i++) { wsprintf(path, TEXT("assets/enemy/spr_pomp_hurtfly/%d.png"), i); m_ImgHurtFly_R[i].Load(path); m_ImgHurtFly_L[i].Load(path); } for (int i = 0; i < 15; i++) { wsprintf(path, TEXT("assets/enemy/spr_pomp_hurtground/%d.png"), i); m_ImgHurtGround_R[i].Load(path); m_ImgHurtGround_L[i].Load(path); } for (int i = 0; i < 10; i++) { wsprintf(path, TEXT("assets/enemy/spr_pomp_run/%d.png"), i); m_ImgRun_R[i].Load(path); m_ImgRun_L[i].Load(path); } }
void Pomp::Release() { for (int i = 0; i < 8; i++) { m_ImgIdle_R[i].Destroy(); m_ImgIdle_L[i].Destroy(); } for (int i = 0; i < 10; i++) { m_ImgWalk_R[i].Destroy(); m_ImgWalk_L[i].Destroy(); } for (int i = 0; i < 6; i++) { m_ImgAttack_R[i].Destroy(); m_ImgAttack_L[i].Destroy(); } for (int i = 0; i < 10; i++) { m_ImgBoxIdle_R[i].Destroy(); m_ImgBoxIdle_L[i].Destroy(); } for (int i = 0; i < 14; i++) { m_ImgBoxHit_R[i].Destroy(); m_ImgBoxHit_L[i].Destroy(); } for (int i = 0; i < 6; i++) { m_ImgTurn_R[i].Destroy(); m_ImgTurn_L[i].Destroy(); } for (int i = 0; i < 13; i++) { m_ImgFall_R[i].Destroy(); m_ImgFall_L[i].Destroy(); } for (int i = 0; i < 2; i++) { m_ImgHurtFly_R[i].Destroy(); m_ImgHurtFly_L[i].Destroy(); } for (int i = 0; i < 15; i++) { m_ImgHurtGround_R[i].Destroy(); m_ImgHurtGround_L[i].Destroy(); } for (int i = 0; i < 10; i++) { m_ImgRun_R[i].Destroy(); m_ImgRun_L[i].Destroy(); } }
void Pomp::Update(float ts, const Player& player) {
    if (!m_isAlive) { if (m_ActionState == PompAction::PA_HURT_FLY && m_vy == 0) { m_ActionState = PompAction::PA_HURT_GROUND; m_vx = 0; m_CurrentFrame = 0; } if (GetTickCount() - m_LastTime >= 100) { if (m_ActionState == PompAction::PA_HURT_GROUND) { if (m_CurrentFrame < 14) m_CurrentFrame++; } else m_CurrentFrame++; m_LastTime = GetTickCount(); } Enemy::Update(ts, player); return; }
    Enemy::Update(ts, player); DWORD ct = GetTickCount();
    if (m_isPlayerDetected) {
        float dx = player.GetX() - m_x; m_isFacingLeft = (dx < 0);
        if (m_ActionState == PompAction::PA_ATTACK) {
            m_vx = 0; m_State = EnemyState::ES_IDLE; if (ct - m_LastTime >= (DWORD)(100.0f / ts)) {
                m_CurrentFrame++; m_LastTime = ct;
                if (m_CurrentFrame == 3) {
                    float ex = m_x + m_colW / 2.0f, ey = m_y + m_colH / 2.0f;
                    float px = player.GetX() + player.GetColW() / 2.0f, py = player.GetY() + player.GetColH() / 2.0f;
                    float dist = (float)sqrt((px - ex) * (px - ex) + (py - ey) * (py - ey));
                    float angle = atan2(py - ey, px - ex) * 180.0f / 3.14159f;
                    float absAngle = (float)fabs(angle);
                    if (dist < 80.0f && (m_isFacingLeft ? (absAngle >= 180.0f - m_detectAngle) : (absAngle <= m_detectAngle))) const_cast<Player&>(player).OnTakeDamage(1.0f);
                }
                if (m_CurrentFrame >= 6) { m_ActionState = PompAction::PA_NONE; m_CurrentFrame = 0; m_patternTimer = ct; }
            }
        } else if (fabs(dx) > 50.0f) { m_State = EnemyState::ES_WALK; m_vx = m_isFacingLeft ? -6.0f : 6.0f; }
        else { m_State = EnemyState::ES_IDLE; m_vx = 0; if (ct - m_patternTimer > (DWORD)(600.0f / ts)) { m_ActionState = PompAction::PA_ATTACK; m_CurrentFrame = 0; m_patternTimer = ct; m_LastTime = ct; } }
    } else {
        if (m_isWaiting) { m_vx = 0.0f; m_State = EnemyState::ES_IDLE; if (ct - m_patternTimer >= (DWORD)(1000.0f / ts)) { m_isWaiting = false; m_patternTimer = ct; m_walkDistance = 0.0f; m_isFacingLeft = !m_isFacingLeft; m_vx = m_isFacingLeft ? -2.2f : 2.2f; } }
        else { m_State = EnemyState::ES_WALK; m_vx = m_isFacingLeft ? -3.0f : 3.0f; float nx = m_x + m_vx * ts; bool oor = (m_isFacingLeft && nx < m_startX - m_patrolRange / 2.0f) || (!m_isFacingLeft && nx > m_startX + m_patrolRange / 2.0f); if (!CheckMapCollision(nx, m_y, m_colW, m_colH) && !oor) { m_walkDistance += (float)fabs(m_vx * ts); } else { m_isWaiting = true; m_patternTimer = ct; } }
    }
    if (m_vy > 5.0f) m_State = EnemyState::ES_FALL; else if (m_vy == 0.0f && m_State == EnemyState::ES_FALL) m_State = EnemyState::ES_IDLE;
    if (ct - m_LastTime >= (DWORD)(100.0f / ts)) { m_CurrentFrame++; m_LastTime = ct; }
}
void Pomp::Render(HDC hdc, Gdiplus::Graphics* g, float camX, float camY, float mapScale, bool showDebugRect, bool isSlowMo) {
    if (!m_isAlive && m_ActionState != PompAction::PA_HURT_FLY && m_ActionState != PompAction::PA_HURT_GROUND) return;
    RenderExclaim(hdc, camX, camY, mapScale); int sx = (int)((m_x - camX) * mapScale), sy = (int)((m_y - camY) * mapScale);
    float es = 1.8f, msX = 1.0f, msY = 1.0f; CImage* img = nullptr;
    if (m_isFacingLeft) { 
        if (m_ActionState == PompAction::PA_HURT_FLY) img = &m_ImgHurtFly_L[m_CurrentFrame % 2]; 
        else if (m_ActionState == PompAction::PA_HURT_GROUND || m_State == EnemyState::ES_DEAD) img = &m_ImgHurtGround_L[m_CurrentFrame % 15]; 
        else if (m_State == EnemyState::ES_FALL) img = &m_ImgFall_L[m_CurrentFrame % 13]; 
        else {
            switch (m_ActionState) {
                case PompAction::PA_NONE: if (m_State == EnemyState::ES_IDLE || m_State == EnemyState::ES_ALERT) { img = &m_ImgIdle_L[m_CurrentFrame % 8]; msX = 1.1f; msY = 1.1f; } else if (m_State == EnemyState::ES_WALK) img = &m_ImgWalk_L[m_CurrentFrame % 10]; break;
                case PompAction::PA_ATTACK: img = &m_ImgAttack_L[m_CurrentFrame % 6]; break;
                case PompAction::PA_BOX_IDLE: img = &m_ImgBoxIdle_L[m_CurrentFrame % 10]; break;
                case PompAction::PA_BOX_HIT: img = &m_ImgBoxHit_L[m_CurrentFrame % 14]; break;
                case PompAction::PA_TURN: img = &m_ImgTurn_L[m_CurrentFrame % 6]; break;
                case PompAction::PA_RUN: img = &m_ImgRun_L[m_CurrentFrame % 10]; break;
            }
        }
    } else { 
        if (m_ActionState == PompAction::PA_HURT_FLY) img = &m_ImgHurtFly_R[m_CurrentFrame % 2]; 
        else if (m_ActionState == PompAction::PA_HURT_GROUND || m_State == EnemyState::ES_DEAD) img = &m_ImgHurtGround_R[m_CurrentFrame % 15]; 
        else if (m_State == EnemyState::ES_FALL) img = &m_ImgFall_R[m_CurrentFrame % 13]; 
        else {
            switch (m_ActionState) {
                case PompAction::PA_NONE: if (m_State == EnemyState::ES_IDLE || m_State == EnemyState::ES_ALERT) { img = &m_ImgIdle_R[m_CurrentFrame % 8]; msX = 1.1f; msY = 1.1f; } else if (m_State == EnemyState::ES_WALK) img = &m_ImgWalk_R[m_CurrentFrame % 10]; break;
                case PompAction::PA_ATTACK: img = &m_ImgAttack_R[m_CurrentFrame % 6]; break;
                case PompAction::PA_BOX_IDLE: img = &m_ImgBoxIdle_R[m_CurrentFrame % 10]; break;
                case PompAction::PA_BOX_HIT: img = &m_ImgBoxHit_R[m_CurrentFrame % 14]; break;
                case PompAction::PA_TURN: img = &m_ImgTurn_R[m_CurrentFrame % 6]; break;
                case PompAction::PA_RUN: img = &m_ImgRun_R[m_CurrentFrame % 10]; break;
            }
        }
    }
    if (img && !img->IsNull()) {
        int fw = (int)(img->GetWidth() * es * msX * mapScale), fh = (int)(img->GetHeight() * es * msY * mapScale);
        int fy = sy + (int)(m_colH * mapScale) - fh, dx = sx + (int)(m_colW * mapScale / 2) - (fw / 2);
        if (m_isFacingLeft) { int om = SetGraphicsMode(hdc, GM_ADVANCED); XFORM xo; GetWorldTransform(hdc, &xo); XFORM xl = { -1.0f, 0.0f, 0.0f, 1.0f, (float)(2 * dx + fw), 0.0f }; SetWorldTransform(hdc, &xl); img->Draw(hdc, dx, fy, fw, fh); SetWorldTransform(hdc, &xo); SetGraphicsMode(hdc, om); }
        else img->Draw(hdc, dx, fy, fw, fh);
    }
    if (showDebugRect) RenderDebug(hdc, camX, camY, mapScale);
}

// ShieldCop
CImage ShieldCop::m_ImgIdle_R[6], ShieldCop::m_ImgIdle_L[6], ShieldCop::m_ImgWalk_R[10], ShieldCop::m_ImgWalk_L[10], ShieldCop::m_ImgRun_R[10], ShieldCop::m_ImgRun_L[10], ShieldCop::m_ImgTurn_R[8], ShieldCop::m_ImgTurn_L[8], ShieldCop::m_ImgAim_R[19], ShieldCop::m_ImgAim_L[19], ShieldCop::m_ImgBash_R[6], ShieldCop::m_ImgBash_L[6], ShieldCop::m_ImgKnockback_R[2], ShieldCop::m_ImgKnockback_L[2], ShieldCop::m_ImgTragedyDie_R[15], ShieldCop::m_ImgTragedyDie_L[15];
ShieldCop::ShieldCop(float x, float y) : Enemy(x, y, EnemyType::SHIELDCOP) { m_ActionState = ShieldCopAction::SA_NONE; }
ShieldCop::~ShieldCop() {}
void ShieldCop::Reset() { Enemy::Reset(); m_ActionState = ShieldCopAction::SA_NONE; }
void ShieldCop::OnTakeDamage(float kvx, float kvy) { if (m_isImmortal) return; Enemy::OnTakeDamage(kvx, kvy); m_ActionState = ShieldCopAction::SA_HURT_FLY; }
void ShieldCop::Init() { if (!m_ImgIdle_R[0].IsNull()) return; TCHAR path[256]; for (int i = 0; i < 6; i++) { wsprintf(path, TEXT("assets/enemy/spr_shieldcop_idle/%d.png"), i); m_ImgIdle_R[i].Load(path); m_ImgIdle_L[i].Load(path); } for (int i = 0; i < 10; i++) { wsprintf(path, TEXT("assets/enemy/spr_shieldcop_walk/%d.png"), i); m_ImgWalk_R[i].Load(path); m_ImgWalk_L[i].Load(path); } for (int i = 0; i < 10; i++) { wsprintf(path, TEXT("assets/enemy/spr_shieldcop_run/%d.png"), i); m_ImgRun_R[i].Load(path); m_ImgRun_L[i].Load(path); } for (int i = 0; i < 8; i++) { wsprintf(path, TEXT("assets/enemy/spr_shieldcop_turn/%d.png"), i); m_ImgTurn_R[i].Load(path); m_ImgTurn_L[i].Load(path); } for (int i = 0; i < 19; i++) { wsprintf(path, TEXT("assets/enemy/spr_shieldcop_aim/%d.png"), i); m_ImgAim_R[i].Load(path); m_ImgAim_L[i].Load(path); } for (int i = 0; i < 6; i++) { wsprintf(path, TEXT("assets/enemy/spr_shieldcop_bash/%d.png"), i); m_ImgBash_R[i].Load(path); m_ImgBash_L[i].Load(path); } for (int i = 0; i < 2; i++) { wsprintf(path, TEXT("assets/enemy/spr_shieldcop_knockback/%d.png"), i); m_ImgKnockback_R[i].Load(path); m_ImgKnockback_L[i].Load(path); } for (int i = 0; i < 15; i++) { wsprintf(path, TEXT("assets/enemy/spr_shieldcop_tragedy_die_1/%d.png"), i); m_ImgTragedyDie_R[i].Load(path); m_ImgTragedyDie_L[i].Load(path); } }
void ShieldCop::Release() { for (int i = 0; i < 6; i++) { m_ImgIdle_R[i].Destroy(); m_ImgIdle_L[i].Destroy(); } for (int i = 0; i < 10; i++) { m_ImgWalk_R[i].Destroy(); m_ImgWalk_L[i].Destroy(); } for (int i = 0; i < 10; i++) { m_ImgRun_R[i].Destroy(); m_ImgRun_L[i].Destroy(); } for (int i = 0; i < 8; i++) { m_ImgTurn_R[i].Destroy(); m_ImgTurn_L[i].Destroy(); } for (int i = 0; i < 19; i++) { m_ImgAim_R[i].Destroy(); m_ImgAim_L[i].Destroy(); } for (int i = 0; i < 6; i++) { m_ImgBash_R[i].Destroy(); m_ImgBash_L[i].Destroy(); } for (int i = 0; i < 2; i++) { m_ImgKnockback_R[i].Destroy(); m_ImgKnockback_L[i].Destroy(); } for (int i = 0; i < 15; i++) { m_ImgTragedyDie_R[i].Destroy(); m_ImgTragedyDie_L[i].Destroy(); } }
void ShieldCop::Update(float ts, const Player& player) {
    if (!m_isAlive) { if (m_ActionState == ShieldCopAction::SA_HURT_FLY && m_vy == 0) { m_ActionState = ShieldCopAction::SA_HURT_GROUND; m_vx = 0; m_CurrentFrame = 0; } if (GetTickCount() - m_LastTime >= 100) { if (m_ActionState == ShieldCopAction::SA_HURT_GROUND) { if (m_CurrentFrame < 14) m_CurrentFrame++; } else m_CurrentFrame++; m_LastTime = GetTickCount(); } Enemy::Update(ts, player); return; }
    Enemy::Update(ts, player); DWORD ct = GetTickCount();
    if (m_isPlayerDetected) {
        float dx = player.GetX() - m_x; m_isFacingLeft = (dx < 0);
        if (m_ActionState == ShieldCopAction::SA_BASH) {
            m_vx = 0; m_State = EnemyState::ES_IDLE; if (ct - m_LastTime >= (DWORD)(100.0f / ts)) {
                m_CurrentFrame++; m_LastTime = ct;
                if (m_CurrentFrame == 3) {
                    float ex = m_x + m_colW / 2.0f, ey = m_y + m_colH / 2.0f;
                    float px = player.GetX() + player.GetColW() / 2.0f, py = player.GetY() + player.GetColH() / 2.0f;
                    float dist = (float)sqrt((px - ex) * (px - ex) + (py - ey) * (py - ey));
                    float angle = atan2(py - ey, px - ex) * 180.0f / 3.14159f;
                    float absAngle = (float)fabs(angle);
                    if (dist < 70.0f && (m_isFacingLeft ? (absAngle >= 180.0f - m_detectAngle) : (absAngle <= m_detectAngle))) const_cast<Player&>(player).OnTakeDamage(1.0f);
                }
                if (m_CurrentFrame >= 6) { m_ActionState = ShieldCopAction::SA_NONE; m_CurrentFrame = 0; m_patternTimer = ct; }
            }
        } else if (fabs(dx) > 40.0f) { m_State = EnemyState::ES_WALK; m_vx = m_isFacingLeft ? -3.5f : 3.5f; }
        else { m_State = EnemyState::ES_IDLE; m_vx = 0; if (ct - m_patternTimer > (DWORD)(1000.0f / ts)) { m_ActionState = ShieldCopAction::SA_BASH; m_CurrentFrame = 0; m_patternTimer = ct; m_LastTime = ct; } }
    } else {
        if (m_isWaiting) { m_vx = 0.0f; m_State = EnemyState::ES_IDLE; if (ct - m_patternTimer >= (DWORD)(1200.0f / ts)) { m_isWaiting = false; m_patternTimer = ct; m_walkDistance = 0.0f; m_isFacingLeft = !m_isFacingLeft; m_vx = m_isFacingLeft ? -1.8f : 1.8f; } }
        else { m_State = EnemyState::ES_WALK; m_vx = m_isFacingLeft ? -2.0f : 2.0f; float nx = m_x + m_vx * ts; bool oor = (m_isFacingLeft && nx < m_startX - m_patrolRange / 2.0f) || (!m_isFacingLeft && nx > m_startX + m_patrolRange / 2.0f); if (!CheckMapCollision(nx, m_y, m_colW, m_colH) && !oor) { m_walkDistance += (float)fabs(m_vx * ts); } else { m_isWaiting = true; m_patternTimer = ct; } }
    }
    if (m_vy > 0.1f) m_State = EnemyState::ES_FALL; else if (m_vy == 0.0f && m_State == EnemyState::ES_FALL) m_State = EnemyState::ES_IDLE;
    if (ct - m_LastTime >= (DWORD)(100.0f / ts)) { m_CurrentFrame++; m_LastTime = ct; }
}
void ShieldCop::Render(HDC hdc, Gdiplus::Graphics* g, float camX, float camY, float mapScale, bool showDebugRect, bool isSlowMo) {
    if (!m_isAlive && m_ActionState != ShieldCopAction::SA_HURT_FLY && m_ActionState != ShieldCopAction::SA_HURT_GROUND) return;
    RenderExclaim(hdc, camX, camY, mapScale); int sx = (int)((m_x - camX) * mapScale), sy = (int)((m_y - camY) * mapScale);
    float es = 1.8f, msX = 1.0f, msY = 1.0f; CImage* img = nullptr;
    if (m_isFacingLeft) { 
        if (m_ActionState == ShieldCopAction::SA_HURT_FLY) img = &m_ImgKnockback_L[m_CurrentFrame % 2]; 
        else if (m_ActionState == ShieldCopAction::SA_HURT_GROUND || m_State == EnemyState::ES_DEAD) img = &m_ImgTragedyDie_L[m_CurrentFrame % 15]; 
        else if (m_State == EnemyState::ES_FALL) img = &m_ImgIdle_L[m_CurrentFrame % 6]; 
        else {
            switch (m_ActionState) {
                case ShieldCopAction::SA_NONE: if (m_State == EnemyState::ES_IDLE || m_State == EnemyState::ES_ALERT) { img = &m_ImgIdle_L[m_CurrentFrame % 6]; msX = 1.1f; msY = 1.1f; } else if (m_State == EnemyState::ES_WALK) img = &m_ImgWalk_L[m_CurrentFrame % 10]; break;
                case ShieldCopAction::SA_AIM: img = &m_ImgAim_L[m_CurrentFrame % 19]; break;
                case ShieldCopAction::SA_BASH: img = &m_ImgBash_L[m_CurrentFrame % 6]; break;
                case ShieldCopAction::SA_TURN: img = &m_ImgTurn_L[m_CurrentFrame % 8]; break;
                case ShieldCopAction::SA_RUN: img = &m_ImgRun_L[m_CurrentFrame % 10]; break;
            }
        }
    } else { 
        if (m_ActionState == ShieldCopAction::SA_HURT_FLY) img = &m_ImgKnockback_R[m_CurrentFrame % 2]; 
        else if (m_ActionState == ShieldCopAction::SA_HURT_GROUND || m_State == EnemyState::ES_DEAD) img = &m_ImgTragedyDie_R[m_CurrentFrame % 15]; 
        else if (m_State == EnemyState::ES_FALL) img = &m_ImgIdle_R[m_CurrentFrame % 6]; 
        else {
            switch (m_ActionState) {
                case ShieldCopAction::SA_NONE: if (m_State == EnemyState::ES_IDLE || m_State == EnemyState::ES_ALERT) { img = &m_ImgIdle_R[m_CurrentFrame % 6]; msX = 1.1f; msY = 1.1f; } else if (m_State == EnemyState::ES_WALK) img = &m_ImgWalk_R[m_CurrentFrame % 10]; break;
                case ShieldCopAction::SA_AIM: img = &m_ImgAim_R[m_CurrentFrame % 19]; break;
                case ShieldCopAction::SA_BASH: img = &m_ImgBash_R[m_CurrentFrame % 6]; break;
                case ShieldCopAction::SA_TURN: img = &m_ImgTurn_R[m_CurrentFrame % 8]; break;
                case ShieldCopAction::SA_RUN: img = &m_ImgRun_R[m_CurrentFrame % 10]; break;
            }
        }
    }
    if (img && !img->IsNull()) {
        int fw = (int)(img->GetWidth() * es * msX * mapScale), fh = (int)(img->GetHeight() * es * msY * mapScale);
        int fy = sy + (int)(m_colH * mapScale) - fh, dx = sx + (int)(m_colW * mapScale / 2) - (fw / 2);
        if (m_isFacingLeft) { int om = SetGraphicsMode(hdc, GM_ADVANCED); XFORM xo; GetWorldTransform(hdc, &xo); XFORM xl = { -1.0f, 0.0f, 0.0f, 1.0f, (float)(2 * dx + fw), 0.0f }; SetWorldTransform(hdc, &xl); img->Draw(hdc, dx, fy, fw, fh); SetWorldTransform(hdc, &xo); SetGraphicsMode(hdc, om); }
        else img->Draw(hdc, dx, fy, fw, fh);
    }
    if (showDebugRect) RenderDebug(hdc, camX, camY, mapScale);
}
