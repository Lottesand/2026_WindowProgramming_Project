#include "Bullet.h"
#include "Physics.h"
#include "Player.h"
#include <cmath>
#include <algorithm>

CImage Bullet::m_imgBullet;
std::vector<Bullet*> Bullet::m_bullets;

Bullet::Bullet(float x, float y, float vx, float vy) 
    : m_x(x), m_y(y), m_vx(vx), m_vy(vy), m_width(12.0f), m_height(8.0f), m_isActive(true) {
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

void Bullet::UpdateAll(float ts, Player& player) {
    for (auto it = m_bullets.begin(); it != m_bullets.end(); ) {
        (*it)->Update(ts, player);
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

void Bullet::Update(float ts, Player& player) {
    if (!m_isActive) return;

    m_x += m_vx * ts;
    m_y += m_vy * ts;

    // Player collision check
    RECT pR = { (int)player.GetX(), (int)player.GetY(), (int)(player.GetX() + player.GetColW()), (int)(player.GetY() + player.GetColH()) };
    RECT bR = GetRect();
    RECT ol;
    if (IntersectRect(&ol, &pR, &bR)) {
        if (player.GetState() == PlayerState::PS_ROLL) {
            // Bullet passes through during roll
            return;
        }
        if (!player.IsGodMode() && player.GetState() != PlayerState::PS_DEAD) {
            float kvx = (m_vx > 0) ? 8.0f : -8.0f;
            float kvy = -6.0f;
            player.OnTakeDamage(1.0f, kvx, kvy);
        }
        m_isActive = false;
        return;
    }

    // Map collision check
    if (CheckCollision((int)(m_x + m_width / 2), (int)(m_y + m_height / 2))) {
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

    // Use original image dimensions
    int imgW = m_imgBullet.GetWidth();
    int imgH = m_imgBullet.GetHeight();

    int sw = (int)(imgW * 1.0f * mapScale); 
    int sh = (int)(imgH * 1.0f * mapScale);
    int sx = (int)((m_x - camX) * mapScale);
    int sy = (int)((m_y - camY) * mapScale) - sh / 2;

    if (m_vx < 0) {
        int om = SetGraphicsMode(hdc, GM_ADVANCED);
        XFORM xo; GetWorldTransform(hdc, &xo);
        XFORM xl = { -1.0f, 0.0f, 0.0f, 1.0f, (float)(2 * sx + sw), 0.0f };
        SetWorldTransform(hdc, &xl);
        m_imgBullet.TransparentBlt(hdc, sx, sy, sw, sh, 0, 0, imgW, imgH, RGB(0, 0, 0));
        SetWorldTransform(hdc, &xo);
        SetGraphicsMode(hdc, om);
    } else {
        m_imgBullet.TransparentBlt(hdc, sx, sy, sw, sh, 0, 0, imgW, imgH, RGB(0, 0, 0));
    }
}
