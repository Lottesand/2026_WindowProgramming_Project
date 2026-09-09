#include "OilDrum.h"
#include "Enemy.h"
#include "Player.h"
#include "../Effects/EffectManager.h"
#include "../Core/SoundManager.h"
#include <gdiplus.h>
#include <cmath>

CImage OilDrum::m_imgDrum;

OilDrum::OilDrum(float x, float y) : m_x(x), m_y(y), m_isExploded(false), m_isPending(false), m_explodeTime(0) {
    m_width = 32.0f;
    m_height = 45.0f;
}

OilDrum::~OilDrum() {}

void OilDrum::LoadAssets() {
    if (m_imgDrum.IsNull()) {
        m_imgDrum.Load(TEXT("assets/spr_oil_drum.png"));
    }
}

void OilDrum::ReleaseAssets() {
    m_imgDrum.Destroy();
}

void OilDrum::Update(float ts, const std::vector<Enemy*>& enemies, std::vector<OilDrum>& allDrums, Player* player) {
    if (m_isExploded) return;

    if (m_isPending) {
        if (GetTickCount() >= m_explodeTime) {
            Explode(enemies, allDrums, player);
        }
    }
}

void OilDrum::Render(HDC hdc, Gdiplus::Graphics* g, float camX, float camY, float mapScale) {
    if (m_isExploded) return;

    if (!m_imgDrum.IsNull()) {
        int dw = (int)(m_imgDrum.GetWidth() * 2.0f * mapScale);
        int dh = (int)(m_imgDrum.GetHeight() * 2.0f * mapScale);
        int dx = (int)((m_x - camX) * mapScale);
        int dy = (int)((m_y - camY) * mapScale);

        m_imgDrum.Draw(hdc, dx, dy, dw, dh);
    }
}

void OilDrum::Trigger(DWORD delay) {
    if (m_isPending || m_isExploded) return;
    m_isPending = true;
    m_explodeTime = GetTickCount() + delay;
}

void OilDrum::Explode(const std::vector<Enemy*>& enemies, std::vector<OilDrum>& allDrums, Player* player) {
    if (m_isExploded) return;

    m_isExploded = true;
    m_isPending = false;

    float explosionRadius = 180.0f;
    float centerX = m_x + m_width / 2.0f;
    float centerY = m_y + m_height / 2.0f;

    // Trigger VFX and Sound
    EffectManager::AddExplosion(centerX, centerY, GetTickCount(), explosionRadius);
    SoundManager::Play(rand() % 2 == 0 ? "SFX_EXPLOSION_1" : "SFX_EXPLOSION_2");

    // Damage Enemies
    for (auto enemy : enemies) {
        if (enemy && enemy->GetIsAlive()) {
            float ex = enemy->GetX() + enemy->GetColW() / 2.0f;
            float ey = enemy->GetY() + enemy->GetColH() / 2.0f;
            float dx = ex - centerX, dy = ey - centerY;
            if (sqrt(dx * dx + dy * dy) <= explosionRadius) {
                float kbx = (ex > centerX) ? 25.0f : -25.0f;
                enemy->OnTakeDamage(kbx, -12.0f, DeathCause::FIRE);
            }
        }
    }

    // Damage Player (Self-damage)
    if (player && !player->IsDead() && !player->IsGodMode()) {
        float px = player->GetX() + player->GetColW() / 2.0f;
        float py = player->GetY() + player->GetColH() / 2.0f;
        float dx = px - centerX, dy = py - centerY;
        if (sqrt(dx * dx + dy * dy) <= explosionRadius) {
            float kbx = (px > centerX) ? 20.0f : -20.0f;
            player->OnTakeDamage(1.0f, kbx, -10.0f, centerX, centerY, DeathCause::FIRE);
        }
    }

    // Chain Reaction to other Drums with MUCH FASTER DELAY
    for (auto& other : allDrums) {
        if (&other == this || other.m_isExploded || other.m_isPending) continue;

        float ox = other.m_x + other.m_width / 2.0f;
        float oy = other.m_y + other.m_height / 2.0f;
        float dx = ox - centerX, dy = oy - centerY;
        // 반경을 살짝 늘리고(explosionRadius + 40.0f), 지연 시간을 대폭 줄임(30ms ~ 80ms)
        if (sqrt(dx * dx + dy * dy) <= explosionRadius + 40.0f) {
            other.Trigger(30 + rand() % 50);
        }
    }
}
