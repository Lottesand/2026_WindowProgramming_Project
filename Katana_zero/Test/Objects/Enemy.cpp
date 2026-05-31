#include "Enemy.h"
#include "Physics.h"
#include "../SceneAndMap/Camera.h"

Enemy::Enemy(float startX, float startY, EnemyType type) {
    m_startX = startX; m_startY = startY; m_x = startX; m_y = startY;
    m_vx = 2.0f; m_vy = 0.0f; m_colW = 40.0f; m_colH = 60.0f;
    m_isAlive = true; m_isFacingLeft = false; m_Type = type; m_State = EnemyState::IDLE;
    m_CurrentFrame = 0; m_LastTime = GetTickCount(); m_friction = 0.92f; m_knockbackVx = 0.0f;
    m_isImmortal = false; m_patternTimer = GetTickCount(); m_isWaiting = false; m_walkDistance = 0.0f;
}
Enemy::~Enemy() {}

void Enemy::Reset() {
    m_x = m_startX; m_y = m_startY; m_vx = 2.0f; m_vy = 0.0f;
    m_isAlive = true; m_isFacingLeft = false; m_State = EnemyState::IDLE;
    m_CurrentFrame = 0; m_LastTime = GetTickCount(); m_knockbackVx = 0.0f;
    m_patternTimer = GetTickCount(); m_isWaiting = false; m_walkDistance = 0.0f;
}

void Enemy::OnTakeDamage(float damage) { if (!m_isImmortal) { m_isAlive = false; m_State = EnemyState::DEAD; } }
void Enemy::ApplyKnockback(float vx) { m_knockbackVx = vx; }

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
    int fx = (int)m_x + (int)(m_colW / 2), nfy = (int)m_y + (int)m_colH + (int)m_vy;
    if (CheckCollision(fx, nfy)) { m_vy = 0.0f; while (CheckCollision(fx, (int)m_y + (int)m_colH)) m_y -= 1.0f; } else m_y += m_vy;
}

