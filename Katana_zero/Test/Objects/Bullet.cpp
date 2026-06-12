#include "Bullet.h"
#include "Physics.h"
#include "Player.h"
#include "Enemy.h"
#include "../Effects/EffectManager.h"
#include <cmath>
#include <algorithm>

CImage Bullet::m_imgBullet;
std::vector<Bullet*> Bullet::m_bullets;

Bullet::Bullet(float x, float y, float vx, float vy) 
    : m_x(x), m_y(y), m_vx(vx), m_vy(vy), m_width(12.0f), m_height(8.0f), m_isActive(true), m_isDeflected(false) {
}

Bullet::~Bullet() {
}

void Bullet::Init() {
    if (m_imgBullet.IsNull()) {
        m_imgBullet.Load(TEXT("assets/Bullet/0.png"));
    }
}

void Bullet::Release() {
    if (!m_imgBullet.IsNull()) {
        m_imgBullet.Destroy();
    }
    ClearAll();
}

void Bullet::UpdateAll(float ts, Player& player, const std::vector<Enemy*>& enemies) {
    for (auto it = m_bullets.begin(); it != m_bullets.end(); ) {
        (*it)->Update(ts, player, enemies);
        if (!(*it)->IsActive()) {
            delete (*it);
            it = m_bullets.erase(it);
        } else {
            ++it;
        }
    }
}

void Bullet::RenderAll(HDC hdc, float camX, float camY, float mapScale) {
    for (auto b : m_bullets) {
        b->Render(hdc, camX, camY, mapScale);
    }
}

void Bullet::AddBullet(float x, float y, float vx, float vy) {
    m_bullets.push_back(new Bullet(x, y, vx, vy));
}

void Bullet::ClearAll() {
    for (auto b : m_bullets) delete b;
    m_bullets.clear();
}

void Bullet::Deflect(float newVx, float newVy) {
    m_vx = newVx;
    m_vy = newVy;
    m_isDeflected = true;
}

void Bullet::Update(float ts, Player& player, const std::vector<Enemy*>& enemies) {
    if (!m_isActive) return;

    m_x += m_vx * ts;
    m_y += m_vy * ts;

    // Player collision check
    if (!m_isDeflected) {
        RECT pR = { (int)player.GetX(), (int)player.GetY(), (int)(player.GetX() + player.GetColW()), (int)(player.GetY() + player.GetColH()) };
        RECT bR = GetRect();
        RECT ol;
        if (IntersectRect(&ol, &pR, &bR)) {
            if (player.GetState() == PlayerState::PS_ROLL) {
                return;
            }
            if (!player.IsGodMode() && player.GetState() != PlayerState::PS_DEAD) {
                float kvx = (m_vx > 0) ? 8.0f : -8.0f;
                float kvy = -6.0f;
                player.OnTakeDamage(1.0f, kvx, kvy, m_x + m_width / 2.0f, m_y + m_height / 2.0f);
            }
            m_isActive = false;
            return;
        }
    } else {
        // Deflected bullet: check enemy collision
        RECT bR = GetRect();
        for (auto e : enemies) {
            if (e && e->GetIsAlive()) {
                RECT eR = e->GetRect();
                RECT ol;
                if (IntersectRect(&ol, &bR, &eR)) {
                    // Damage enemy
                    float kbx = (m_vx > 0) ? 15.0f : -15.0f;
                    e->OnTakeDamage(kbx, -5.0f);

                    // Visual feedback: Neon trail and Hit VFX
                    float ex = e->GetX() + e->GetColW() / 2.0f;
                    float ey = e->GetY() + e->GetColH() / 2.0f;
                    float distV = sqrt(m_vx * m_vx + m_vy * m_vy);
                    float ux = (distV > 0) ? m_vx / distV : 1.0f;
                    float uy = (distV > 0) ? m_vy / distV : 0.0f;
                    float angle = atan2(uy, ux);
                    
                    EffectManager::AddNeonTrail(ex, ey, ux, uy, angle);
                    EffectManager::AddHitVFX(ex, ey, angle, GetTickCount());

                    m_isActive = false;
                    return;
                }
            }
        }
    }

    // Map collision check
    if (CheckCollision((int)(m_x + m_width / 2), (int)(m_y + m_height / 2)) || CheckDoorCollision(m_x, m_y, m_width, m_height)) {
        m_isActive = false;
    }

    // Screen boundary check
    if (m_x < -2000 || m_x > 8000 || m_y < -2000 || m_y > 8000) {
        m_isActive = false;
    }
}
void Bullet::Render(HDC hdc, float camX, float camY, float mapScale) {
    if (!m_isActive) return;
    if (m_imgBullet.IsNull()) return;

    int imgW = m_imgBullet.GetWidth();
    int imgH = m_imgBullet.GetHeight();

    int sw = (int)(imgW * 1.0f * mapScale); 
    int sh = (int)(imgH * 1.0f * mapScale);
    int sx = (int)((m_x - camX) * mapScale);
    int sy = (int)((m_y - camY) * mapScale);

    int om = SetGraphicsMode(hdc, GM_ADVANCED);
    XFORM xo; GetWorldTransform(hdc, &xo);

    float angle = atan2(m_vy, m_vx); // Radian
    float cosA = cos(angle), sinA = sin(angle);
    float px = (float)(sx + sw / 2.0f), py = (float)(sy); // Center of rotation

    // If the sprite default is facing right, we rotate it by 'angle'
    // If the sprite is moving left, atan2 will give +/- PI, which flips it correctly.
    XFORM rot = { cosA, sinA, -sinA, cosA, px - px * cosA + py * sinA, py - px * sinA - py * cosA };
    
    XFORM combined;
    CombineTransform(&combined, &rot, &xo);
    SetWorldTransform(hdc, &combined);

    m_imgBullet.TransparentBlt(hdc, sx - sw / 2, sy - sh / 2, sw, sh, 0, 0, imgW, imgH, RGB(0, 0, 0));

    SetWorldTransform(hdc, &xo);
    SetGraphicsMode(hdc, om);
}
