#pragma once
#include <windows.h>
#include <atlimage.h>
#include <gdiplus.h>

class EndScene {
public:
    static void Init();
    static void LoadAssets();
    static void ReleaseAssets();
    
    static void Reset();
    static void Update(float dT);
    static void Render(HDC hDC, int virtualWidth, int virtualHeight);

    static bool IsActive() { return m_active; }
    static void SetActive(bool active) { m_active = active; if (active) Reset(); }

private:
    static bool m_active;
    static float m_timer;
    static float m_titleAlpha;
    static float m_titleYOffset;

    static CImage m_imgBackground;
    static CImage m_imgTitle;

    static constexpr int VIRTUAL_WIDTH = 1280;
    static constexpr int VIRTUAL_HEIGHT = 720;

    static void RenderStaticEffect(HDC hDC, int w, int h);
};
