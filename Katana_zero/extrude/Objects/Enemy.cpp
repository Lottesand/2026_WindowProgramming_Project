#include "Enemy.h"
#include "Physics.h"
#include "../SceneAndMap/Camera.h"
#include "Player.h"
#include "Bullet.h"
#include <algorithm>
#include <cmath>

CImage Enemy::m_ImgExclaim[2];

Enemy::Enemy(float startX, float startY, EnemyType type) {
    m_startX = startX; m_startY = startY; m_x = startX; m_y = startY;
    m_vx = 2.0f; m_vy = 0.0f; m_colW = 40.0f; m_colH = 60.0f;
    m_isAlive = true; m_isFacingLeft = false; m_Type = type; m_State = EnemyState::IDLE;
    m_CurrentFrame = 0; m_LastTime = GetTickCount(); m_friction = 0.8f; m_knockbackVx = 0.0f;
    m_isImmortal = false; m_patternTimer = GetTickCount(); m_isWaiting = false; m_walkDistance = 0.0f;
    m_isPlayerDetected = false; m_alertStartTime = 0; m_exclaimFrame = 0;
}
Enemy::~Enemy() {}

void Enemy::ReleaseAll() {
    Gangster::Release();
    Grunt::Release();
    Pomp::Release();
    ShieldCop::Release();
    m_ImgExclaim[0].Destroy(); m_ImgExclaim[1].Destroy();
}

void Enemy::Reset() {
    m_x = m_startX; m_y = m_startY; m_vx = 2.0f; m_vy = 0.0f;
    m_isAlive = true; m_isFacingLeft = false; m_State = EnemyState::IDLE;
    m_CurrentFrame = 0; m_LastTime = GetTickCount(); m_knockbackVx = 0.0f;
    m_patternTimer = GetTickCount(); m_isWaiting = false; m_walkDistance = 0.0f;
    m_isPlayerDetected = false; m_alertStartTime = 0; m_exclaimFrame = 0;
}

void Enemy::OnTakeDamage(float damage) { if (!m_isImmortal) { m_isAlive = false; m_State = EnemyState::DEAD; } }
void Enemy::ApplyKnockback(float vx) { m_knockbackVx = vx; }

bool Enemy::IsPlayerInCone(float px, float py, float pw, float ph) {
    float ex = m_x + m_colW / 2.0f, ey = m_y + m_colH / 2.0f;
    float pcx = px + pw / 2.0f, pcy = py + ph / 2.0f;
    float dx = pcx - ex, dy = pcy - ey, dist = sqrt(dx * dx + dy * dy);
    if (dist > m_detectRange) return false;

    float angle = atan2(dy, dx) * 180.0f / 3.14159f;
    float absAngle = fabs(angle);
    if (absAngle <= m_detectAngle) return true; // Right cone
    if (absAngle >= 180.0f - m_detectAngle) return true; // Left cone
    return false;
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

        // Pie(hdc, left, top, right, bottom, xr1, yr1, xr2, yr2)
        // xr1, yr1: start of arc
        // xr2, yr2: end of arc
        // CCW from (xr1, yr1) to (xr2, yr2)
        // In GDI (Y-down), CCW is actually from positive Y to positive X? 
        // No, GDI Pie documentation: "The arc is drawn counterclockwise from the starting point to the ending point."
        // With Y down, if we start at +45 deg and go CCW, we go through 0 to -45.
        
        int x1 = ex + (int)(100 * cos(endAngle));
        int y1 = ey + (int)(100 * sin(endAngle));
        int x2 = ex + (int)(100 * cos(startAngle));
        int y2 = ey + (int)(100 * sin(startAngle));

        Pie(hdc, ex - r, ey - r, ex + r, ey + r, x1, y1, x2, y2);
    };

    DrawCone(0.0f);   // Right
    DrawCone(180.0f); // Left

    SelectObject(hdc, hOldBrush);
    SelectObject(hdc, hOldPen);
    DeleteObject(hRedPen);
}