// Gangster
Gangster::Gangster(float x, float y) : Enemy(x, y, EnemyType::GANGSTER) { m_ActionState = GangsterAction::NONE; }
Gangster::~Gangster() {}
void Gangster::Reset() { Enemy::Reset(); m_ActionState = GangsterAction::NONE; }
void Gangster::Init() {
    TCHAR path[256];
    for (int i = 0; i < 8; i++) { wsprintf(path, TEXT("assets/enemy/spr_gangsteridle/%d.png"), i); m_ImgIdle_R[i].Load(path); m_ImgIdle_L[i].Load(path); }
    for (int i = 0; i < 8; i++) { wsprintf(path, TEXT("assets/enemy/spr_gangsterwalk/%d.png"), i); m_ImgWalk_R[i].Load(path); m_ImgWalk_L[i].Load(path); }
    for (int i = 0; i < 7; i++) { wsprintf(path, TEXT("assets/enemy/spr_gangster_aim/%d.png"), i); m_ImgAim_R[i].Load(path); m_ImgAim_L[i].Load(path); }
    for (int i = 0; i < 6; i++) { wsprintf(path, TEXT("assets/enemy/spr_fire_1/%d.png"), i); m_ImgFire_R[i].Load(path); m_ImgFire_L[i].Load(path); }
    for (int i = 0; i < 6; i++) { wsprintf(path, TEXT("assets/enemy/spr_gangsterturn/%d.png"), i); m_ImgTurn_R[i].Load(path); m_ImgTurn_L[i].Load(path); }
    for (int i = 0; i < 12; i++) { wsprintf(path, TEXT("assets/enemy/spr_gangsterfall/%d.png"), i); m_ImgFall_R[i].Load(path); m_ImgFall_L[i].Load(path); }
    for (int i = 0; i < 2; i++) { wsprintf(path, TEXT("assets/enemy/spr_gangsterhurtfly/%d.png"), i); m_ImgHurtFly_R[i].Load(path); m_ImgHurtFly_L[i].Load(path); }
    for (int i = 0; i < 14; i++) { wsprintf(path, TEXT("assets/enemy/spr_gangsterhurtground/%d.png"), i); m_ImgHurtGround_R[i].Load(path); m_ImgHurtGround_L[i].Load(path); }
    for (int i = 0; i < 10; i++) { wsprintf(path, TEXT("assets/enemy/spr_gangsterrun/%d.png"), i); m_ImgRun_R[i].Load(path); m_ImgRun_L[i].Load(path); }
}
void Gangster::Update(float ts) {
    if (m_ActionState == GangsterAction::HURT_FLY || m_ActionState == GangsterAction::HURT_GROUND || m_State == EnemyState::DEAD) {
        if (m_ActionState == GangsterAction::HURT_FLY) { m_vy += 1.5f * ts; m_y += m_vy; m_x += m_vx * ts; if (CheckCollision((int)m_x + (int)(m_colW/2), (int)m_y + (int)m_colH)) { m_ActionState = GangsterAction::HURT_GROUND; m_State = EnemyState::DEAD; m_vx = 0; m_vy = 0; m_CurrentFrame = 0; } }
        DWORD ct = GetTickCount(); if (ct - m_LastTime >= (DWORD)(100 / ts)) { m_CurrentFrame++; m_LastTime = ct; }
        return;
    }
    Enemy::Update(ts);
    DWORD cpt = GetTickCount();
    if (m_isWaiting) { m_vx = 0.0f; m_State = EnemyState::IDLE; if (cpt - m_patternTimer >= (DWORD)(2000 / ts)) { m_isWaiting = false; m_patternTimer = cpt; m_walkDistance = 0.0f; m_isFacingLeft = !m_isFacingLeft; m_vx = m_isFacingLeft ? -2.0f : 2.0f; } }
    else { m_State = EnemyState::WALK; int nx = (int)m_x + (int)(m_colW/2) + (int)(m_vx * ts); if (!CheckCollision(nx, (int)m_y + (int)(m_colH * 0.9f))) { m_x += m_vx * ts; m_walkDistance += fabs(m_vx * ts); m_isFacingLeft = (m_vx < 0.0f); if (m_walkDistance >= 150.0f) { m_isWaiting = true; m_patternTimer = cpt; } } else { m_isWaiting = true; m_patternTimer = cpt; } }
    if (m_vy > 0.1f) m_State = EnemyState::FALL;
    DWORD ct = GetTickCount(); if (ct - m_LastTime >= (DWORD)(100 / ts)) { m_CurrentFrame++; m_LastTime = ct; }
}
void Gangster::Render(HDC h, float cx, float cy, float ms, bool dr) {
    if (!h) return;
    int sx = (int)((m_x - cx) * ms), sy = (int)((m_y - cy) * ms);
    CImage* img = nullptr; float msX = 1.0f, msY = 1.0f; int oy = 0;
    int safeF = (std::max)(0, m_CurrentFrame);
    if (m_isFacingLeft) { if (m_ActionState == GangsterAction::HURT_FLY) img = &m_ImgHurtFly_L[safeF % 2]; else if (m_ActionState == GangsterAction::HURT_GROUND || m_State == EnemyState::DEAD) img = &m_ImgHurtGround_L[safeF % 14]; else if (m_State == EnemyState::FALL) img = &m_ImgFall_L[safeF % 12]; else { switch (m_ActionState) { case GangsterAction::NONE: if (m_State == EnemyState::IDLE) { img = &m_ImgIdle_L[safeF % 8]; msX = 1.6f; msY = 1.6f; oy = -35; } else img = &m_ImgWalk_L[safeF % 8]; break; case GangsterAction::AIM: img = &m_ImgAim_L[safeF % 7]; break; case GangsterAction::FIRE: img = &m_ImgFire_L[safeF % 6]; break; case GangsterAction::TURN: img = &m_ImgTurn_L[safeF % 6]; break; case GangsterAction::RUN: img = &m_ImgRun_L[safeF % 10]; break; } } }
    else { if (m_ActionState == GangsterAction::HURT_FLY) img = &m_ImgHurtFly_R[safeF % 2]; else if (m_ActionState == GangsterAction::HURT_GROUND || m_State == EnemyState::DEAD) img = &m_ImgHurtGround_R[safeF % 14]; else if (m_State == EnemyState::FALL) img = &m_ImgFall_R[safeF % 12]; else { switch (m_ActionState) { case GangsterAction::NONE: if (m_State == EnemyState::IDLE) { img = &m_ImgIdle_R[safeF % 8]; msX = 1.6f; msY = 1.6f; oy = -35; } else img = &m_ImgWalk_R[safeF % 8]; break; case GangsterAction::AIM: img = &m_ImgAim_R[safeF % 7]; break; case GangsterAction::FIRE: img = &m_ImgFire_R[safeF % 6]; break; case GangsterAction::TURN: img = &m_ImgTurn_R[safeF % 6]; break; case GangsterAction::RUN: img = &m_ImgRun_R[safeF % 10]; break; } } }
    if (img && !img->IsNull()) { int fw = (int)(40 * msX * ms), fh = (int)(60 * msY * ms), fy = sy + (int)(oy * ms); if (m_isFacingLeft) { int om = SetGraphicsMode(h, GM_ADVANCED); XFORM xfO; GetWorldTransform(h, &xfO); XFORM xfL; xfL.eM11 = -1.0f; xfL.eM12 = 0.0f; xfL.eM21 = 0.0f; xfL.eM22 = 1.0f; xfL.eDx = (float)(2 * sx + fw); xfL.eDy = 0.0f; SetWorldTransform(h, &xfL); img->Draw(h, sx, fy, fw, fh); SetWorldTransform(h, &xfO); SetGraphicsMode(h, om); } else img->Draw(h, sx, fy, fw, fh); }
    else Rectangle(h, sx, sy, sx + (int)(m_colW * ms), sy + (int)(m_colH * ms));
    if (dr) { HBRUSH rb = CreateSolidBrush(RGB(255, 0, 0)); RECT r = { sx, sy, sx + (int)(m_colW * ms), sy + (int)(m_colH * ms) }; FrameRect(h, &r, rb); DeleteObject(rb); }
}

