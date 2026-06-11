#pragma once
#include <windows.h>
#include <atlimage.h>

class GlassDome {
public:
    GlassDome(float x, float y, float w, float h);
    ~GlassDome();

    void Update(float attackHitX, float attackHitY, float attackHitW, float attackHitH, bool isAttacking, DWORD currentTime);
    void Render(HDC hDC, float camX, float camY, float mapScale, bool showDebugRect);
    void Break(DWORD currentTime);
    void Reset();

    bool IsBroken() const { return m_isBroken; }
    float GetX() const { return m_x; }
    float GetY() const { return m_y; }
    float GetW() const { return m_w; }
    float GetH() const { return m_h; }

    static void LoadAssets();
    static void ReleaseAssets();

private:
    float m_x, m_y, m_w, m_h;
    bool m_isBroken;
    
    static CImage m_imgDome;
};
