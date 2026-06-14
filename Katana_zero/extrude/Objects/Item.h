    #pragma once
#include <windows.h>
#include <atlimage.h>
#include <vector>
#include <map>

namespace Gdiplus { class Graphics; }

enum class ItemType {
    BEER_BOTTLE,
    BUTCHER_KNIFE,
    BUST,
    POTTED_PLANT,
    KNIFE,
    SMOKE_BOMB
};

enum class ItemState {
    ON_GROUND,
    HELD,
    THROWN
};

class Item {
protected:
    ItemType m_type;
    ItemState m_state;
    float m_x, m_y, m_vx, m_vy;
    float m_colW, m_colH;
    bool m_isActive;
    float m_angle;
    float m_rotationSpeed;

    static std::map<ItemType, std::vector<CImage>> m_itemImages;
    static std::vector<CImage> m_arrowImages;
    static std::map<ItemType, CImage> m_hudImages; // 추가: HUD 아이콘 저장

    bool m_showIndicator;
    float m_indicatorOffsetX; // 추가
    float m_indicatorOffsetY; // 추가
    int m_arrowFrame;
    DWORD m_arrowLastTime;

public:
    Item(ItemType type, float x, float y);
    virtual ~Item();

    static void LoadAssets();
    static void ReleaseAssets();
    static CImage& GetItemImage(ItemType type, int index) { 
        if (m_itemImages.find(type) != m_itemImages.end() && index < (int)m_itemImages[type].size()) {
            return m_itemImages[type][index];
        }
        static CImage nullImg; 
        return nullImg;
    }
    static CImage& GetHUDImage(ItemType type) {
        if (m_hudImages.find(type) != m_hudImages.end()) {
            return m_hudImages[type];
        }
        static CImage nullImg;
        return nullImg;
    }

    void Update(float ts, class Player& player, const std::vector<class Enemy*>& enemies);
    void Render(HDC hdc, Gdiplus::Graphics* g, float camX, float camY, float mapScale);

    void OnPickUp();
    void OnThrow(float x, float y, float vx, float vy);

    ItemType GetType() const { return m_type; }
    ItemState GetState() const { return m_state; }
    float GetX() const { return m_x; }
    float GetY() const { return m_y; }
    bool IsActive() const { return m_isActive; }
    void SetActive(bool active) { m_isActive = active; }
    void SetShowIndicator(bool show) { m_showIndicator = show; }
};