void Gangster::OnTakeDamage(float damage) { if (m_isImmortal) return; m_isAlive = false; m_ActionState = GangsterAction::HURT_FLY; m_vy = -15.0f; m_vx = m_isFacingLeft ? 8.0f : -8.0f; m_CurrentFrame = 0; }

// Grunt
Grunt::Grunt(float x, float y) : Enemy(x, y, EnemyType::GRUNT) { m_ActionState = GruntAction::NONE; }
Grunt::~Grunt() {}
void Grunt::Reset() { Enemy::Reset(); m_ActionState = GruntAction::NONE; }
void Grunt::Init() {
    TCHAR path[256];
    for (int i = 0; i < 8; i++) { wsprintf(path, TEXT("assets/enemy/spr_grunt_idle/%d.png"), i); m_ImgIdle_R[i].Load(path); m_ImgIdle_L[i].Load(path); }
    for (int i = 0; i < 10; i++) { wsprintf(path, TEXT("assets/enemy/spr_grunt_walk/%d.png"), i); m_ImgWalk_R[i].Load(path); m_ImgWalk_L[i].Load(path); }
    for (int i = 0; i < 8; i++) { wsprintf(path, TEXT("assets/enemy/spr_grunt_attack/%d.png"), i); m_ImgAttack_R[i].Load(path); m_ImgAttack_L[i].Load(path); }
    for (int i = 0; i < 5; i++) { wsprintf(path, TEXT("assets/enemy/spr_gruntslash/%d.png"), i); m_ImgSlash_R[i].Load(path); m_ImgSlash_L[i].Load(path); }
    for (int i = 0; i < 8; i++) { wsprintf(path, TEXT("assets/enemy/spr_grunt_turn/%d.png"), i); m_ImgTurn_R[i].Load(path); m_ImgTurn_L[i].Load(path); }
    for (int i = 0; i < 13; i++) { wsprintf(path, TEXT("assets/enemy/spr_grunt_fall/%d.png"), i); m_ImgFall_R[i].Load(path); m_ImgFall_L[i].Load(path); }
    for (int i = 0; i < 2; i++) { wsprintf(path, TEXT("assets/enemy/spr_grunt_hurtfly/%d.png"), i); m_ImgHurtFly_R[i].Load(path); m_ImgHurtFly_L[i].Load(path); }
    for (int i = 0; i < 16; i++) { wsprintf(path, TEXT("assets/enemy/spr_grunt_hurtground/%d.png"), i); m_ImgHurtGround_R[i].Load(path); m_ImgHurtGround_L[i].Load(path); }
    for (int i = 0; i < 10; i++) { wsprintf(path, TEXT("assets/enemy/spr_grunt_run/%d.png"), i); m_ImgRun_R[i].Load(path); m_ImgRun_L[i].Load(path); }
}
void Grunt::Update(float ts) {
    if (m_ActionState == GruntAction::HURT_FLY || m_ActionState == GruntAction::HURT_GROUND || m_State == EnemyState::DEAD) {
        if (m_ActionState == GruntAction::HURT_FLY) { m_vy += 1.5f * ts; m_y += m_vy; m_x += m_vx * ts; if (CheckCollision((int)m_x + (int)(m_colW/2), (int)m_y + (int)m_colH)) { m_ActionState = GruntAction::HURT_GROUND; m_State = EnemyState::DEAD; m_vx = 0; m_vy = 0; m_CurrentFrame = 0; } }
        DWORD ct = GetTickCount(); if (ct - m_LastTime >= (DWORD)(100 / ts)) { m_CurrentFrame++; m_LastTime = ct; }
        return;
    }
    Enemy::Update(ts);
    DWORD cpt = GetTickCount();
    if (m_isWaiting) { m_vx = 0.0f; m_State = EnemyState::IDLE; if (cpt - m_patternTimer >= (DWORD)(1500 / ts)) { m_isWaiting = false; m_patternTimer = cpt; m_walkDistance = 0.0f; m_isFacingLeft = !m_isFacingLeft; m_vx = m_isFacingLeft ? -2.5f : 2.5f; } }
    else { m_State = EnemyState::WALK; int nx = (int)m_x + (int)(m_colW/2) + (int)(m_vx * ts); if (!CheckCollision(nx, (int)m_y + (int)(m_colH * 0.9f))) { m_x += m_vx * ts; m_walkDistance += fabs(m_vx * ts); m_isFacingLeft = (m_vx < 0.0f); if (m_walkDistance >= 120.0f) { m_isWaiting = true; m_patternTimer = cpt; } } else { m_isWaiting = true; m_patternTimer = cpt; } }
    if (m_vy > 0.1f) m_State = EnemyState::FALL;
    DWORD ct = GetTickCount(); if (ct - m_LastTime >= (DWORD)(100 / ts)) { m_CurrentFrame++; m_LastTime = ct; }
}
void Grunt::Render(HDC h, float cx, float cy, float ms, bool dr) {
    if (!h) return;
    int sx = (int)((m_x - cx) * ms), sy = (int)((m_y - cy) * ms);
    CImage* img = nullptr;
    int safeF = (std::max)(0, m_CurrentFrame);
    if (m_isFacingLeft) { if (m_ActionState == GruntAction::HURT_FLY) img = &m_ImgHurtFly_L[safeF % 2]; else if (m_ActionState == GruntAction::HURT_GROUND || m_State == EnemyState::DEAD) img = &m_ImgHurtGround_L[safeF % 16]; else if (m_State == EnemyState::FALL) img = &m_ImgFall_L[safeF % 13]; else { switch (m_ActionState) { case GruntAction::NONE: if (m_State == EnemyState::IDLE) img = &m_ImgIdle_L[safeF % 8]; else img = &m_ImgWalk_L[safeF % 10]; break; case GruntAction::ATTACK: img = &m_ImgAttack_L[safeF % 8]; break; case GruntAction::SLASH: img = &m_ImgSlash_L[safeF % 5]; break; case GruntAction::TURN: img = &m_ImgTurn_L[safeF % 8]; break; case GruntAction::RUN: img = &m_ImgRun_L[safeF % 10]; break; } } }
    else { if (m_ActionState == GruntAction::HURT_FLY) img = &m_ImgHurtFly_R[safeF % 2]; else if (m_ActionState == GruntAction::HURT_GROUND || m_State == EnemyState::DEAD) img = &m_ImgHurtGround_R[safeF % 16]; else if (m_State == EnemyState::FALL) img = &m_ImgFall_R[safeF % 13]; else { switch (m_ActionState) { case GruntAction::NONE: if (m_State == EnemyState::IDLE) img = &m_ImgIdle_R[safeF % 8]; else img = &m_ImgWalk_R[safeF % 10]; break; case GruntAction::ATTACK: img = &m_ImgAttack_R[safeF % 8]; break; case GruntAction::SLASH: img = &m_ImgSlash_R[safeF % 5]; break; case GruntAction::TURN: img = &m_ImgTurn_R[safeF % 8]; break; case GruntAction::RUN: img = &m_ImgRun_R[safeF % 10]; break; } } }
    if (img && !img->IsNull()) { int fw = (int)(40 * ms), fh = (int)(60 * ms); if (m_isFacingLeft) { int om = SetGraphicsMode(h, GM_ADVANCED); XFORM xfO; GetWorldTransform(h, &xfO); XFORM xfL; xfL.eM11 = -1.0f; xfL.eM12 = 0.0f; xfL.eM21 = 0.0f; xfL.eM22 = 1.0f; xfL.eDx = (float)(2 * sx + fw); xfL.eDy = 0.0f; SetWorldTransform(h, &xfL); img->Draw(h, sx, sy, fw, fh); SetWorldTransform(h, &xfO); SetGraphicsMode(h, om); } else img->Draw(h, sx, sy, fw, fh); }
    else Rectangle(h, sx, sy, sx + (int)(m_colW * ms), sy + (int)(m_colH * ms));
    if (dr) { HBRUSH rb = CreateSolidBrush(RGB(255, 0, 0)); RECT r = { sx, sy, sx + (int)(m_colW * ms), sy + (int)(m_colH * ms) }; FrameRect(h, &r, rb); DeleteObject(rb); }
}

