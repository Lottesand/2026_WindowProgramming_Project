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
    static void UpdateAll(float ts, class Player& player, const std::vector<class Enemy*>& enemies);
    static void RenderAll(HDC hdc, float camX, float camY, float mapScale);
    static void AddBullet(float x, float y, float vx, float vy);
    static void ClearAll();
    static const std::vector<Bullet*>& GetBullets() { return m_bullets; }

    void Update(float ts, class Player& player, const std::vector<class Enemy*>& enemies);
    void Render(HDC hdc, float camX, float camY, float mapScale);

    void Deflect(float newVx, float newVy);
    bool IsDeflected() const { return m_isDeflected; }
    void SetDeflected(bool deflected) { m_isDeflected = deflected; }
    float GetX() const { return m_x; }
    float GetY() const { return m_y; }
    float GetVX() const { return m_vx; }
    float GetVY() const { return m_vy; }
    float GetWidth() const { return m_width; }
    float GetHeight() const { return m_height; }
    float GetRadius() const { return (m_width + m_height) / 4.0f; }
    bool IsActive() const { return m_isActive; }
    RECT GetRect() const { return { (int)m_x, (int)m_y, (int)(m_x + m_width), (int)(m_y + m_height) }; }

private:
    float m_x, m_y;
    float m_vx, m_vy;
    float m_width, m_height;
    bool m_isActive;
    bool m_isDeflected;

    static CImage m_imgBullet;
    static std::vector<Bullet*> m_bullets;
};
