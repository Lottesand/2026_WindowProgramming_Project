#include "EndScene.h"
#include <cmath>
#include <cstdlib>

bool EndScene::m_active = false;
float EndScene::m_timer = 0.0f;
float EndScene::m_titleAlpha = 0.0f;
float EndScene::m_titleYOffset = 50.0f;

CImage EndScene::m_imgBackground;
CImage EndScene::m_imgTitle;

void EndScene::Init() {}

void EndScene::LoadAssets() {
    if (m_imgBackground.IsNull()) m_imgBackground.Load(TEXT("assets/spr_endoffice.png"));
    if (m_imgTitle.IsNull()) m_imgTitle.Load(TEXT("assets/spr_ending_title.png"));
}

void EndScene::ReleaseAssets() {
    m_imgBackground.Destroy();
    m_imgTitle.Destroy();
}

void EndScene::Reset() {
    m_timer = 0.0f;
    m_titleAlpha = 0.0f;
    m_titleYOffset = 50.0f;
}

void EndScene::Update(float dT) {
    if (!m_active) return;
    m_timer += dT;

    // Fade in and slide up title after a small delay
    if (m_timer > 1.0f) {
        if (m_titleAlpha < 1.0f) {
            m_titleAlpha += dT * 0.5f;
            if (m_titleAlpha > 1.0f) m_titleAlpha = 1.0f;
        }
        if (m_titleYOffset > 0.0f) {
            m_titleYOffset -= dT * 20.0f;
            if (m_titleYOffset < 0.0f) m_titleYOffset = 0.0f;
        }
    }
}

void EndScene::Render(HDC hDC, int virtualWidth, int virtualHeight) {
    if (!m_active) return;

    // 1. Background
    if (!m_imgBackground.IsNull()) {
        m_imgBackground.Draw(hDC, 0, 0, virtualWidth, virtualHeight);
    }

    // 2. TV Static Effect
    RenderStaticEffect(hDC, virtualWidth, virtualHeight);

    // 3. Ending Title
    if (!m_imgTitle.IsNull() && m_titleAlpha > 0.01f) {
        Gdiplus::Graphics g(hDC);
        int tw = m_imgTitle.GetWidth();
        int th = m_imgTitle.GetHeight();
        int dx = (virtualWidth - tw) / 2;
        int dy = (virtualHeight - th) / 2 + (int)m_titleYOffset + 100; // Positioned lower

        Gdiplus::ImageAttributes attr;
        Gdiplus::ColorMatrix mat = {
            1, 0, 0, 0, 0,
            0, 1, 0, 0, 0,
            0, 0, 1, 0, 0,
            0, 0, 0, m_titleAlpha, 0,
            0, 0, 0, 0, 1
        };
        attr.SetColorMatrix(&mat);

        Gdiplus::Bitmap bmp(m_imgTitle, NULL);
        g.DrawImage(&bmp, Gdiplus::Rect(dx, dy, tw, th), 0, 0, tw, th, Gdiplus::UnitPixel, &attr);
    }
}

void EndScene::RenderStaticEffect(HDC hDC, int w, int h) {
    // Simple TV static: draw random semi-transparent white/black dots or lines
    // To make it look "glitchy", we'll draw some horizontal thin lines
    for (int i = 0; i < 15; i++) {
        int ly = rand() % h;
        int lh = rand() % 3 + 1;
        int lw = w;
        int alpha = rand() % 50 + 20;

        HBRUSH hBrush = CreateSolidBrush(RGB(200, 200, 200)); // Light gray
        RECT r = { 0, ly, lw, ly + lh };
        
        // Use AlphaBlend for semi-transparency if available, 
        // but for a "지지직" effect, even solid random thin lines work well.
        // We'll use GDI+ for transparency
        Gdiplus::Graphics g(hDC);
        Gdiplus::SolidBrush staticBrush(Gdiplus::Color(alpha, 255, 255, 255));
        g.FillRectangle(&staticBrush, 0, ly, w, lh);
        DeleteObject(hBrush);
    }

    // Occasional full-screen flicker
    if (rand() % 100 < 5) {
        Gdiplus::Graphics g(hDC);
        Gdiplus::SolidBrush flickerBrush(Gdiplus::Color(30, 255, 255, 255));
        g.FillRectangle(&flickerBrush, 0, 0, w, h);
    }
}