void Grunt::OnTakeDamage(float damage) { if (m_isImmortal) return; m_isAlive = false; m_ActionState = GruntAction::HURT_FLY; m_vy = -15.0f; m_vx = m_isFacingLeft ? 8.0f : -8.0f; m_CurrentFrame = 0; }

// Pomp
Pomp::Pomp(float x, float y) : Enemy(x, y, EnemyType::POMP) { m_ActionState = PompAction::NONE; }
Pomp::~Pomp() {}
void Pomp::Reset() { Enemy::Reset(); m_ActionState = PompAction::NONE; }
void Pomp::Init() {
    TCHAR path[256];
    for (int i = 0; i < 8; i++) { wsprintf(path, TEXT("assets/enemy/spr_pomp_idle/%d.png"), i); m_ImgIdle_R[i].Load(path); m_ImgIdle_L[i].Load(path); }
    for (int i = 0; i < 10; i++) { wsprintf(path, TEXT("assets/enemy/spr_pomp_walk/%d.png"), i); m_ImgWalk_R[i].Load(path); m_ImgWalk_L[i].Load(path); }
    for (int i = 0; i < 6; i++) { wsprintf(path, TEXT("assets/enemy/spr_pomp_attack/%d.png"), i); m_ImgAttack_R[i].Load(path); m_ImgAttack_L[i].Load(path); }
    for (int i = 0; i < 10; i++) { wsprintf(path, TEXT("assets/enemy/spr_pomp_box_idle/%d.png"), i); m_ImgBoxIdle_R[i].Load(path); m_ImgBoxIdle_L[i].Load(path); }
    for (int i = 0; i < 14; i++) { wsprintf(path, TEXT("assets/enemy/spr_pomp_box_hit/%d.png"), i); m_ImgBoxHit_R[i].Load(path); m_ImgBoxHit_L[i].Load(path); }
    for (int i = 0; i < 6; i++) { wsprintf(path, TEXT("assets/enemy/spr_pomp_turn/%d.png"), i); m_ImgTurn_R[i].Load(path); m_ImgTurn_L[i].Load(path); }
    for (int i = 0; i < 13; i++) { wsprintf(path, TEXT("assets/enemy/spr_pomp_fall/%d.png"), i); m_ImgFall_R[i].Load(path); m_ImgFall_L[i].Load(path); }
    for (int i = 0; i < 2; i++) { wsprintf(path, TEXT("assets/enemy/spr_pomp_hurtfly/%d.png"), i); m_ImgHurtFly_R[i].Load(path); m_ImgHurtFly_L[i].Load(path); }
    for (int i = 0; i < 15; i++) { wsprintf(path, TEXT("assets/enemy/spr_pomp_hurtground/%d.png"), i); m_ImgHurtGround_R[i].Load(path); m_ImgHurtGround_L[i].Load(path); }
    for (int i = 0; i < 10; i++) { wsprintf(path, TEXT("assets/enemy/spr_pomp_run/%d.png"), i); m_ImgRun_R[i].Load(path); m_ImgRun_L[i].Load(path); }
}
void Pomp::Update(float ts) {
    if (m_ActionState == PompAction::HURT_FLY || m_ActionState == PompAction::HURT_GROUND || m_State == EnemyState::DEAD) {
        if (m_ActionState == PompAction::HURT_FLY) { m_vy += 1.5f * ts; m_y += m_vy; m_x += m_vx * ts; if (CheckCollision((int)m_x + (int)(m_colW/2), (int)m_y + (int)m_colH)) { m_ActionState = PompAction::HURT_GROUND; m_State = EnemyState::DEAD; m_vx = 0; m_vy = 0; m_CurrentFrame = 0; } }
        DWORD ct = GetTickCount(); if (ct - m_LastTime >= (DWORD)(100 / ts)) { m_CurrentFrame++; m_LastTime = ct; }
        return;
    }
    Enemy::Update(ts);
    DWORD cpt = GetTickCount();
    if (m_isWaiting) { m_vx = 0.0f; m_State = EnemyState::IDLE; if (cpt - m_patternTimer >= (DWORD)(1800 / ts)) { m_isWaiting = false; m_patternTimer = cpt; m_walkDistance = 0.0f; m_isFacingLeft = !m_isFacingLeft; m_vx = m_isFacingLeft ? -2.2f : 2.2f; } }
    else { m_State = EnemyState::WALK; int nx = (int)m_x + (int)(m_colW/2) + (int)(m_vx * ts); if (!CheckCollision(nx, (int)m_y + (int)(m_colH * 0.9f))) { m_x += m_vx * ts; m_walkDistance += fabs(m_vx * ts); m_isFacingLeft = (m_vx < 0.0f); if (m_walkDistance >= 100.0f) { m_isWaiting = true; m_patternTimer = cpt; } } else { m_isWaiting = true; m_patternTimer = cpt; } }
    if (m_vy > 0.1f) m_State = EnemyState::FALL;
    DWORD ct = GetTickCount(); if (ct - m_LastTime >= (DWORD)(100 / ts)) { m_CurrentFrame++; m_LastTime = ct; }
}
void Pomp::Render(HDC h, float cx, float cy, float ms, bool dr) {
    if (!h) return;
    int sx = (int)((m_x - cx) * ms), sy = (int)((m_y - cy) * ms);
    CImage* img = nullptr;
    int safeF = (std::max)(0, m_CurrentFrame);
    if (m_isFacingLeft) { if (m_ActionState == PompAction::HURT_FLY) img = &m_ImgHurtFly_L[safeF % 2]; else if (m_ActionState == PompAction::HURT_GROUND || m_State == EnemyState::DEAD) img = &m_ImgHurtGround_L[safeF % 15]; else if (m_State == EnemyState::FALL) img = &m_ImgFall_L[safeF % 13]; else { switch (m_ActionState) { case PompAction::NONE: if (m_State == EnemyState::IDLE) img = &m_ImgIdle_L[safeF % 8]; else img = &m_ImgWalk_L[safeF % 10]; break; case PompAction::ATTACK: img = &m_ImgAttack_L[safeF % 6]; break; case PompAction::BOX_IDLE: img = &m_ImgBoxIdle_L[safeF % 10]; break; case PompAction::BOX_HIT: img = &m_ImgBoxHit_L[safeF % 14]; break; case PompAction::TURN: img = &m_ImgTurn_L[safeF % 6]; break; case PompAction::RUN: img = &m_ImgRun_L[safeF % 10]; break; } } }
    else { if (m_ActionState == PompAction::HURT_FLY) img = &m_ImgHurtFly_R[safeF % 2]; else if (m_ActionState == PompAction::HURT_GROUND || m_State == EnemyState::DEAD) img = &m_ImgHurtGround_R[safeF % 15]; else if (m_State == EnemyState::FALL) img = &m_ImgFall_R[safeF % 13]; else { switch (m_ActionState) { case PompAction::NONE: if (m_State == EnemyState::IDLE) img = &m_ImgIdle_R[safeF % 8]; else img = &m_ImgWalk_R[safeF % 10]; break; case PompAction::ATTACK: img = &m_ImgAttack_R[safeF % 6]; break; case PompAction::BOX_IDLE: img = &m_ImgBoxIdle_R[safeF % 10]; break; case PompAction::BOX_HIT: img = &m_ImgBoxHit_R[safeF % 14]; break; case PompAction::TURN: img = &m_ImgTurn_R[safeF % 6]; break; case PompAction::RUN: img = &m_ImgRun_R[safeF % 10]; break; } } }
    if (img && !img->IsNull()) { int fw = (int)(40 * ms), fh = (int)(60 * ms); if (m_isFacingLeft) { int om = SetGraphicsMode(h, GM_ADVANCED); XFORM xfO; GetWorldTransform(h, &xfO); XFORM xfL; xfL.eM11 = -1.0f; xfL.eM12 = 0.0f; xfL.eM21 = 0.0f; xfL.eM22 = 1.0f; xfL.eDx = (float)(2 * sx + fw); xfL.eDy = 0.0f; SetWorldTransform(h, &xfL); img->Draw(h, sx, sy, fw, fh); SetWorldTransform(h, &xfO); SetGraphicsMode(h, om); } else img->Draw(h, sx, sy, fw, fh); }
    else Rectangle(h, sx, sy, sx + (int)(m_colW * ms), sy + (int)(m_colH * ms));
    if (dr) { HBRUSH rb = CreateSolidBrush(RGB(255, 0, 0)); RECT r = { sx, sy, sx + (int)(m_colW * ms), sy + (int)(m_colH * ms) }; FrameRect(h, &r, rb); DeleteObject(rb); }
}

