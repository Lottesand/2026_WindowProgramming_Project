#include "Kissyface.h"
#include "Player.h"
#include "Physics.h"
#include "../SceneAndMap/Camera.h"
#include "../UI/UIManager.h"
#include "../Core/SoundManager.h"
#include <gdiplus.h>
#include <cstdio>
#include <vector>
#include <cmath>
#include <map>

std::vector<CImage> Kissyface::m_imgIdle;
std::vector<CImage> Kissyface::m_imgWalk;
std::vector<CImage> Kissyface::m_imgSlash;
std::vector<CImage> Kissyface::m_imgBlock;
std::vector<CImage> Kissyface::m_imgThrow;
std::vector<CImage> Kissyface::m_imgTug;
std::vector<CImage> Kissyface::m_imgReturnAxe;
std::vector<CImage> Kissyface::m_imgPreJump;
std::vector<CImage> Kissyface::m_imgJump;
std::vector<CImage> Kissyface::m_imgLand;
std::vector<CImage> Kissyface::m_imgHurtFly;
std::vector<CImage> Kissyface::m_imgHurtGround;
std::vector<CImage> Kissyface::m_imgStruggle;
std::vector<CImage> Kissyface::m_imgRecover;
std::vector<CImage> Kissyface::m_imgPreLunge;
std::vector<CImage> Kissyface::m_imgLunge;
std::vector<CImage> Kissyface::m_imgLungeAttack;
std::vector<CImage> Kissyface::m_imgDie;
std::vector<CImage> Kissyface::m_imgDead;
std::vector<CImage> Kissyface::m_imgNoHead;
CImage Kissyface::m_imgAxe;

// Helper to destroy vector of images
void destroyVec(std::vector<CImage>& vec) {
    for (auto& img : vec) img.Destroy();
    vec.clear();
}

Kissyface::Kissyface(float x, float y) : Enemy(x, y, EnemyType::KISSYFACE, 0.0f) {
    m_hp = 100.0f;
    m_ActionState = KissyfaceAction::KF_IDLE;
    m_animFrame = 0;
    m_animTimer = 0.0f;
    m_patternDelayTimer = 0.0f;
    m_strugglePhase = 0;
    m_axe.state = AxeState::INACTIVE;
    m_colW = 60.0f;
    m_colH = 80.0f;
}

Kissyface::~Kissyface() {
}

RECT Kissyface::GetVulnerableRect() const {
    float pc = m_isFacingLeft ? 0.0f : 0.5f;
    return { (int)(m_x + m_colW * pc), (int)m_y, (int)(m_x + m_colW * (pc + 0.5f)), (int)(m_y + m_colH) };
}

RECT Kissyface::GetInvincibleRect() const {
    float pc = m_isFacingLeft ? 0.5f : 0.0f;
    return { (int)(m_x + m_colW * pc), (int)m_y, (int)(m_x + m_colW * (pc + 0.5f)), (int)(m_y + m_colH) };
}

void Kissyface::Init() {
    if (!m_imgIdle.empty()) return;
    auto loadVec = [](std::vector<CImage>& vec, const wchar_t* format, int count) {
        vec.resize(count);
        for (int i = 0; i < count; i++) {
            wchar_t path[256];
            swprintf_s(path, format, i);
            vec[i].Load(path);
        }
    };
    loadVec(m_imgIdle, L"assets/boss/spr_kissyface_idle/%d.png", 10);
    loadVec(m_imgWalk, L"assets/boss/spr_kissyface_walk/%d.png", 10);
    loadVec(m_imgJump, L"assets/boss/spr_kissyface_jump/%d.png", 1);
    loadVec(m_imgSlash, L"assets/boss/spr_kissyface_slash/%d.png", 7);
    loadVec(m_imgBlock, L"assets/boss/spr_kissyface_block/%d.png", 5);
    loadVec(m_imgHurtFly, L"assets/boss/spr_kissyface_hurt/%d.png", 1);
    loadVec(m_imgHurtGround, L"assets/boss/spr_kissyface_recover/%d.png", 1);
    loadVec(m_imgStruggle, L"assets/boss/spr_kissyface_struggle/%d.png", 12);
    loadVec(m_imgRecover, L"assets/boss/spr_kissyface_recover/%d.png", 8);
    loadVec(m_imgPreLunge, L"assets/boss/spr_kissyface_prelunge/%d.png", 4);
    loadVec(m_imgLunge, L"assets/boss/spr_kissyface_lunge/%d.png", 1);
    loadVec(m_imgLungeAttack, L"assets/boss/spr_kissyface_lungeattack/%d.png", 6);
    loadVec(m_imgDie, L"assets/boss/spr_kissyface_die/%d.png", 10);
    loadVec(m_imgDead, L"assets/boss/spr_kissyface_dead/%d.png", 1);
    loadVec(m_imgNoHead, L"assets/boss/spr_kissyface_nohead/%d.png", 1);
    if (m_imgAxe.IsNull()) m_imgAxe.Load(L"assets/boss/spr_kissyface_axe.png");
}

void Kissyface::Reset() {
    Enemy::Reset();
    m_hp = 100.0f;
    m_ActionState = KissyfaceAction::KF_IDLE;
    m_animFrame = 0;
    m_animTimer = 0.0f;
    m_strugglePhase = 0;
    m_axe.state = AxeState::INACTIVE;
}

