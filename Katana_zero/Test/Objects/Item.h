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
    EXPLOSIVE_VIAL,
    FLAMETHROWER
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
    float m_width, m_height;
    bool m_isActive;
    float m_rotation;

    static CImage m_imgBeerBottle[2];
    static CImage m_imgButcherKnife[2];
    static CImage m_imgBust[2];
    static CImage m_imgPottedPlant[2];
    static CImage m_imgKnife[2];
    static CImage m_imgExplosiveVial[2];
    static CImage m_imgFlamethrower[2];
    static CImage m_imgArrow[8];
    static std::map<ItemType, CImage> m_hudImages; // 추가

    bool m_showIndicator;
    int m_arrowFrame;
    DWORD m_arrowLastTime;

public:
    Item(ItemType type, float x, float y);
    virtual ~Item();

    static void LoadAssets();
    static void ReleaseAssets();

    static CImage& GetItemImage(ItemType type, int index = 0);
    static CImage& GetHUDImage(ItemType type);

    void Update(float ts, class Player& player, const std::vector<class Enemy*>& enemies);
    void Render(HDC hdc, Gdiplus::Graphics* g, float camX, float camY, float mapScale);

    void OnPickUp();
    void OnThrow(float vx, float vy);
    void OnHit(const std::vector<Enemy*>& enemies, class Player* player = nullptr);

    void SetPos(float x, float y) { m_x = x; m_y = y; }

    ItemType GetType() const { return m_type; }
    ItemState GetState() const { return m_state; }
    float GetX() const { return m_x; }
    float GetY() const { return m_y; }
    bool IsActive() const { return m_isActive; }
    void SetActive(bool active) { m_isActive = active; }
    void SetShowIndicator(bool show) { m_showIndicator = show; }
};
