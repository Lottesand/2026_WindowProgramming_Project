#pragma once
#include <windows.h>
#include <atlimage.h>
#include <vector>

namespace Gdiplus { class Graphics; }

class OilDrum {
public:
    OilDrum(float x, float y);
    ~OilDrum();

    static void LoadAssets();
    static void ReleaseAssets();

    void Update(float ts, const std::vector<class Enemy*>& enemies, std::vector<OilDrum>& allDrums, class Player* player);
    void Render(HDC hdc, Gdiplus::Graphics* g, float camX, float camY, float mapScale);

    void Trigger(DWORD delay); 
    void Explode(const std::vector<class Enemy*>& enemies, std::vector<OilDrum>& allDrums, class Player* player);
    
    bool IsExploded() const { return m_isExploded; }
    bool IsPending() const { return m_isPending; }
    float GetX() const { return m_x; }
    float GetY() const { return m_y; }
    float GetWidth() const { return m_width; }
    float GetHeight() const { return m_height; }

private:
    float m_x, m_y;
    float m_width, m_height;
    bool m_isExploded;
    bool m_isPending;
    DWORD m_explodeTime;

    static CImage m_imgDrum;
};