void Kissyface::Update(float ts, const Player& player) {
    Enemy::Update(ts, player);
    m_animTimer += ts;
    if (!m_isAlive) {
        if (m_ActionState == KissyfaceAction::KF_DIE) {
            if (m_animTimer >= 0.1f) { m_animFrame++; m_animTimer = 0; if (m_animFrame >= 10) { m_ActionState = KissyfaceAction::KF_DEAD; m_animFrame = 0; } }
        }
        return;
    }
    float dx = player.GetX() - m_x;
    m_isFacingLeft = (dx < 0);
    if (m_ActionState == KissyfaceAction::KF_IDLE) {
        if (abs(dx) > 200.0f) { m_ActionState = KissyfaceAction::KF_WALK; m_animFrame = 0; }
        else { m_patternDelayTimer += ts; if (m_patternDelayTimer >= 1.0f) { m_ActionState = KissyfaceAction::KF_ATTACK; m_animFrame = 0; m_patternDelayTimer = 0; } }
    } else if (m_ActionState == KissyfaceAction::KF_WALK) {
        m_vx = m_isFacingLeft ? -5.0f : 5.0f;
        if (abs(dx) < 150.0f) { m_ActionState = KissyfaceAction::KF_IDLE; m_vx = 0; }
    } else if (m_ActionState == KissyfaceAction::KF_ATTACK) {
        m_vx = 0;
        if (m_animTimer >= 0.08f) { m_animFrame++; m_animTimer = 0; if (m_animFrame >= 7) { m_ActionState = KissyfaceAction::KF_IDLE; m_animFrame = 0; } }
    }
    UpdateAxe(ts, player);
}

void Kissyface::UpdateAxe(float ts, const Player& player) {}

void Kissyface::Render(HDC hdc, Gdiplus::Graphics* g, float camX, float camY, float mapScale, bool showDebugRect, bool isSlowMo) {
    std::vector<CImage>* currentVec = &m_imgIdle;
    switch (m_ActionState) {
    case KissyfaceAction::KF_IDLE: currentVec = &m_imgIdle; break;
    case KissyfaceAction::KF_WALK: currentVec = &m_imgWalk; break;
    case KissyfaceAction::KF_ATTACK: currentVec = &m_imgSlash; break;
    case KissyfaceAction::KF_BLOCK: currentVec = &m_imgBlock; break;
    case KissyfaceAction::KF_HURT_FLY: currentVec = &m_imgHurtFly; break;
    case KissyfaceAction::KF_DIE: currentVec = &m_imgDie; break;
    case KissyfaceAction::KF_DEAD: currentVec = &m_imgDead; break;
    case KissyfaceAction::KF_NOHEAD: currentVec = &m_imgNoHead; break;
    }
    if (currentVec && !currentVec->empty()) {
        int frame = m_animFrame % currentVec->size();
        CImage& img = (*currentVec)[frame];
        if (!img.IsNull()) {
            int fw = (int)(img.GetWidth() * 2.0f * mapScale);
            int fh = (int)(img.GetHeight() * 2.0f * mapScale);
            int dx = (int)((m_x - camX) * mapScale) + (int)(m_colW * mapScale / 2) - fw / 2, dy = (int)((m_y - camY) * mapScale) + (int)(m_colH * mapScale) - fh;
            if (m_isFacingLeft) {
                int om = SetGraphicsMode(hdc, GM_ADVANCED); XFORM xo; GetWorldTransform(hdc, &xo);
                XFORM flip = { -1.0f, 0.0f, 0.0f, 1.0f, (float)(2 * dx + fw), 0.0f };
                SetWorldTransform(hdc, &flip); img.Draw(hdc, dx, dy, fw, fh); SetWorldTransform(hdc, &xo); SetGraphicsMode(hdc, om);
            } else img.Draw(hdc, dx, dy, fw, fh);
        }
    }
    RenderBurningEffect(hdc, mapScale, camX, camY);
}

bool Kissyface::OnTakeDamage(float kvx, float kvy, DeathCause cause) {
    if (!m_isAlive && m_ActionState != KissyfaceAction::KF_DEAD) return false;
    if (m_ActionState == KissyfaceAction::KF_HURT_FLY || m_ActionState == KissyfaceAction::KF_DIE || m_ActionState == KissyfaceAction::KF_NOHEAD) return false;
    if (m_ActionState == KissyfaceAction::KF_DEAD) { m_ActionState = KissyfaceAction::KF_NOHEAD; m_animFrame = 0; return false; }
    m_hp -= 20.0f;
    if (m_hp <= 0) { m_hp = 0; m_isAlive = false; m_ActionState = KissyfaceAction::KF_DIE; m_animFrame = 0; }
    else m_ActionState = KissyfaceAction::KF_HURT_FLY;
    m_vx = kvx; m_vy = kvy;
    Enemy::OnTakeDamage(kvx, kvy, cause);
    return false;
}

void Kissyface::Parry() { m_ActionState = KissyfaceAction::KF_BLOCK; m_animFrame = 0; m_animTimer = 0; }

void Kissyface::ReleaseAll() {
    destroyVec(m_imgIdle); destroyVec(m_imgWalk); destroyVec(m_imgSlash); destroyVec(m_imgBlock);
    destroyVec(m_imgThrow); destroyVec(m_imgTug); destroyVec(m_imgReturnAxe); destroyVec(m_imgPreJump);
    destroyVec(m_imgJump); destroyVec(m_imgLand); destroyVec(m_imgHurtFly); destroyVec(m_imgHurtGround);
    destroyVec(m_imgStruggle); destroyVec(m_imgRecover); destroyVec(m_imgPreLunge); destroyVec(m_imgLunge);
    destroyVec(m_imgLungeAttack); destroyVec(m_imgDie); destroyVec(m_imgDead); destroyVec(m_imgNoHead);
    m_imgAxe.Destroy();
}
