#include "Enemy.h"
#include "Physics.h"
#include "../SceneAndMap/Camera.h"
#include <gdiplus.h>

Enemy::Enemy(float startX, float startY, EnemyType type) {
    m_startX = startX; m_startY = startY; m_x = startX; m_y = startY;
    m_vx = 2.0f; m_vy = 0.0f; m_colW = 40.0f; m_colH = 60.0f;
    m_isAlive = true; m_isFacingLeft = false; m_Type = type; m_State = EnemyState::IDLE;
    m_CurrentFrame = 0; m_LastTime = GetTickCount(); m_friction = 0.92f; m_knockbackVx = 0.0f;
    m_isImmortal = false; m_patternTimer = GetTickCount(); m_isWaiting = false; m_walkDistance = 0.0f;
}
Enemy::~Enemy() {}

void Enemy::ReleaseAll() {
    Gangster::Release();
    Grunt::Release();
    Pomp::Release();
    ShieldCop::Release();
}

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

// Helper for grayscale rendering
void RenderImageGrayscale(HDC hdc, CImage* img, int sx, int sy, int sw, int sh) {
    if (!img || img->IsNull()) return;
    Gdiplus::Graphics g(hdc);
    void* bits = img->GetBits();
    if (bits) {
        Gdiplus::Bitmap gb(img->GetWidth(), img->GetHeight(), img->GetPitch(), PixelFormat32bppARGB, (BYTE*)bits);
        Gdiplus::ImageAttributes at; 
        Gdiplus::ColorMatrix mat = { 
            0.3f, 0.3f, 0.3f, 0, 0, 
            0.59f, 0.59f, 0.59f, 0, 0, 
            0.11f, 0.11f, 0.11f, 0, 0, 
            0, 0, 0, 1, 0, 
            0, 0, 0, 0, 1 
        };
        at.SetColorMatrix(&mat, Gdiplus::ColorMatrixFlagsDefault, Gdiplus::ColorAdjustTypeBitmap);
        g.DrawImage(&gb, Gdiplus::RectF((float)sx, (float)sy, (float)sw, (float)sh), 0, 0, (float)img->GetWidth(), (float)img->GetHeight(), Gdiplus::UnitPixel, &at);
    }
}

