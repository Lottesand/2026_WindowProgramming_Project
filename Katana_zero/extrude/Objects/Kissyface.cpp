#include "Kissyface.h"
#include "Player.h"
#include "Physics.h"
#include "../SceneAndMap/Camera.h"
#include <gdiplus.h>

CImage Kissyface::m_imgIdle;

Kissyface::Kissyface(float startX, float startY) 
    : Enemy(startX, startY, EnemyType::KISSYFACE) {
    m_colW = 40.0f;
    m_colH = 50.0f;
    m_ActionState = KissyfaceAction::KF_IDLE;
    m_vx = 0.0f;
    m_vy = 0.0f;
}

void Kissyface::Init() {
    if (m_imgIdle.IsNull()) m_imgIdle.Load(TEXT("assets/boss/spr_kissyface_idle.png"));
}

void Kissyface::Update(float ts, const Player& player) {
    if (!m_isAlive) return;

    // Apply gravity
    m_vy += 1.5f * ts; 
    if (m_vy > 30.0f) m_vy = 30.0f;

    float ny = m_y + m_vy * ts;
    bool floorHit = false;

    if (m_vy > 0.0f) {
        for (float sy = m_y; sy <= ny; sy += 1.0f) {
            // Physics::GetFloorY 대신 GetCollisionType 직접 사용
            int tc = GetCollisionType((int)(m_x + m_colW / 2.0f), (int)(sy + m_colH));
            if (tc == 1 || tc == 3) {
                m_y = sy;
                m_vy = 0;
                floorHit = true;
                break;
            }
        }
    }

    if (!floorHit) m_y = ny;

    // Face player
    m_isFacingLeft = (player.GetX() < m_x);
}

void Kissyface::Render(HDC hdc, Gdiplus::Graphics* g, float camX, float camY, float mapScale, bool showDebugRect, bool isSlowMo) {
    if (!m_isAlive) return;

    float pFS = mapScale;
    float vx = (m_x - camX) * pFS;
    float vy = (m_y - camY) * pFS;

    if (!m_imgIdle.IsNull()) {
        int imgW = m_imgIdle.GetWidth();
        int imgH = m_imgIdle.GetHeight();
        
        float drawScale = 2.0f * pFS;
        int fw = (int)(imgW * drawScale);
        int fh = (int)(imgH * drawScale);
        
        int dx = (int)(vx + (m_colW * pFS) / 2.0f - fw / 2.0f);
        int dy = (int)(vy + (m_colH * pFS) - fh);

        if (m_isFacingLeft) {
            Gdiplus::Matrix xo; g->GetTransform(&xo);
            Gdiplus::Matrix xl; 
            xl.Scale(-1.0f, 1.0f);
            xl.Translate(-(float)(dx * 2 + fw), 0, Gdiplus::MatrixOrderAppend);
            g->SetTransform(&xl);
            m_imgIdle.Draw(hdc, dx, dy, fw, fh);
            g->SetTransform(&xo);
        } else {
            m_imgIdle.Draw(hdc, dx, dy, fw, fh);
        }
    }

    if (showDebugRect) RenderDebug(hdc, camX, camY, mapScale);
}

void Kissyface::ReleaseAll() {
    m_imgIdle.Destroy();
}
