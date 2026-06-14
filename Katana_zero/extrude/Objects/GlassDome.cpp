#include "GlassDome.h"
#include "../Effects/EffectManager.h"
#include <gdiplus.h>

CImage GlassDome::m_imgDome;

void GlassDome::LoadAssets() {
    if (m_imgDome.IsNull()) {
        m_imgDome.Load(TEXT("assets/stage2/spr_glassdome.png"));
    }
}

void GlassDome::ReleaseAssets() {
    if (!m_imgDome.IsNull()) {
        m_imgDome.Destroy();
    }
}

GlassDome::GlassDome(float x, float y, float w, float h) 
    : m_x(x), m_y(y), m_w(w), m_h(h), m_isBroken(false) {
}

GlassDome::~GlassDome() {}

void GlassDome::Update(float attackHitX, float attackHitY, float attackHitW, float attackHitH, bool isAttacking, DWORD currentTime) {
    if (m_isBroken) return;
    if (isAttacking) {
        bool attackHit = (attackHitX < m_x + m_w && attackHitX + attackHitW > m_x &&
                          attackHitY < m_y + m_h && attackHitY + attackHitH > m_y);
        if (attackHit) {
            Break(currentTime);
        }
    }
}

void GlassDome::Render(HDC hDC, float camX, float camY, float mapScale, bool showDebugRect) {
    if (m_isBroken) return; // 깨지면 렌더링 생략

    int dx = (int)((m_x - camX) * mapScale);
    int dy = (int)((m_y - camY) * mapScale);
    int dw = (int)(m_w * mapScale);
    int dh = (int)(m_h * mapScale);

    if (!m_imgDome.IsNull()) {
        // 이미지가 크다면 중앙 정렬이나 적절한 스케일링 필요
        // 현재는 플랫폼 크기에 맞춰 그리기
        int om = SetStretchBltMode(hDC, HALFTONE);
        
        if (m_imgDome.GetBPP() == 32) {
            BLENDFUNCTION bf = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
            HDC hSrcDC = m_imgDome.GetDC();
            ::AlphaBlend(hDC, dx, dy, dw, dh, hSrcDC, 0, 0, m_imgDome.GetWidth(), m_imgDome.GetHeight(), bf);
            m_imgDome.ReleaseDC();
        } else {
            m_imgDome.TransparentBlt(hDC, dx, dy, dw, dh, 0, 0, m_imgDome.GetWidth(), m_imgDome.GetHeight(), RGB(0, 0, 0));
        }
        
        SetStretchBltMode(hDC, om);
    }

    if (showDebugRect) {
        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(0, 255, 255)); // Cyan for glass dome
        HPEN hOldPen = (HPEN)SelectObject(hDC, hPen);
        HBRUSH hOldBrush = (HBRUSH)SelectObject(hDC, GetStockObject(NULL_BRUSH));
        Rectangle(hDC, dx, dy, dx + dw, dy + dh);
        SelectObject(hDC, hOldPen);
        SelectObject(hDC, hOldBrush);
        DeleteObject(hPen);
    }
}

void GlassDome::Break(DWORD currentTime) {
    if (m_isBroken) return;
    m_isBroken = true;
    
    // 이펙트 발생 (먼지 구름 등)
    float centerX = m_x + m_w / 2.0f;
    float centerY = m_y + m_h / 2.0f;
    for (int i = 0; i < 5; i++) {
        EffectManager::AddDustCloudVFX(m_x + (rand() % (int)m_w), m_y + (rand() % (int)m_h), rand() % 2 == 0, currentTime);
    }
}

void GlassDome::Reset() {
    m_isBroken = false;
}