void Enemy::UpdateDetection(float px, float py, float pw, float ph, float ts) {
    if (!m_isAlive) return;
    if (!m_isPlayerDetected) {
        if (IsPlayerInCone(px, py, pw, ph)) {
            m_isPlayerDetected = true;
            m_alertStartTime = GetTickCount();
            m_exclaimFrame = 0;
            m_State = EnemyState::ALERT;
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

void Enemy::Update(float ts) {
    if (!m_isAlive) {
        if (fabs(m_knockbackVx) > 0.1f) {
            float nx = m_x + m_knockbackVx * ts;
            if (!CheckCollision((int)(nx + m_colW / 2), (int)(m_y + m_colH * 0.9f))) m_x = nx;
            m_knockbackVx *= m_friction;
        }
        return;
    }
    if (fabs(m_knockbackVx) > 0.1f) {
        float nx = m_x + m_knockbackVx * ts;
        if (!CheckCollision((int)(nx + m_colW / 2), (int)(m_y + m_colH * 0.9f))) m_x = nx;
        m_knockbackVx *= m_friction;
    }
    m_vy += 2.0f * ts; if (m_vy > 30.0f) m_vy = 30.0f;
    int fx = (int)m_x + (int)(m_colW / 2), nfy = (int)(m_y + m_colH + m_vy * ts);
    if (CheckCollision(fx, nfy)) { m_vy = 0.0f; while (CheckCollision(fx, (int)(m_y + m_colH))) m_y -= 1.0f; } else m_y += m_vy * ts;
}

// Gangster
CImage Gangster::m_ImgIdle_R[8], Gangster::m_ImgIdle_L[8], Gangster::m_ImgWalk_R[8], Gangster::m_ImgWalk_L[8], Gangster::m_ImgAim_R[4], Gangster::m_ImgAim_L[4], Gangster::m_ImgFire_R[6], Gangster::m_ImgFire_L[6], Gangster::m_ImgTurn_R[6], Gangster::m_ImgTurn_L[6], Gangster::m_ImgFall_R[12], Gangster::m_ImgFall_L[12], Gangster::m_ImgHurtFly_R[2], Gangster::m_ImgHurtFly_L[2], Gangster::m_ImgHurtGround_R[14], Gangster::m_ImgHurtGround_L[14], Gangster::m_ImgRun_R[10], Gangster::m_ImgRun_L[10], Gangster::m_ImgGun_R[2], Gangster::m_ImgGun_L[2], Gangster::m_ImgArm[2];
Gangster::Gangster(float x, float y) : Enemy(x, y, EnemyType::GANGSTER) { m_ActionState = GangsterAction::NONE; m_aimAngle = 0.0f; }
Gangster::~Gangster() {}
void Gangster::Reset() { Enemy::Reset(); m_ActionState = GangsterAction::NONE; m_aimAngle = 0.0f; }
void Gangster::Init() {
    if (!m_ImgIdle_R[0].IsNull()) return;
    TCHAR p[256];
    for (int i = 0; i < 8; i++) { wsprintf(p, TEXT("assets/enemy/spr_gangsteridle/%d.png"), i); m_ImgIdle_R[i].Load(p); m_ImgIdle_L[i].Load(p); }
    for (int i = 0; i < 8; i++) { wsprintf(p, TEXT("assets/enemy/spr_gangsterwalk/%d.png"), i); m_ImgWalk_R[i].Load(p); m_ImgWalk_L[i].Load(p); }
    for (int i = 0; i < 4; i++) { wsprintf(p, TEXT("assets/enemy/spr_gangster_aim/%d.png"), i); m_ImgAim_R[i].Load(p); m_ImgAim_L[i].Load(p); }
    for (int i = 0; i < 6; i++) { wsprintf(p, TEXT("assets/enemy/spr_fire_1/%d.png"), i); m_ImgFire_R[i].Load(p); m_ImgFire_L[i].Load(p); }
    for (int i = 0; i < 6; i++) { wsprintf(p, TEXT("assets/enemy/spr_gangsterturn/%d.png"), i); m_ImgTurn_R[i].Load(p); m_ImgTurn_L[i].Load(p); }
    for (int i = 0; i < 12; i++) { wsprintf(p, TEXT("assets/enemy/spr_gangsterfall/%d.png"), i); m_ImgFall_R[i].Load(p); m_ImgFall_L[i].Load(p); }
    for (int i = 0; i < 2; i++) { wsprintf(p, TEXT("assets/enemy/spr_gangsterhurtfly/%d.png"), i); m_ImgHurtFly_R[i].Load(p); m_ImgHurtFly_L[i].Load(p); }
    for (int i = 0; i < 14; i++) { wsprintf(p, TEXT("assets/enemy/spr_gangsterhurtground/%d.png"), i); m_ImgHurtGround_R[i].Load(p); m_ImgHurtGround_L[i].Load(p); }
    for (int i = 0; i < 10; i++) { wsprintf(p, TEXT("assets/enemy/spr_gangsterrun/%d.png"), i); m_ImgRun_R[i].Load(p); m_ImgRun_L[i].Load(p); }
    for (int i = 0; i < 2; i++) { wsprintf(p, TEXT("assets/enemy/spr_gangstergun/%d.png"), i); m_ImgGun_R[i].Load(p); m_ImgGun_L[i].Load(p); }
    for (int i = 0; i < 2; i++) { wsprintf(p, TEXT("assets/enemy/spr_arm/%d.png"), i); m_ImgArm[i].Load(p); }
}
void Gangster::Release() {
    for (int i = 0; i < 8; i++) { m_ImgIdle_R[i].Destroy(); m_ImgIdle_L[i].Destroy(); }
    for (int i = 0; i < 8; i++) { m_ImgWalk_R[i].Destroy(); m_ImgWalk_L[i].Destroy(); }
    for (int i = 0; i < 4; i++) { m_ImgAim_R[i].Destroy(); m_ImgAim_L[i].Destroy(); }
    for (int i = 0; i < 6; i++) { m_ImgFire_R[i].Destroy(); m_ImgFire_L[i].Destroy(); }
    for (int i = 0; i < 6; i++) { m_ImgTurn_R[i].Destroy(); m_ImgTurn_L[i].Destroy(); }
    for (int i = 0; i < 12; i++) { m_ImgFall_R[i].Destroy(); m_ImgFall_L[i].Destroy(); }
    for (int i = 0; i < 2; i++) { m_ImgHurtFly_R[i].Destroy(); m_ImgHurtFly_L[i].Destroy(); }
    for (int i = 0; i < 14; i++) { m_ImgHurtGround_R[i].Destroy(); m_ImgHurtGround_L[i].Destroy(); }
    for (int i = 0; i < 10; i++) { m_ImgRun_R[i].Destroy(); m_ImgRun_L[i].Destroy(); }
    for (int i = 0; i < 2; i++) { m_ImgGun_R[i].Destroy(); m_ImgGun_L[i].Destroy(); }
    for (int i = 0; i < 2; i++) { m_ImgArm[i].Destroy(); }
}
void Gangster::Update(float ts) {
    if (m_ActionState == GangsterAction::HURT_FLY || m_ActionState == GangsterAction::HURT_GROUND || m_State == EnemyState::DEAD) {
        if (m_ActionState == GangsterAction::HURT_FLY) {
            m_vy += 1.5f * ts; m_y += m_vy * ts; m_x += m_vx * ts;
            if (CheckCollision((int)m_x + (int)(m_colW / 2), (int)(m_y + m_colH))) {
                m_ActionState = GangsterAction::HURT_GROUND; m_State = EnemyState::DEAD; m_vx = 0; m_vy = 0; m_CurrentFrame = 0;
            }
        }
        if (GetTickCount() - m_LastTime >= (DWORD)(100.0f / ts)) {
            if (m_ActionState == GangsterAction::HURT_GROUND || m_State == EnemyState::DEAD) { if (m_CurrentFrame < 13) m_CurrentFrame++; }
            else m_CurrentFrame++;
            m_LastTime = GetTickCount();
        }
        return;
    }

    Enemy::Update(ts);
    DWORD ct = GetTickCount();

    float px = Player::GetInstance().GetX(), py = Player::GetInstance().GetY();
    float pw = Player::GetInstance().GetColW(), ph = Player::GetInstance().GetColH();
    UpdateDetection(px, py, pw, ph, ts);

    if (m_isPlayerDetected) {
        float px = Player::GetInstance().GetX(), py = Player::GetInstance().GetY();
        float dx = px - m_x;
        float dy = py - m_y;
        m_aimAngle = atan2(dy, dx) * 180.0f / 3.14159f;
        bool nextFacingLeft = (dx < 0);

        if (fabs(dx) > 250.0f) {
            m_ActionState = GangsterAction::RUN;
            m_State = EnemyState::WALK;
            m_vx = m_isFacingLeft ? -8.0f : 8.0f;
            m_x += m_vx * ts;
        } else if (m_ActionState == GangsterAction::NONE || m_ActionState == GangsterAction::RUN) {
            m_vx = 0;
            m_ActionState = GangsterAction::AIM;
            m_CurrentFrame = 0;
            m_patternTimer = ct;
            m_LastTime = ct;
        } else if (m_ActionState == GangsterAction::AIM) {
            m_vx = 0;
            if (ct - m_LastTime >= (DWORD)(100.0f / ts)) {
                if (m_CurrentFrame < 3) {
                    m_CurrentFrame++;
                } else {
                    if (ct - m_patternTimer > (DWORD)(800.0f / ts)) {
                        m_ActionState = GangsterAction::FIRE;
                        m_CurrentFrame = 0;
                        m_patternTimer = ct;
                    }
                }
                m_LastTime = ct;
            }
        } else if (m_ActionState == GangsterAction::FIRE) {
            m_vx = 0;
            if (ct - m_LastTime >= (DWORD)(80.0f / ts)) {
                if (m_CurrentFrame < 5) {
                    m_CurrentFrame++;
                    if (m_CurrentFrame == 2) {
                        float bx = m_x + (m_isFacingLeft ? -15.0f : 55.0f);
                        float by = m_y + 30.0f;
                        float bvx = m_isFacingLeft ? -20.0f : 20.0f;
                        Bullet::AddBullet(bx, by, bvx, 0.0f);
                    }
                } else {
                    m_ActionState = GangsterAction::AIM;
                    m_patternTimer = ct;
                    m_CurrentFrame = 3;
                }
                m_LastTime = ct;
            }
        }
    } else {
        if (m_isWaiting) {
            m_vx = 0.0f; m_State = EnemyState::IDLE;
            if (ct - m_patternTimer >= (DWORD)(2000.0f / ts)) { m_isWaiting = false; m_patternTimer = ct; m_walkDistance = 0.0f; m_isFacingLeft = !m_isFacingLeft; m_vx = m_isFacingLeft ? -2.0f : 2.0f; }
        } else {
            m_State = EnemyState::WALK; int nx = (int)m_x + (int)(m_colW / 2) + (int)(m_vx * ts);
            if (!CheckCollision(nx, (int)m_y + (int)(m_colH * 0.9f))) {
                m_x += m_vx * ts; m_walkDistance += fabs(m_vx * ts); m_isFacingLeft = (m_vx < 0.0f);
                if (m_walkDistance >= 150.0f) { m_isWaiting = true; m_patternTimer = ct; }
            } else { m_isWaiting = true; m_patternTimer = ct; }
        }
    }

    if (m_vy > 5.0f) m_State = EnemyState::FALL;
    else if (m_vy == 0.0f && m_State == EnemyState::FALL) m_State = EnemyState::IDLE;

    if (ct - m_LastTime >= (DWORD)(100.0f / ts)) { m_CurrentFrame++; m_LastTime = ct; }
}
void Gangster::Render(HDC hdc, float camX, float camY, float mapScale, bool showDebugRect) {
    RenderExclaim(hdc, camX, camY, mapScale);
    int sx = (int)((m_x - camX) * mapScale), sy = (int)((m_y - camY) * mapScale);
    
    CImage* imgBody = nullptr; 
    CImage* imgFlash = nullptr;
    CImage* imgGun = nullptr;
    CImage* imgArm = nullptr;
    float msX = 1.0f, msY = 1.0f, es = 1.8f;

    if (m_isFacingLeft) {
        imgArm = &m_ImgArm[1];
        if (m_ActionState == GangsterAction::HURT_FLY) { imgBody = &m_ImgHurtFly_L[m_CurrentFrame % 2]; imgArm = nullptr; }
        else if (m_ActionState == GangsterAction::HURT_GROUND || m_State == EnemyState::DEAD) { imgBody = &m_ImgHurtGround_L[m_CurrentFrame % 14]; imgArm = nullptr; }
        else if (m_State == EnemyState::FALL) { imgBody = &m_ImgFall_L[m_CurrentFrame % 12]; imgArm = nullptr; }
        else {
            switch (m_ActionState) {
            case GangsterAction::NONE:
                if (m_State == EnemyState::IDLE) { imgBody = &m_ImgIdle_L[m_CurrentFrame % 8]; msX = 1.1f; msY = 1.1f; }
                else if (m_State == EnemyState::WALK) imgBody = &m_ImgWalk_L[m_CurrentFrame % 8];
                break;
            case GangsterAction::AIM: 
                imgBody = &m_ImgAim_L[m_CurrentFrame % 4]; 
                imgGun = &m_ImgGun_L[0];
                break;
            case GangsterAction::FIRE: 
                imgBody = &m_ImgAim_L[3];
                imgGun = &m_ImgGun_L[m_CurrentFrame == 2 ? 1 : 0];
                break;
            case GangsterAction::TURN: imgBody = &m_ImgTurn_L[m_CurrentFrame % 6]; break;
            case GangsterAction::RUN: imgBody = &m_ImgRun_L[m_CurrentFrame % 10]; break;
            }
        }
    } else {
        imgArm = &m_ImgArm[0];
        if (m_ActionState == GangsterAction::HURT_FLY) { imgBody = &m_ImgHurtFly_R[m_CurrentFrame % 2]; imgArm = nullptr; }
        else if (m_ActionState == GangsterAction::HURT_GROUND || m_State == EnemyState::DEAD) { imgBody = &m_ImgHurtGround_R[m_CurrentFrame % 14]; imgArm = nullptr; }
        else if (m_State == EnemyState::FALL) { imgBody = &m_ImgFall_R[m_CurrentFrame % 12]; imgArm = nullptr; }
        else {
            switch (m_ActionState) {
            case GangsterAction::NONE:
                if (m_State == EnemyState::IDLE) { imgBody = &m_ImgIdle_R[m_CurrentFrame % 8]; msX = 1.1f; msY = 1.1f; }
                else if (m_State == EnemyState::WALK) imgBody = &m_ImgWalk_R[m_CurrentFrame % 8];
                break;
            case GangsterAction::AIM: 
                imgBody = &m_ImgAim_R[m_CurrentFrame % 4]; 
                imgGun = &m_ImgGun_R[0];
                break;
            case GangsterAction::FIRE:
                imgBody = &m_ImgAim_R[3];
                imgGun = &m_ImgGun_R[m_CurrentFrame == 2 ? 1 : 0];
                break;
            case GangsterAction::TURN: imgBody = &m_ImgTurn_R[m_CurrentFrame % 6]; break;
            case GangsterAction::RUN: imgBody = &m_ImgRun_R[m_CurrentFrame % 10]; break;
            }
        }
    }

    auto DrawImg = [&](CImage* img, float scaleX, float scaleY, float offX = 0, float offY = 0, int flipMode = 0) {
        if (!img || img->IsNull()) return;
        int fw = (int)(img->GetWidth() * es * scaleX * mapScale);
        int fh = (int)(img->GetHeight() * es * scaleY * mapScale);
        int fy = sy + (int)(m_colH * mapScale) - fh + (int)(offY * mapScale);
        int dx = sx + (int)(m_colW * mapScale / 2) - (fw / 2) + (int)(offX * mapScale);
        
        bool doFlip = (flipMode == 0) ? m_isFacingLeft : (flipMode == 1);

        if (doFlip) {
            int om = SetGraphicsMode(hdc, GM_ADVANCED); XFORM xo; GetWorldTransform(hdc, &xo);
            XFORM xl = { -1.0f, 0.0f, 0.0f, 1.0f, (float)(2 * dx + fw), 0.0f }; SetWorldTransform(hdc, &xl);
            img->Draw(hdc, dx, fy, fw, fh); SetWorldTransform(hdc, &xo); SetGraphicsMode(hdc, om);
        } else {
            img->Draw(hdc, dx, fy, fw, fh);
        }
    };

    DrawImg(imgBody, msX, msY, 0, 0, 0);
    float gunOffX = m_isFacingLeft ? -10.0f : 10.0f;
    float gunOffY = -30.0f;
    if (imgArm && (m_ActionState == GangsterAction::AIM || m_ActionState == GangsterAction::FIRE)) {
        // Arm assets 0 (R) and 1 (L) are already oriented, so force no flip (flipMode 2)
        DrawImg(imgArm, 1.0f, 1.0f, gunOffX, gunOffY, 2);
    }
    if (imgGun) DrawImg(imgGun, 1.0f, 1.0f, gunOffX, gunOffY, 0);

    if (showDebugRect) {
        RenderDetectionRange(hdc, camX, camY, mapScale);
        int sw = (int)(m_colW * mapScale), sh = (int)(m_colH * mapScale);
        HBRUSH hr = CreateSolidBrush(RGB(255, 0, 0)); RECT r = { sx, sy, sx + sw, sy + sh }; FrameRect(hdc, &r, hr); DeleteObject(hr);
    }
}
void Gangster::OnTakeDamage(float d) { m_isAlive = false; m_ActionState = GangsterAction::HURT_FLY; m_vy = -15.0f; m_vx = m_isFacingLeft ? 8.0f : -8.0f; m_CurrentFrame = 0; }

CImage Grunt::m_ImgIdle_R[8], Grunt::m_ImgIdle_L[8], Grunt::m_ImgWalk_R[10], Grunt::m_ImgWalk_L[10], Grunt::m_ImgAttack_R[8], Grunt::m_ImgAttack_L[8], Grunt::m_ImgSlash_R[5], Grunt::m_ImgSlash_L[5], Grunt::m_ImgTurn_R[8], Grunt::m_ImgTurn_L[8], Grunt::m_ImgFall_R[13], Grunt::m_ImgFall_L[13], Grunt::m_ImgHurtFly_R[2], Grunt::m_ImgHurtFly_L[2], Grunt::m_ImgHurtGround_R[16], Grunt::m_ImgHurtGround_L[16], Grunt::m_ImgRun_R[10], Grunt::m_ImgRun_L[10];
Grunt::Grunt(float x, float y) : Enemy(x, y, EnemyType::GRUNT) { m_ActionState = GruntAction::NONE; }
Grunt::~Grunt() {}
void Grunt::Reset() { Enemy::Reset(); m_ActionState = GruntAction::NONE; }
void Grunt::Init() {
    if (!m_ImgIdle_R[0].IsNull()) return;
    TCHAR p[256];
    for (int i = 0; i < 8; i++) { wsprintf(p, TEXT("assets/enemy/spr_grunt_idle/%d.png"), i); m_ImgIdle_R[i].Load(p); m_ImgIdle_L[i].Load(p); }
    for (int i = 0; i < 10; i++) { wsprintf(p, TEXT("assets/enemy/spr_grunt_walk/%d.png"), i); m_ImgWalk_R[i].Load(p); m_ImgWalk_L[i].Load(p); }
    for (int i = 0; i < 8; i++) { wsprintf(p, TEXT("assets/enemy/spr_grunt_attack/%d.png"), i); m_ImgAttack_R[i].Load(p); m_ImgAttack_L[i].Load(p); }
    for (int i = 0; i < 5; i++) { wsprintf(p, TEXT("assets/enemy/spr_gruntslash/%d.png"), i); m_ImgSlash_R[i].Load(p); m_ImgSlash_L[i].Load(p); }
    for (int i = 0; i < 8; i++) { wsprintf(p, TEXT("assets/enemy/spr_grunt_turn/%d.png"), i); m_ImgTurn_R[i].Load(p); m_ImgTurn_L[i].Load(p); }
    for (int i = 0; i < 13; i++) { wsprintf(p, TEXT("assets/enemy/spr_grunt_fall/%d.png"), i); m_ImgFall_R[i].Load(p); m_ImgFall_L[i].Load(p); }
    for (int i = 0; i < 2; i++) { wsprintf(p, TEXT("assets/enemy/spr_grunt_hurtfly/%d.png"), i); m_ImgHurtFly_R[i].Load(p); m_ImgHurtFly_L[i].Load(p); }
    for (int i = 0; i < 16; i++) { wsprintf(p, TEXT("assets/enemy/spr_grunt_hurtground/%d.png"), i); m_ImgHurtGround_R[i].Load(p); m_ImgHurtGround_L[i].Load(p); }
    for (int i = 0; i < 10; i++) { wsprintf(p, TEXT("assets/enemy/spr_grunt_run/%d.png"), i); m_ImgRun_R[i].Load(p); m_ImgRun_L[i].Load(p); }
}
void Grunt::Release() {
    for (int i = 0; i < 8; i++) { m_ImgIdle_R[i].Destroy(); m_ImgIdle_L[i].Destroy(); }
    for (int i = 0; i < 10; i++) { m_ImgWalk_R[i].Destroy(); m_ImgWalk_L[i].Destroy(); }
    for (int i = 0; i < 8; i++) { m_ImgAttack_R[i].Destroy(); m_ImgAttack_L[i].Destroy(); }
    for (int i = 0; i < 5; i++) { m_ImgSlash_R[i].Destroy(); m_ImgSlash_L[i].Destroy(); }
    for (int i = 0; i < 8; i++) { m_ImgTurn_R[i].Destroy(); m_ImgTurn_L[i].Destroy(); }
    for (int i = 0; i < 13; i++) { m_ImgFall_R[i].Destroy(); m_ImgFall_L[i].Destroy(); }
    for (int i = 0; i < 2; i++) { m_ImgHurtFly_R[i].Destroy(); m_ImgHurtFly_L[i].Destroy(); }
    for (int i = 0; i < 16; i++) { m_ImgHurtGround_R[i].Destroy(); m_ImgHurtGround_L[i].Destroy(); }
    for (int i = 0; i < 10; i++) { m_ImgRun_R[i].Destroy(); m_ImgRun_L[i].Destroy(); }
}
void Grunt::Update(float ts) {
    if (m_ActionState == GruntAction::HURT_FLY || m_ActionState == GruntAction::HURT_GROUND || m_State == EnemyState::DEAD) {
        if (m_ActionState == GruntAction::HURT_FLY) {
            m_vy += 1.5f * ts; m_y += m_vy * ts; m_x += m_vx * ts;
            if (CheckCollision((int)m_x + (int)(m_colW / 2), (int)(m_y + m_colH))) {
                m_ActionState = GruntAction::HURT_GROUND; m_State = EnemyState::DEAD; m_vx = 0; m_vy = 0; m_CurrentFrame = 0;
            }
        }
        if (GetTickCount() - m_LastTime >= (DWORD)(100.0f / ts)) {
            if (m_ActionState == GruntAction::HURT_GROUND || m_State == EnemyState::DEAD) { if (m_CurrentFrame < 15) m_CurrentFrame++; }
            else m_CurrentFrame++;
            m_LastTime = GetTickCount();
        }
        return;
    }

    Enemy::Update(ts);
    DWORD ct = GetTickCount();

    float px = Player::GetInstance().GetX(), py = Player::GetInstance().GetY();
    float pw = Player::GetInstance().GetColW(), ph = Player::GetInstance().GetColH();
    UpdateDetection(px, py, pw, ph, ts);

    if (m_isPlayerDetected) {
        float dx = px - m_x;
        bool nextFacingLeft = (dx < 0);
        if (m_isFacingLeft != nextFacingLeft && m_ActionState != GruntAction::TURN) {
            m_ActionState = GruntAction::TURN;
            m_CurrentFrame = 0;
            m_patternTimer = ct;
        }
        m_isFacingLeft = nextFacingLeft;
        
        if (m_ActionState == GruntAction::TURN) {
            m_vx = 0;
            if (ct - m_LastTime >= (DWORD)(100.0f / ts)) {
                m_CurrentFrame++; m_LastTime = ct;
                if (m_CurrentFrame >= 8) { m_ActionState = GruntAction::NONE; }
            }
        }
        else if (m_ActionState == GruntAction::ATTACK) {
            m_vx = 0;
            if (ct - m_LastTime >= (DWORD)(100.0f / ts)) {
                m_CurrentFrame++;
                m_LastTime = ct;
                if (m_CurrentFrame == 5) {
                    if (fabs(dx) < 100.0f && fabs(py - m_y) < 50.0f) {
                        Player::GetInstance().OnTakeDamage(1.0f);
                    }
                }
                if (m_CurrentFrame >= 8) {
                    m_ActionState = GruntAction::NONE;
                    m_CurrentFrame = 0;
                    m_patternTimer = ct;
                }
            }
        } else if (fabs(dx) > 60.0f) {
            m_ActionState = GruntAction::RUN;
            m_State = EnemyState::WALK;
            m_vx = m_isFacingLeft ? -9.0f : 9.0f;
            m_x += m_vx * ts;
        } else {
            if (m_ActionState == GruntAction::RUN) m_ActionState = GruntAction::NONE;
            m_vx = 0;
            if (ct - m_patternTimer > (DWORD)(800.0f / ts)) {
                m_ActionState = GruntAction::ATTACK;
                m_CurrentFrame = 0;
                m_patternTimer = ct;
                m_LastTime = ct;
            }
        }
    } else {
        if (m_isWaiting) {
            m_vx = 0.0f; m_State = EnemyState::IDLE;
            if (ct - m_patternTimer >= (DWORD)(2000.0f / ts)) { m_isWaiting = false; m_patternTimer = ct; m_walkDistance = 0.0f; m_isFacingLeft = !m_isFacingLeft; m_vx = m_isFacingLeft ? -2.0f : 2.0f; }
        } else {
            m_State = EnemyState::WALK; int nx = (int)m_x + (int)(m_colW / 2) + (int)(m_vx * ts);
            if (!CheckCollision(nx, (int)m_y + (int)(m_colH * 0.9f))) {
                m_x += m_vx * ts; m_walkDistance += fabs(m_vx * ts); m_isFacingLeft = (m_vx < 0.0f);
                if (m_walkDistance >= 150.0f) { m_isWaiting = true; m_patternTimer = ct; }
            } else { m_isWaiting = true; m_patternTimer = ct; }
        }
    }

    if (m_vy > 5.0f) m_State = EnemyState::FALL;
    else if (m_vy == 0.0f && m_State == EnemyState::FALL) m_State = EnemyState::IDLE;

    if (ct - m_LastTime >= (DWORD)(100.0f / ts)) { m_CurrentFrame++; m_LastTime = ct; }
}
void Grunt::Render(HDC hdc, float camX, float camY, float ms, bool dr) {
    RenderExclaim(hdc, camX, camY, ms);
    int sx = (int)((m_x - camX) * ms), sy = (int)((m_y - camY) * ms);
    CImage* img = nullptr; 
    CImage* imgSlash = nullptr;
    float msX = 1.0f, msY = 1.0f, es = 1.8f;

    if (m_isFacingLeft) {
        if (m_ActionState == GruntAction::HURT_FLY) img = &m_ImgHurtFly_L[m_CurrentFrame % 2];
        else if (m_ActionState == GruntAction::HURT_GROUND || m_State == EnemyState::DEAD) img = &m_ImgHurtGround_L[m_CurrentFrame % 16];
        else if (m_State == EnemyState::FALL) img = &m_ImgFall_L[m_CurrentFrame % 13];
        else {
            switch (m_ActionState) {
            case GruntAction::NONE:
                if (m_State == EnemyState::IDLE) { img = &m_ImgIdle_L[m_CurrentFrame % 8]; msX = 1.1f; msY = 1.1f; }
                else if (m_State == EnemyState::WALK) img = &m_ImgWalk_L[m_CurrentFrame % 10];
                break;
            case GruntAction::ATTACK: 
                img = &m_ImgAttack_L[m_CurrentFrame % 8]; 
                if (m_CurrentFrame >= 3 && m_CurrentFrame <= 7) imgSlash = &m_ImgSlash_L[m_CurrentFrame - 3];
                break;
            case GruntAction::SLASH: img = &m_ImgSlash_L[m_CurrentFrame % 5]; break;
            case GruntAction::TURN: img = &m_ImgTurn_L[m_CurrentFrame % 8]; break;
            case GruntAction::RUN: img = &m_ImgRun_L[m_CurrentFrame % 10]; break;
            }
        }
    } else {
        if (m_ActionState == GruntAction::HURT_FLY) img = &m_ImgHurtFly_R[m_CurrentFrame % 2];
        else if (m_ActionState == GruntAction::HURT_GROUND || m_State == EnemyState::DEAD) img = &m_ImgHurtGround_R[m_CurrentFrame % 16];
        else if (m_State == EnemyState::FALL) img = &m_ImgIdle_R[m_CurrentFrame % 8];
        else {
            switch (m_ActionState) {
            case GruntAction::NONE:
                if (m_State == EnemyState::IDLE) { img = &m_ImgIdle_R[m_CurrentFrame % 8]; msX = 1.1f; msY = 1.1f; }
                else if (m_State == EnemyState::WALK) img = &m_ImgWalk_R[m_CurrentFrame % 10];
                break;
            case GruntAction::ATTACK: 
                img = &m_ImgAttack_R[m_CurrentFrame % 8]; 
                if (m_CurrentFrame >= 3 && m_CurrentFrame <= 7) imgSlash = &m_ImgSlash_R[m_CurrentFrame - 3];
                break;
            case GruntAction::SLASH: img = &m_ImgSlash_R[m_CurrentFrame % 5]; break;
            case GruntAction::TURN: img = &m_ImgTurn_R[m_CurrentFrame % 8]; break;
            case GruntAction::RUN: img = &m_ImgRun_R[m_CurrentFrame % 10]; break;
            }
        }
    }

    auto DrawSingleImg = [&](CImage* targetImg, float scaleX, float scaleY) {
        if (!targetImg || targetImg->IsNull()) return;
        int fw = (int)(targetImg->GetWidth() * es * scaleX * ms), fh = (int)(targetImg->GetHeight() * es * scaleY * ms);
        int fy = sy + (int)(m_colH * ms) - fh, dx = sx + (int)(m_colW * ms / 2) - (fw / 2);
        if (m_isFacingLeft) {
            int om = SetGraphicsMode(hdc, GM_ADVANCED); XFORM xo; GetWorldTransform(hdc, &xo);
            XFORM xl = { -1.0f, 0.0f, 0.0f, 1.0f, (float)(2 * dx + fw), 0.0f }; SetWorldTransform(hdc, &xl);
            targetImg->Draw(hdc, dx, fy, fw, fh); SetWorldTransform(hdc, &xo); SetGraphicsMode(hdc, om);
        } else targetImg->Draw(hdc, dx, fy, fw, fh);
    };

    DrawSingleImg(img, msX, msY);
    if (imgSlash) DrawSingleImg(imgSlash, 1.0f, 1.0f);

    if (dr) {
        RenderDetectionRange(hdc, camX, camY, ms);
        int sw = (int)(m_colW * ms), sh = (int)(m_colH * ms);
        HBRUSH hr = CreateSolidBrush(RGB(255, 0, 0)); RECT r = { sx, sy, sx + sw, sy + sh }; FrameRect(hdc, &r, hr); DeleteObject(hr);
    }
}
void Grunt::OnTakeDamage(float d) { m_isAlive = false; m_ActionState = GruntAction::HURT_FLY; m_vy = -15.0f; m_vx = m_isFacingLeft ? 8.0f : -8.0f; m_CurrentFrame = 0; }

CImage Pomp::m_ImgIdle_R[8], Pomp::m_ImgIdle_L[8], Pomp::m_ImgWalk_R[10], Pomp::m_ImgWalk_L[10], Pomp::m_ImgAttack_R[6], Pomp::m_ImgAttack_L[6], Pomp::m_ImgBoxIdle_R[10], Pomp::m_ImgBoxIdle_L[10], Pomp::m_ImgBoxHit_R[14], Pomp::m_ImgBoxHit_L[14], Pomp::m_ImgTurn_R[6], Pomp::m_ImgTurn_L[6], Pomp::m_ImgFall_R[13], Pomp::m_ImgFall_L[13], Pomp::m_ImgHurtFly_R[2], Pomp::m_ImgHurtFly_L[2], Pomp::m_ImgHurtGround_R[15], Pomp::m_ImgHurtGround_L[15], Pomp::m_ImgRun_R[10], Pomp::m_ImgRun_L[10];
Pomp::Pomp(float x, float y) : Enemy(x, y, EnemyType::POMP) { m_ActionState = PompAction::NONE; }
Pomp::~Pomp() {}
void Pomp::Reset() { Enemy::Reset(); m_ActionState = PompAction::NONE; }
void Pomp::Init() {
    if (!m_ImgIdle_R[0].IsNull()) return;
    TCHAR p[256];
    for (int i = 0; i < 8; i++) { wsprintf(p, TEXT("assets/enemy/spr_pomp_idle/%d.png"), i); m_ImgIdle_R[i].Load(p); m_ImgIdle_L[i].Load(p); }
    for (int i = 0; i < 10; i++) { wsprintf(p, TEXT("assets/enemy/spr_pomp_walk/%d.png"), i); m_ImgWalk_R[i].Load(p); m_ImgWalk_L[i].Load(p); }
    for (int i = 0; i < 6; i++) { wsprintf(p, TEXT("assets/enemy/spr_pomp_attack/%d.png"), i); m_ImgAttack_R[i].Load(p); m_ImgAttack_L[i].Load(p); }
    for (int i = 0; i < 10; i++) { wsprintf(p, TEXT("assets/enemy/spr_pomp_box_idle/%d.png"), i); m_ImgBoxIdle_R[i].Load(p); m_ImgBoxIdle_L[i].Load(p); }
    for (int i = 0; i < 14; i++) { wsprintf(p, TEXT("assets/enemy/spr_pomp_box_hit/%d.png"), i); m_ImgBoxHit_R[i].Load(p); m_ImgBoxHit_L[i].Load(p); }
    for (int i = 0; i < 6; i++) { wsprintf(p, TEXT("assets/enemy/spr_pomp_turn/%d.png"), i); m_ImgTurn_R[i].Load(p); m_ImgTurn_L[i].Load(p); }
    for (int i = 0; i < 13; i++) { wsprintf(p, TEXT("assets/enemy/spr_pomp_fall/%d.png"), i); m_ImgFall_R[i].Load(p); m_ImgFall_L[i].Load(p); }
    for (int i = 0; i < 2; i++) { wsprintf(p, TEXT("assets/enemy/spr_pomp_hurtfly/%d.png"), i); m_ImgHurtFly_R[i].Load(p); m_ImgHurtFly_L[i].Load(p); }
    for (int i = 0; i < 15; i++) { wsprintf(p, TEXT("assets/enemy/spr_pomp_hurtground/%d.png"), i); m_ImgHurtGround_R[i].Load(p); m_ImgHurtGround_L[i].Load(p); }
    for (int i = 0; i < 10; i++) { wsprintf(p, TEXT("assets/enemy/spr_pomp_run/%d.png"), i); m_ImgRun_R[i].Load(p); m_ImgRun_L[i].Load(p); }
}
void Pomp::Release() {
    for (int i = 0; i < 8; i++) { m_ImgIdle_R[i].Destroy(); m_ImgIdle_L[i].Destroy(); }
    for (int i = 0; i < 10; i++) { m_ImgWalk_R[i].Destroy(); m_ImgWalk_L[i].Destroy(); }
    for (int i = 0; i < 6; i++) { m_ImgAttack_R[i].Destroy(); m_ImgAttack_L[i].Destroy(); }
    for (int i = 0; i < 10; i++) { m_ImgBoxIdle_R[i].Destroy(); m_ImgBoxIdle_L[i].Destroy(); }
    for (int i = 0; i < 14; i++) { m_ImgBoxHit_R[i].Destroy(); m_ImgBoxHit_L[i].Destroy(); }
    for (int i = 0; i < 6; i++) { m_ImgTurn_R[i].Destroy(); m_ImgTurn_L[i].Destroy(); }
    for (int i = 0; i < 13; i++) { m_ImgFall_R[i].Destroy(); m_ImgFall_L[i].Destroy(); }
    for (int i = 0; i < 2; i++) { m_ImgHurtFly_R[i].Destroy(); m_ImgHurtFly_L[i].Destroy(); }
    for (int i = 0; i < 15; i++) { m_ImgHurtGround_R[i].Destroy(); m_ImgHurtGround_L[i].Destroy(); }
    for (int i = 0; i < 10; i++) { m_ImgRun_R[i].Destroy(); m_ImgRun_L[i].Destroy(); }
}
void Pomp::Update(float ts) {
    if (m_ActionState == PompAction::HURT_FLY || m_ActionState == PompAction::HURT_GROUND || m_State == EnemyState::DEAD) {
        if (m_ActionState == PompAction::HURT_FLY) {
            m_vy += 1.5f * ts; m_y += m_vy * ts; m_x += m_vx * ts;
            if (CheckCollision((int)m_x + (int)(m_colW / 2), (int)(m_y + m_colH))) {
                m_ActionState = PompAction::HURT_GROUND; m_State = EnemyState::DEAD; m_vx = 0; m_vy = 0; m_CurrentFrame = 0;
            }
        }
        if (GetTickCount() - m_LastTime >= (DWORD)(100.0f / ts)) {
            if (m_ActionState == PompAction::HURT_GROUND || m_State == EnemyState::DEAD) { if (m_CurrentFrame < 14) m_CurrentFrame++; }
            else m_CurrentFrame++;
            m_LastTime = GetTickCount();
        }
        return;
    }

    Enemy::Update(ts);
    DWORD ct = GetTickCount();

    float px = Player::GetInstance().GetX(), py = Player::GetInstance().GetY();
    float pw = Player::GetInstance().GetColW(), ph = Player::GetInstance().GetColH();
    UpdateDetection(px, py, pw, ph, ts);

    if (m_isPlayerDetected) {
        float dx = px - m_x;
        bool nextFacingLeft = (dx < 0);
        if (m_isFacingLeft != nextFacingLeft && m_ActionState != PompAction::TURN) {
            m_ActionState = PompAction::TURN;
            m_CurrentFrame = 0;
            m_patternTimer = ct;
        }
        m_isFacingLeft = nextFacingLeft;
        
        if (m_ActionState == PompAction::TURN) {
            m_vx = 0;
            if (ct - m_LastTime >= (DWORD)(100.0f / ts)) {
                m_CurrentFrame++; m_LastTime = ct;
                if (m_CurrentFrame >= 6) { m_ActionState = PompAction::NONE; }
            }
        }
        else if (m_ActionState == PompAction::ATTACK) {
            m_vx = 0;
            if (ct - m_LastTime >= (DWORD)(100.0f / ts)) {
                m_CurrentFrame++;
                m_LastTime = ct;
                if (m_CurrentFrame == 4) {
                    if (fabs(dx) < 120.0f && fabs(py - m_y) < 50.0f) {
                        Player::GetInstance().OnTakeDamage(1.0f);
                    }
                }
                if (m_CurrentFrame >= 6) {
                    m_ActionState = PompAction::NONE;
                    m_CurrentFrame = 0;
                    m_patternTimer = ct;
                }
            }
        } else if (fabs(dx) > 100.0f) {
            m_ActionState = PompAction::RUN;
            m_State = EnemyState::WALK;
            m_vx = m_isFacingLeft ? -10.0f : 10.0f;
            m_x += m_vx * ts;
        } else {
            if (m_ActionState == PompAction::RUN) m_ActionState = PompAction::NONE;
            m_vx = 0;
            if (ct - m_patternTimer > (DWORD)(600.0f / ts)) {
                m_ActionState = PompAction::ATTACK;
                m_CurrentFrame = 0;
                m_patternTimer = ct;
                m_LastTime = ct;
            }
        }
    } else {
        if (m_isWaiting) {
            m_vx = 0.0f; m_State = EnemyState::IDLE;
            if (ct - m_patternTimer >= (DWORD)(2000.0f / ts)) { m_isWaiting = false; m_patternTimer = ct; m_walkDistance = 0.0f; m_isFacingLeft = !m_isFacingLeft; m_vx = m_isFacingLeft ? -2.0f : 2.0f; }
        } else {
            m_State = EnemyState::WALK; int nx = (int)m_x + (int)(m_colW / 2) + (int)(m_vx * ts);
            if (!CheckCollision(nx, (int)m_y + (int)(m_colH * 0.9f))) {
                m_x += m_vx * ts; m_walkDistance += fabs(m_vx * ts); m_isFacingLeft = (m_vx < 0.0f);
                if (m_walkDistance >= 150.0f) { m_isWaiting = true; m_patternTimer = ct; }
            } else { m_isWaiting = true; m_patternTimer = ct; }
        }
    }

    if (m_vy > 5.0f) m_State = EnemyState::FALL;
    else if (m_vy == 0.0f && m_State == EnemyState::FALL) m_State = EnemyState::IDLE;

    if (ct - m_LastTime >= (DWORD)(100.0f / ts)) { m_CurrentFrame++; m_LastTime = ct; }
}
void Pomp::Render(HDC hdc, float camX, float camY, float ms, bool dr) {
    RenderExclaim(hdc, camX, camY, ms);
    int sx = (int)((m_x - camX) * ms), sy = (int)((m_y - camY) * ms);
    CImage* img = nullptr; float msX = 1.0f, msY = 1.0f, es = 1.8f;
    if (m_isFacingLeft) {
        if (m_ActionState == PompAction::HURT_FLY) img = &m_ImgHurtFly_L[m_CurrentFrame % 2];
        else if (m_ActionState == PompAction::HURT_GROUND || m_State == EnemyState::DEAD) img = &m_ImgHurtGround_L[m_CurrentFrame % 15];
        else if (m_State == EnemyState::FALL) img = &m_ImgFall_L[m_CurrentFrame % 13];
        else {
            switch (m_ActionState) {
            case PompAction::NONE:
                if (m_State == EnemyState::IDLE) { img = &m_ImgIdle_L[m_CurrentFrame % 8]; msX = 1.1f; msY = 1.1f; }
                else if (m_State == EnemyState::WALK) img = &m_ImgWalk_L[m_CurrentFrame % 10];
                break;
            case PompAction::ATTACK: img = &m_ImgAttack_L[m_CurrentFrame % 6]; break;
            case PompAction::BOX_IDLE: img = &m_ImgBoxIdle_L[m_CurrentFrame % 10]; break;
            case PompAction::BOX_HIT: img = &m_ImgBoxHit_L[m_CurrentFrame % 14]; break;
            case PompAction::TURN: img = &m_ImgTurn_L[m_CurrentFrame % 6]; break;
            case PompAction::RUN: img = &m_ImgRun_L[m_CurrentFrame % 10]; break;
            }
        }
    } else {
        if (m_ActionState == PompAction::HURT_FLY) img = &m_ImgHurtFly_R[m_CurrentFrame % 2];
        else if (m_ActionState == PompAction::HURT_GROUND || m_State == EnemyState::DEAD) img = &m_ImgHurtGround_R[m_CurrentFrame % 15];
        else if (m_State == EnemyState::FALL) img = &m_ImgFall_R[m_CurrentFrame % 13];
        else {
            switch (m_ActionState) {
            case PompAction::NONE:
                if (m_State == EnemyState::IDLE) { img = &m_ImgIdle_R[m_CurrentFrame % 8]; msX = 1.1f; msY = 1.1f; }
                else if (m_State == EnemyState::WALK) img = &m_ImgWalk_R[m_CurrentFrame % 10];
                break;
            case PompAction::ATTACK: img = &m_ImgAttack_R[m_CurrentFrame % 6]; break;
            case PompAction::BOX_IDLE: img = &m_ImgBoxIdle_R[m_CurrentFrame % 10]; break;
            case PompAction::BOX_HIT: img = &m_ImgBoxHit_R[m_CurrentFrame % 14]; break;
            case PompAction::TURN: img = &m_ImgTurn_R[m_CurrentFrame % 6]; break;
            case PompAction::RUN: img = &m_ImgRun_R[m_CurrentFrame % 10]; break;
            }
        }
    }
    if (img && !img->IsNull()) {
        int fw = (int)(img->GetWidth() * es * msX * ms), fh = (int)(img->GetHeight() * es * msY * ms);
        int fy = sy + (int)(m_colH * ms) - fh, dx = sx + (int)(m_colW * ms / 2) - (fw / 2);
        if (m_isFacingLeft) {
            int om = SetGraphicsMode(hdc, GM_ADVANCED); XFORM xo; GetWorldTransform(hdc, &xo);
            XFORM xl = { -1.0f, 0.0f, 0.0f, 1.0f, (float)(2 * dx + fw), 0.0f }; SetWorldTransform(hdc, &xl);
            img->Draw(hdc, dx, fy, fw, fh); SetWorldTransform(hdc, &xo); SetGraphicsMode(hdc, om);
        } else img->Draw(hdc, dx, fy, fw, fh);
    }
    if (dr) {
        RenderDetectionRange(hdc, camX, camY, ms);
        int sw = (int)(m_colW * ms), sh = (int)(m_colH * ms);
        HBRUSH hr = CreateSolidBrush(RGB(255, 0, 0)); RECT r = { sx, sy, sx + sw, sy + sh }; FrameRect(hdc, &r, hr); DeleteObject(hr);
    }
}
void Pomp::OnTakeDamage(float d) { m_isAlive = false; m_ActionState = PompAction::HURT_FLY; m_vy = -15.0f; m_vx = m_isFacingLeft ? 8.0f : -8.0f; m_CurrentFrame = 0; }

CImage ShieldCop::m_ImgIdle_R[6], ShieldCop::m_ImgIdle_L[6], ShieldCop::m_ImgWalk_R[10], ShieldCop::m_ImgWalk_L[10], ShieldCop::m_ImgRun_R[10], ShieldCop::m_ImgRun_L[10], ShieldCop::m_ImgTurn_R[8], ShieldCop::m_ImgTurn_L[8], ShieldCop::m_ImgAim_R[19], ShieldCop::m_ImgAim_L[19], ShieldCop::m_ImgBash_R[6], ShieldCop::m_ImgBash_L[6], ShieldCop::m_ImgKnockback_R[2], ShieldCop::m_ImgKnockback_L[2], ShieldCop::m_ImgTragedyDie_R[15], ShieldCop::m_ImgTragedyDie_L[15];
ShieldCop::ShieldCop(float x, float y) : Enemy(x, y, EnemyType::SHIELDCOP) { m_ActionState = ShieldCopAction::NONE; }
ShieldCop::~ShieldCop() {}
void ShieldCop::Reset() { Enemy::Reset(); m_ActionState = ShieldCopAction::NONE; }
void ShieldCop::Init() {
    if (!m_ImgIdle_R[0].IsNull()) return;
    TCHAR p[256];
    for (int i = 0; i < 6; i++) { wsprintf(p, TEXT("assets/enemy/spr_shieldcop_idle/%d.png"), i); m_ImgIdle_R[i].Load(p); m_ImgIdle_L[i].Load(p); }
    for (int i = 0; i < 10; i++) { wsprintf(p, TEXT("assets/enemy/spr_shieldcop_walk/%d.png"), i); m_ImgWalk_R[i].Load(p); m_ImgWalk_L[i].Load(p); }
    for (int i = 0; i < 10; i++) { wsprintf(p, TEXT("assets/enemy/spr_shieldcop_run/%d.png"), i); m_ImgRun_R[i].Load(p); m_ImgRun_L[i].Load(p); }
    for (int i = 0; i < 8; i++) { wsprintf(p, TEXT("assets/enemy/spr_shieldcop_turn/%d.png"), i); m_ImgTurn_R[i].Load(p); m_ImgTurn_L[i].Load(p); }
    for (int i = 0; i < 19; i++) { wsprintf(p, TEXT("assets/enemy/spr_shieldcop_aim/%d.png"), i); m_ImgAim_R[i].Load(p); m_ImgAim_L[i].Load(p); }
    for (int i = 0; i < 6; i++) { wsprintf(p, TEXT("assets/enemy/spr_shieldcop_bash/%d.png"), i); m_ImgBash_R[i].Load(p); m_ImgBash_L[i].Load(p); }
    for (int i = 0; i < 2; i++) { wsprintf(p, TEXT("assets/enemy/spr_shieldcop_knockback/%d.png"), i); m_ImgKnockback_R[i].Load(p); m_ImgKnockback_L[i].Load(p); }
    for (int i = 0; i < 15; i++) { wsprintf(p, TEXT("assets/enemy/spr_shieldcop_tragedy_die_1/%d.png"), i); m_ImgTragedyDie_R[i].Load(p); m_ImgTragedyDie_L[i].Load(p); }
}
void ShieldCop::Release() {
    for (int i = 0; i < 6; i++) { m_ImgIdle_R[i].Destroy(); m_ImgIdle_L[i].Destroy(); }
    for (int i = 0; i < 10; i++) { m_ImgWalk_R[i].Destroy(); m_ImgWalk_L[i].Destroy(); }
    for (int i = 0; i < 10; i++) { m_ImgRun_R[i].Destroy(); m_ImgRun_L[i].Destroy(); }
    for (int i = 0; i < 8; i++) { m_ImgTurn_R[i].Destroy(); m_ImgTurn_L[i].Destroy(); }
    for (int i = 0; i < 19; i++) { m_ImgAim_R[i].Destroy(); m_ImgAim_L[i].Destroy(); }
    for (int i = 0; i < 6; i++) { m_ImgBash_R[i].Destroy(); m_ImgBash_L[i].Destroy(); }
    for (int i = 0; i < 2; i++) { m_ImgKnockback_R[i].Destroy(); m_ImgKnockback_L[i].Destroy(); }
    for (int i = 0; i < 15; i++) { m_ImgTragedyDie_R[i].Destroy(); m_ImgTragedyDie_L[i].Destroy(); }
}
void ShieldCop::Update(float ts) {
    if (m_ActionState == ShieldCopAction::HURT_FLY || m_ActionState == ShieldCopAction::HURT_GROUND || m_State == EnemyState::DEAD) {
        if (m_ActionState == ShieldCopAction::HURT_FLY) {
            m_vy += 1.5f * ts; m_y += m_vy * ts; m_x += m_vx * ts;
            if (CheckCollision((int)m_x + (int)(m_colW / 2), (int)(m_y + m_colH))) {
                m_ActionState = ShieldCopAction::HURT_GROUND; m_State = EnemyState::DEAD; m_vx = 0; m_vy = 0; m_CurrentFrame = 0;
            }
        }
        if (GetTickCount() - m_LastTime >= (DWORD)(100.0f / ts)) {
            if (m_ActionState == ShieldCopAction::HURT_GROUND || m_State == EnemyState::DEAD) { if (m_CurrentFrame < 14) m_CurrentFrame++; }
            else m_CurrentFrame++;
            m_LastTime = GetTickCount();
        }
        return;
    }

    Enemy::Update(ts);
    DWORD ct = GetTickCount();

    float px = Player::GetInstance().GetX(), py = Player::GetInstance().GetY();
    float pw = Player::GetInstance().GetColW(), ph = Player::GetInstance().GetColH();
    UpdateDetection(px, py, pw, ph, ts);

    if (m_isPlayerDetected) {
        float dx = px - m_x;
        bool nextFacingLeft = (dx < 0);
        if (m_isFacingLeft != nextFacingLeft && m_ActionState != ShieldCopAction::TURN) {
            m_ActionState = ShieldCopAction::TURN;
            m_CurrentFrame = 0;
            m_patternTimer = ct;
        }
        m_isFacingLeft = nextFacingLeft;
        
        if (m_ActionState == ShieldCopAction::TURN) {
            m_vx = 0;
            if (ct - m_LastTime >= (DWORD)(100.0f / ts)) {
                m_CurrentFrame++; m_LastTime = ct;
                if (m_CurrentFrame >= 8) { m_ActionState = ShieldCopAction::NONE; }
            }
        }
        else if (fabs(dx) > 120.0f) {
            m_ActionState = ShieldCopAction::RUN;
            m_State = EnemyState::WALK;
            m_vx = m_isFacingLeft ? -7.0f : 7.0f;
            m_x += m_vx * ts;
        } else {
            if (m_ActionState == ShieldCopAction::RUN) m_ActionState = ShieldCopAction::NONE;
            m_vx = 0;
            if (ct - m_patternTimer > (DWORD)(1200.0f / ts)) {
                m_ActionState = ShieldCopAction::BASH;
                m_CurrentFrame = 0;
                m_patternTimer = ct;
                if (fabs(py - m_y) < 50.0f) Player::GetInstance().OnTakeDamage(1.0f);
            }
        }
    } else {
        if (m_isWaiting) {
            m_vx = 0.0f; m_State = EnemyState::IDLE;
            if (ct - m_patternTimer >= (DWORD)(2000.0f / ts)) { m_isWaiting = false; m_patternTimer = ct; m_walkDistance = 0.0f; m_isFacingLeft = !m_isFacingLeft; m_vx = m_isFacingLeft ? -2.0f : 2.0f; }
        } else {
            m_State = EnemyState::WALK; int nx = (int)m_x + (int)(m_colW / 2) + (int)(m_vx * ts);
            if (!CheckCollision(nx, (int)m_y + (int)(m_colH * 0.9f))) {
                m_x += m_vx * ts; m_walkDistance += fabs(m_vx * ts); m_isFacingLeft = (m_vx < 0.0f);
                if (m_walkDistance >= 150.0f) { m_isWaiting = true; m_patternTimer = ct; }
            } else { m_isWaiting = true; m_patternTimer = ct; }
        }
    }

    if (m_vy > 5.0f) m_State = EnemyState::FALL;
    else if (m_vy == 0.0f && m_State == EnemyState::FALL) m_State = EnemyState::IDLE;

    if (ct - m_LastTime >= (DWORD)(100.0f / ts)) { m_CurrentFrame++; m_LastTime = ct; }
}
void ShieldCop::Render(HDC hdc, float camX, float camY, float ms, bool dr) {
    RenderExclaim(hdc, camX, camY, ms);
    int sx = (int)((m_x - camX) * ms), sy = (int)((m_y - camY) * ms);
    CImage* img = nullptr; float msX = 1.0f, msY = 1.0f, es = 1.8f;
    if (m_isFacingLeft) {
        if (m_ActionState == ShieldCopAction::HURT_FLY) img = &m_ImgKnockback_L[m_CurrentFrame % 2];
        else if (m_ActionState == ShieldCopAction::HURT_GROUND || m_State == EnemyState::DEAD) img = &m_ImgTragedyDie_L[m_CurrentFrame % 15];
        else if (m_State == EnemyState::FALL) img = &m_ImgIdle_L[m_CurrentFrame % 6];
        else {
            switch (m_ActionState) {
            case ShieldCopAction::NONE:
                if (m_State == EnemyState::IDLE) { img = &m_ImgIdle_L[m_CurrentFrame % 6]; msX = 1.1f; msY = 1.1f; }
                else if (m_State == EnemyState::WALK) img = &m_ImgWalk_L[m_CurrentFrame % 10];
                break;
            case ShieldCopAction::AIM: img = &m_ImgAim_L[m_CurrentFrame % 19]; break;
            case ShieldCopAction::BASH: img = &m_ImgBash_L[m_CurrentFrame % 6]; break;
            case ShieldCopAction::TURN: img = &m_ImgTurn_L[m_CurrentFrame % 8]; break;
            case ShieldCopAction::RUN: img = &m_ImgRun_L[m_CurrentFrame % 10]; break;
            }
        }
    } else {
        if (m_ActionState == ShieldCopAction::HURT_FLY) img = &m_ImgKnockback_R[m_CurrentFrame % 2];
        else if (m_ActionState == ShieldCopAction::HURT_GROUND || m_State == EnemyState::DEAD) img = &m_ImgTragedyDie_R[m_CurrentFrame % 15];
        else if (m_State == EnemyState::FALL) img = &m_ImgIdle_R[m_CurrentFrame % 6];
        else {
            switch (m_ActionState) {
            case ShieldCopAction::NONE:
                if (m_State == EnemyState::IDLE) { img = &m_ImgIdle_R[m_CurrentFrame % 6]; msX = 1.1f; msY = 1.1f; }
                else if (m_State == EnemyState::WALK) img = &m_ImgWalk_R[m_CurrentFrame % 10];
                break;
            case ShieldCopAction::AIM: img = &m_ImgAim_R[m_CurrentFrame % 19]; break;
            case ShieldCopAction::BASH: img = &m_ImgBash_R[m_CurrentFrame % 6]; break;
            case ShieldCopAction::TURN: img = &m_ImgTurn_R[m_CurrentFrame % 8]; break;
            case ShieldCopAction::RUN: img = &m_ImgRun_R[m_CurrentFrame % 10]; break;
            }
        }
    }
    if (img && !img->IsNull()) {
        int fw = (int)(img->GetWidth() * es * msX * ms), fh = (int)(img->GetHeight() * es * msY * ms);
        int fy = sy + (int)(m_colH * ms) - fh, dx = sx + (int)(m_colW * ms / 2) - (fw / 2);
        if (m_isFacingLeft) {
            int om = SetGraphicsMode(hdc, GM_ADVANCED); XFORM xo; GetWorldTransform(hdc, &xo);
            XFORM xl = { -1.0f, 0.0f, 0.0f, 1.0f, (float)(2 * dx + fw), 0.0f }; SetWorldTransform(hdc, &xl);
            img->Draw(hdc, dx, fy, fw, fh); SetWorldTransform(hdc, &xo); SetGraphicsMode(hdc, om);
        } else img->Draw(hdc, dx, fy, fw, fh);
    }
    if (dr) {
        RenderDetectionRange(hdc, camX, camY, ms);
        int sw = (int)(m_colW * ms), sh = (int)(m_colH * ms);
        HBRUSH hr = CreateSolidBrush(RGB(255, 0, 0)); RECT r = { sx, sy, sx + sw, sy + sh }; FrameRect(hdc, &r, hr); DeleteObject(hr);
    }
}
void ShieldCop::OnTakeDamage(float d) { m_isAlive = false; m_ActionState = ShieldCopAction::HURT_FLY; m_vy = -15.0f; m_vx = m_isFacingLeft ? 8.0f : -8.0f; m_CurrentFrame = 0; }
