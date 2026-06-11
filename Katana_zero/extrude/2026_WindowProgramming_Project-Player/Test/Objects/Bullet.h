#pragma once
#include <windows.h>
#include <atlimage.h>
#include <vector>

class Bullet {
public:
    Bullet(float x, float y, float vx, float vy);
    ~Bullet();

    static void Init();
    static void Release();
    static void UpdateAll(float ts, class Player& player);
    static void RenderAll(HDC hdc, float camX, float camY, float mapScale);
    static void AddBullet(float x, float y, float vx, float vy);
    static void ClearAll();

    void Update(float ts, class Player& player);
    void Render(HDC hdc, float camX, float camY, float mapScale);

    bool IsActive() const { return m_isActive; }
    RECT GetRect() const { return { (int)m_x, (int)m_y, (int)(m_x + m_width), (int)(m_y + m_height) }; }

private:
    float m_x, m_y;
    float m_vx, m_vy;
    float m_width, m_height;
    float m_angle;
    bool m_isActive;

    static CImage m_imgBullet;
    static std::vector<Bullet*> m_bullets;
};