// Gangster
CImage Gangster::m_ImgIdle_R[8], Gangster::m_ImgIdle_L[8], Gangster::m_ImgWalk_R[8], Gangster::m_ImgWalk_L[8], Gangster::m_ImgAim_R[7], Gangster::m_ImgAim_L[7], Gangster::m_ImgFire_R[6], Gangster::m_ImgFire_L[6], Gangster::m_ImgTurn_R[6], Gangster::m_ImgTurn_L[6], Gangster::m_ImgFall_R[12], Gangster::m_ImgFall_L[12], Gangster::m_ImgHurtFly_R[2], Gangster::m_ImgHurtFly_L[2], Gangster::m_ImgHurtGround_R[14], Gangster::m_ImgHurtGround_L[14], Gangster::m_ImgRun_R[10], Gangster::m_ImgRun_L[10];
Gangster::Gangster(float x, float y) : Enemy(x, y, EnemyType::GANGSTER) { m_ActionState = GangsterAction::NONE; }
Gangster::~Gangster() {}
void Gangster::Reset() { Enemy::Reset(); m_ActionState = GangsterAction::NONE; }
void Gangster::Init() {
    if (!m_ImgIdle_R[0].IsNull()) return;
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
void Gangster::Release() {
    for (int i = 0; i < 8; i++) { m_ImgIdle_R[i].Destroy(); m_ImgIdle_L[i].Destroy(); }
    for (int i = 0; i < 8; i++) { m_ImgWalk_R[i].Destroy(); m_ImgWalk_L[i].Destroy(); }
    for (int i = 0; i < 7; i++) { m_ImgAim_R[i].Destroy(); m_ImgAim_L[i].Destroy(); }
    for (int i = 0; i < 6; i++) { m_ImgFire_R[i].Destroy(); m_ImgFire_L[i].Destroy(); }
    for (int i = 0; i < 6; i++) { m_ImgTurn_R[i].Destroy(); m_ImgTurn_L[i].Destroy(); }
    for (int i = 0; i < 12; i++) { m_ImgFall_R[i].Destroy(); m_ImgFall_L[i].Destroy(); }
    for (int i = 0; i < 2; i++) { m_ImgHurtFly_R[i].Destroy(); m_ImgHurtFly_L[i].Destroy(); }
    for (int i = 0; i < 14; i++) { m_ImgHurtGround_R[i].Destroy(); m_ImgHurtGround_L[i].Destroy(); }
    for (int i = 0; i < 10; i++) { m_ImgRun_R[i].Destroy(); m_ImgRun_L[i].Destroy(); }
}
void Gangster::Update(float ts) {
    Enemy::Update(ts);
    if (GetTickCount() - m_LastTime > 100) { m_CurrentFrame = (m_CurrentFrame + 1) % 8; m_LastTime = GetTickCount(); }
}
void Gangster::Render(HDC hdc, float camX, float camY, float mapScale, bool showDebugRect) {
    if (!m_isAlive) return;
    int sx = (int)((m_x - camX) * mapScale), sy = (int)((m_y - camY) * mapScale), sw = (int)(m_colW * mapScale), sh = (int)(m_colH * mapScale);
    CImage* img = m_isFacingLeft ? &m_ImgIdle_L[m_CurrentFrame % 8] : &m_ImgIdle_R[m_CurrentFrame % 8];
    if (img && !img->IsNull()) img->Draw(hdc, sx, sy, sw, sh);
    if (showDebugRect) { HBRUSH hb = CreateSolidBrush(RGB(255, 0, 0)); RECT r = { sx, sy, sx + sw, sy + sh }; FrameRect(hdc, &r, hb); DeleteObject(hb); }
}
void Gangster::OnTakeDamage(float d) { Enemy::OnTakeDamage(d); }

// Grunt
CImage Grunt::m_ImgIdle_R[8], Grunt::m_ImgIdle_L[8], Grunt::m_ImgWalk_R[10], Grunt::m_ImgWalk_L[10], Grunt::m_ImgAttack_R[8], Grunt::m_ImgAttack_L[8], Grunt::m_ImgSlash_R[5], Grunt::m_ImgSlash_L[5], Grunt::m_ImgTurn_R[8], Grunt::m_ImgTurn_L[8], Grunt::m_ImgFall_R[13], Grunt::m_ImgFall_L[13], Grunt::m_ImgHurtFly_R[2], Grunt::m_ImgHurtFly_L[2], Grunt::m_ImgHurtGround_R[16], Grunt::m_ImgHurtGround_L[16], Grunt::m_ImgRun_R[10], Grunt::m_ImgRun_L[10];
Grunt::Grunt(float x, float y) : Enemy(x, y, EnemyType::GRUNT) { m_ActionState = GruntAction::NONE; }
Grunt::~Grunt() {}
void Grunt::Reset() { Enemy::Reset(); m_ActionState = GruntAction::NONE; }
void Grunt::Init() {
    if (!m_ImgIdle_R[0].IsNull()) return;
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
    Enemy::Update(ts);
    if (GetTickCount() - m_LastTime > 100) { m_CurrentFrame = (m_CurrentFrame + 1) % 8; m_LastTime = GetTickCount(); }
}
void Grunt::Render(HDC hdc, float camX, float camY, float mapScale, bool showDebugRect) {
    if (!m_isAlive) return;
    int sx = (int)((m_x - camX) * mapScale), sy = (int)((m_y - camY) * mapScale), sw = (int)(m_colW * mapScale), sh = (int)(m_colH * mapScale);
    CImage* img = m_isFacingLeft ? &m_ImgIdle_L[m_CurrentFrame % 8] : &m_ImgIdle_R[m_CurrentFrame % 8];
    if (img && !img->IsNull()) img->Draw(hdc, sx, sy, sw, sh);
    if (showDebugRect) { HBRUSH hb = CreateSolidBrush(RGB(255, 0, 0)); RECT r = { sx, sy, sx + sw, sy + sh }; FrameRect(hdc, &r, hb); DeleteObject(hb); }
}
void Grunt::OnTakeDamage(float d) { Enemy::OnTakeDamage(d); }

// Pomp
CImage Pomp::m_ImgIdle_R[8], Pomp::m_ImgIdle_L[8], Pomp::m_ImgWalk_R[10], Pomp::m_ImgWalk_L[10], Pomp::m_ImgAttack_R[6], Pomp::m_ImgAttack_L[6], Pomp::m_ImgBoxIdle_R[10], Pomp::m_ImgBoxIdle_L[10], Pomp::m_ImgBoxHit_R[14], Pomp::m_ImgBoxHit_L[14], Pomp::m_ImgTurn_R[6], Pomp::m_ImgTurn_L[6], Pomp::m_ImgFall_R[13], Pomp::m_ImgFall_L[13], Pomp::m_ImgHurtFly_R[2], Pomp::m_ImgHurtFly_L[2], Pomp::m_ImgHurtGround_R[15], Pomp::m_ImgHurtGround_L[15], Pomp::m_ImgRun_R[10], Pomp::m_ImgRun_L[10];
Pomp::Pomp(float x, float y) : Enemy(x, y, EnemyType::POMP) { m_ActionState = PompAction::NONE; }
Pomp::~Pomp() {}
void Pomp::Reset() { Enemy::Reset(); m_ActionState = PompAction::NONE; }
void Pomp::Init() {
    if (!m_ImgIdle_R[0].IsNull()) return;
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
    Enemy::Update(ts);
    if (GetTickCount() - m_LastTime > 100) { m_CurrentFrame = (m_CurrentFrame + 1) % 8; m_LastTime = GetTickCount(); }
}
void Pomp::Render(HDC hdc, float camX, float camY, float mapScale, bool showDebugRect) {
    if (!m_isAlive) return;
    int sx = (int)((m_x - camX) * mapScale), sy = (int)((m_y - camY) * mapScale), sw = (int)(m_colW * mapScale), sh = (int)(m_colH * mapScale);
    CImage* img = m_isFacingLeft ? &m_ImgIdle_L[m_CurrentFrame % 8] : &m_ImgIdle_R[m_CurrentFrame % 8];
    if (img && !img->IsNull()) img->Draw(hdc, sx, sy, sw, sh);
    if (showDebugRect) { HBRUSH hb = CreateSolidBrush(RGB(255, 0, 0)); RECT r = { sx, sy, sx + sw, sy + sh }; FrameRect(hdc, &r, hb); DeleteObject(hb); }
}
void Pomp::OnTakeDamage(float d) { Enemy::OnTakeDamage(d); }

// ShieldCop
CImage ShieldCop::m_ImgIdle_R[6], ShieldCop::m_ImgIdle_L[6], ShieldCop::m_ImgWalk_R[10], ShieldCop::m_ImgWalk_L[10], ShieldCop::m_ImgRun_R[10], ShieldCop::m_ImgRun_L[10], ShieldCop::m_ImgTurn_R[8], ShieldCop::m_ImgTurn_L[8], ShieldCop::m_ImgAim_R[19], ShieldCop::m_ImgAim_L[19], ShieldCop::m_ImgBash_R[6], ShieldCop::m_ImgBash_L[6], ShieldCop::m_ImgKnockback_R[2], ShieldCop::m_ImgKnockback_L[2], ShieldCop::m_ImgTragedyDie_R[15], ShieldCop::m_ImgTragedyDie_L[15];
ShieldCop::ShieldCop(float x, float y) : Enemy(x, y, EnemyType::SHIELDCOP) { m_ActionState = ShieldCopAction::NONE; }
ShieldCop::~ShieldCop() {}
void ShieldCop::Reset() { Enemy::Reset(); m_ActionState = ShieldCopAction::NONE; }
void ShieldCop::Init() {
    if (!m_ImgIdle_R[0].IsNull()) return;
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
    Enemy::Update(ts);
    if (GetTickCount() - m_LastTime > 100) { m_CurrentFrame = (m_CurrentFrame + 1) % 6; m_LastTime = GetTickCount(); }
}
void ShieldCop::Render(HDC hdc, float camX, float camY, float mapScale, bool showDebugRect) {
    if (!m_isAlive) return;
    int sx = (int)((m_x - camX) * mapScale), sy = (int)((m_y - camY) * mapScale), sw = (int)(m_colW * mapScale), sh = (int)(m_colH * mapScale);
    CImage* img = m_isFacingLeft ? &m_ImgIdle_L[m_CurrentFrame % 6] : &m_ImgIdle_R[m_CurrentFrame % 6];
    if (img && !img->IsNull()) img->Draw(hdc, sx, sy, sw, sh);
    if (showDebugRect) { HBRUSH hb = CreateSolidBrush(RGB(255, 0, 0)); RECT r = { sx, sy, sx + sw, sy + sh }; FrameRect(hdc, &r, hb); DeleteObject(hb); }
}
void ShieldCop::OnTakeDamage(float d) { Enemy::OnTakeDamage(d); }