void Pomp::OnTakeDamage(float damage) { if (m_isImmortal) return; m_isAlive = false; m_ActionState = PompAction::HURT_FLY; m_vy = -15.0f; m_vx = m_isFacingLeft ? 8.0f : -8.0f; m_CurrentFrame = 0; }

// ShieldCop
ShieldCop::ShieldCop(float x, float y) : Enemy(x, y, EnemyType::SHIELDCOP) { m_ActionState = ShieldCopAction::NONE; }
ShieldCop::~ShieldCop() {}
void ShieldCop::Reset() { Enemy::Reset(); m_ActionState = ShieldCopAction::NONE; }
void ShieldCop::Init() {
    TCHAR path[256];
    for (int i = 0; i < 6; i++) { wsprintf(path, TEXT("assets/enemy/spr_shieldcop_idle/%d.png"), i); m_ImgIdle_R[i].Load(path); m_ImgIdle_L[i].Load(path); }
    for (int i = 0; i < 10; i++) { wsprintf(path, TEXT("assets/enemy/spr_shieldcop_walk/%d.png"), i); m_ImgWalk_R[i].Load(path); m_ImgWalk_L[i].Load(path); }
    for (int i = 0; i < 10; i++) { wsprintf(path, TEXT("assets/enemy/spr_shieldcop_run/%d.png"), i); m_ImgRun_R[i].Load(path); m_ImgRun_L[i].Load(path); }
    for (int i = 0; i < 8; i++) { wsprintf(path, TEXT("assets/enemy/spr_shieldcop_turn/%d.png"), i); m_ImgTurn_R[i].Load(path); m_ImgTurn_L[i].Load(path); }
    for (int i = 0; i < 19; i++) { wsprintf(path, TEXT("assets/enemy/spr_shieldcop_aim/%d.png"), i); m_ImgAim_R[i].Load(path); m_ImgAim_L[i].Load(path); }
    for (int i = 0; i < 6; i++) { wsprintf(path, TEXT("assets/enemy/spr_shieldcop_bash/%d.png"), i); m_ImgBash_R[i].Load(path); m_ImgBash_L[i].Load(path); }
    for (int i = 0; i < 2; i++) { wsprintf(path, TEXT("assets/enemy/spr_shieldcop_knockback/%d.png"), i); m_ImgKnockback_R[i].Load(path); m_ImgKnockback_L[i].Load(path); }
    for (int i = 0; i < 15; i++) { wsprintf(path, TEXT("assets/enemy/spr_shieldcop_tragedy_die_1/%d.png"), i); m_ImgTragedyDie_R[i].Load(path); m_ImgTragedyDie_L[i].Load(path); }
}
void ShieldCop::Update(float ts) {
    if (m_ActionState == ShieldCopAction::HURT_FLY || m_ActionState == ShieldCopAction::HURT_GROUND || m_State == EnemyState::DEAD) {
        if (m_ActionState == ShieldCopAction::HURT_FLY) { m_vy += 1.5f * ts; m_y += m_vy; m_x += m_vx * ts; if (CheckCollision((int)m_x + (int)(m_colW/2), (int)m_y + (int)m_colH)) { m_ActionState = ShieldCopAction::HURT_GROUND; m_State = EnemyState::DEAD; m_vx = 0; m_vy = 0; m_CurrentFrame = 0; } }
        DWORD ct = GetTickCount(); if (ct - m_LastTime >= (DWORD)(100 / ts)) { m_CurrentFrame++; m_LastTime = ct; }
        return;
    }
    Enemy::Update(ts);
    DWORD cpt = GetTickCount();
    if (m_isWaiting) { m_vx = 0.0f; m_State = EnemyState::IDLE; if (cpt - m_patternTimer >= (DWORD)(2200 / ts)) { m_isWaiting = false; m_patternTimer = cpt; m_walkDistance = 0.0f; m_isFacingLeft = !m_isFacingLeft; m_vx = m_isFacingLeft ? -1.8f : 1.8f; } }
    else { m_State = EnemyState::WALK; int nx = (int)m_x + (int)(m_colW/2) + (int)(m_vx * ts); if (!CheckCollision(nx, (int)m_y + (int)(m_colH * 0.9f))) { m_x += m_vx * ts; m_walkDistance += fabs(m_vx * ts); m_isFacingLeft = (m_vx < 0.0f); if (m_walkDistance >= 140.0f) { m_isWaiting = true; m_patternTimer = cpt; } } else { m_isWaiting = true; m_patternTimer = cpt; } }
    if (m_vy > 0.1f) m_State = EnemyState::FALL;
    DWORD ct = GetTickCount(); if (ct - m_LastTime >= (DWORD)(100 / ts)) { m_CurrentFrame++; m_LastTime = ct; }
}
void ShieldCop::Render(HDC h, float cx, float cy, float ms, bool dr) {
    if (!h) return;
    int sx = (int)((m_x - cx) * ms), sy = (int)((m_y - cy) * ms);
    CImage* img = nullptr;
    int safeF = (std::max)(0, m_CurrentFrame);
    if (m_isFacingLeft) { if (m_ActionState == ShieldCopAction::HURT_FLY) img = &m_ImgKnockback_L[safeF % 2]; else if (m_ActionState == ShieldCopAction::HURT_GROUND || m_State == EnemyState::DEAD) img = &m_ImgTragedyDie_L[safeF % 15]; else if (m_State == EnemyState::FALL) img = &m_ImgIdle_L[safeF % 6]; else { switch (m_ActionState) { case ShieldCopAction::NONE: if (m_State == EnemyState::IDLE) img = &m_ImgIdle_L[safeF % 6]; else img = &m_ImgWalk_L[safeF % 10]; break; case ShieldCopAction::AIM: img = &m_ImgAim_L[safeF % 19]; break; case ShieldCopAction::BASH: img = &m_ImgBash_L[safeF % 6]; break; case ShieldCopAction::TURN: img = &m_ImgTurn_L[safeF % 8]; break; case ShieldCopAction::RUN: img = &m_ImgRun_L[safeF % 10]; break; } } }
    else { if (m_ActionState == ShieldCopAction::HURT_FLY) img = &m_ImgKnockback_R[safeF % 2]; else if (m_ActionState == ShieldCopAction::HURT_GROUND || m_State == EnemyState::DEAD) img = &m_ImgTragedyDie_R[safeF % 15]; else if (m_State == EnemyState::FALL) img = &m_ImgIdle_R[safeF % 6]; else { switch (m_ActionState) { case ShieldCopAction::NONE: if (m_State == EnemyState::IDLE) img = &m_ImgIdle_R[safeF % 6]; else img = &m_ImgWalk_R[safeF % 10]; break; case ShieldCopAction::AIM: img = &m_ImgAim_R[safeF % 19]; break; case ShieldCopAction::BASH: img = &m_ImgBash_R[safeF % 6]; break; case ShieldCopAction::TURN: img = &m_ImgTurn_R[safeF % 8]; break; case ShieldCopAction::RUN: img = &m_ImgRun_R[safeF % 10]; break; } } }
    if (img && !img->IsNull()) { int fw = (int)(40 * ms), fh = (int)(60 * ms); if (m_isFacingLeft) { int om = SetGraphicsMode(h, GM_ADVANCED); XFORM xfO; GetWorldTransform(h, &xfO); XFORM xfL; xfL.eM11 = -1.0f; xfL.eM12 = 0.0f; xfL.eM21 = 0.0f; xfL.eM22 = 1.0f; xfL.eDx = (float)(2 * sx + fw); xfL.eDy = 0.0f; SetWorldTransform(h, &xfL); img->Draw(h, sx, sy, fw, fh); SetWorldTransform(h, &xfO); SetGraphicsMode(h, om); } else img->Draw(h, sx, sy, fw, fh); }
    else Rectangle(h, sx, sy, sx + (int)(m_colW * ms), sy + (int)(m_colH * ms));
    if (dr) { HBRUSH rb = CreateSolidBrush(RGB(255, 0, 0)); RECT r = { sx, sy, sx + (int)(m_colW * ms), sy + (int)(m_colH * ms) }; FrameRect(h, &r, rb); DeleteObject(rb); }
}

void ShieldCop::OnTakeDamage(float damage) { if (m_isImmortal) return; m_isAlive = false; m_ActionState = ShieldCopAction::HURT_FLY; m_vy = -15.0f; m_vx = m_isFacingLeft ? 8.0f : -8.0f; m_CurrentFrame = 0; }

