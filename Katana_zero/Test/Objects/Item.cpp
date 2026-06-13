#include "Item.h"
#include "Player.h"
#include "Enemy.h"
#include "Physics.h"
#include "../Effects/EffectManager.h"
#include "../SceneAndMap/StageManager.h"
#include "../SceneAndMap/Camera.h"
#include "../Core/SoundManager.h"
#include <cmath>
#include <algorithm>

CImage Item::m_imgBeerBottle;
CImage Item::m_imgButcherKnife;
CImage Item::m_imgBust;
CImage Item::m_imgPottedPlant;
CImage Item::m_imgKnife;
CImage Item::m_imgExplosiveVial;
CImage Item::m_imgFlamethrower;
CImage Item::m_imgArrow[8];

Item::Item(ItemType type, float x, float y) : m_type(type), m_x(x), m_y(y) {
    m_state = ItemState::ON_GROUND;
    m_vx = 0; m_vy = 0;
    m_width = 30.0f; m_height = 30.0f;
    m_isActive = true;
    m_showIndicator = false;
    m_arrowFrame = 0;
    m_arrowLastTime = GetTickCount();
    m_rotation = 0.0f;
}

Item::~Item() {}

void Item::LoadAssets() {
    if (!m_imgBeerBottle.IsNull()) return;
    m_imgBeerBottle.Load(TEXT("assets/spr_beer_bottle_3/0.png"));
    m_imgButcherKnife.Load(TEXT("assets/spr_butcher_knife/0.png"));
    m_imgBust.Load(TEXT("assets/spr_bust/0.png"));
    m_imgPottedPlant.Load(TEXT("assets/spr_potted_plant/0.png"));
    m_imgKnife.Load(TEXT("assets/spr_knife/0.png"));
    m_imgExplosiveVial.Load(TEXT("assets/spr_explosive_vial/0.png"));
    m_imgFlamethrower.Load(TEXT("assets/spr_flamethrower/0.png"));
    
    TCHAR path[256];
    for (int i = 0; i < 8; i++) {
        wsprintf(path, TEXT("assets/arrow/%d.png"), i);
        m_imgArrow[i].Load(path);
    }
}

void Item::ReleaseAssets() {
    m_imgBeerBottle.Destroy();
    m_imgButcherKnife.Destroy();
    m_imgBust.Destroy();
    m_imgPottedPlant.Destroy();
    m_imgKnife.Destroy();
    m_imgExplosiveVial.Destroy();
    m_imgFlamethrower.Destroy();
    for (int i = 0; i < 8; i++) m_imgArrow[i].Destroy();
}

void Item::OnPickUp() {
    m_state = ItemState::HELD;
    m_isActive = true;
}

void Item::OnThrow(float vx, float vy) {
    m_state = ItemState::THROWN;
    m_vx = vx;
    m_vy = vy;
    m_isActive = true;
}

void Item::OnHit(const std::vector<Enemy*>& enemies) {
    if (m_type == ItemType::EXPLOSIVE_VIAL) {
        float explosionRadius = 150.0f;
        EffectManager::AddExplosion(m_x, m_y, GetTickCount(), explosionRadius);
        SoundManager::Play("SFX_EXPLOSION");

        for (auto enemy : enemies) {
            if (enemy && enemy->GetIsAlive()) {
                float ex = enemy->GetX() + enemy->GetColW() / 2.0f;
                float ey = enemy->GetY() + enemy->GetColH() / 2.0f;
                float dx = ex - m_x, dy = ey - m_y;
                if (sqrt(dx * dx + dy * dy) <= explosionRadius) {
                    float kbx = (ex > m_x) ? 20.0f : -20.0f;
                    enemy->OnTakeDamage(kbx, -10.0f, DeathCause::FIRE);
                }
            }
        }
    }
}

CImage& Item::GetItemImage(ItemType type, int index) {
    switch (type) {
    case ItemType::BEER_BOTTLE: return m_imgBeerBottle;
    case ItemType::BUTCHER_KNIFE: return m_imgButcherKnife;
    case ItemType::BUST: return m_imgBust;
    case ItemType::POTTED_PLANT: return m_imgPottedPlant;
    case ItemType::KNIFE: return m_imgKnife;
    case ItemType::EXPLOSIVE_VIAL: return m_imgExplosiveVial;
    case ItemType::FLAMETHROWER: return m_imgFlamethrower;
    }
    return m_imgBeerBottle;
}

CImage& Item::GetHUDImage(ItemType type) {
    return GetItemImage(type, 0);
}

void Item::Update(float ts, Player& player, const std::vector<Enemy*>& enemies) {
    if (!m_isActive || m_state == ItemState::HELD) return;

    if (m_state == ItemState::ON_GROUND) {
        m_vy += 1.0f * ts;
        float ny = m_y + m_vy * ts;
        if (CheckMapCollision(m_x, ny, m_width, m_height)) m_vy = 0;
        else m_y = ny;
    } else if (m_state == ItemState::THROWN) {
        m_x += m_vx * ts; m_y += m_vy * ts; m_rotation += 0.5f * ts;
        RECT itemR = { (int)m_x, (int)m_y, (int)(m_x + m_width), (int)(m_y + m_height) };
        for (auto enemy : enemies) {
            if (enemy && enemy->GetIsAlive()) {
                RECT eR = enemy->GetRect(), ol;
                if (IntersectRect(&ol, &itemR, &eR)) {
                    DeathCause cause = (m_type == ItemType::EXPLOSIVE_VIAL) ? DeathCause::FIRE : 
                                      ((m_type == ItemType::KNIFE || m_type == ItemType::BUTCHER_KNIFE) ? DeathCause::KNIFE : DeathCause::BOTTLE);
                    enemy->OnTakeDamage(m_vx * 0.5f, -5.0f, cause);
                    OnHit(enemies); m_isActive = false; return;
                }
            }
        }
        if (CheckMapCollision(m_x, m_y, m_width, m_height)) { OnHit(enemies); m_isActive = false; }
    }
}

void Item::Render(HDC hdc, Gdiplus::Graphics* g, float camX, float camY, float mapScale) {
    if (!m_isActive || m_state == ItemState::HELD) return;
    CImage* img = &GetItemImage(m_type, 0);
    if (img && !img->IsNull()) {
        int fw = (int)(img->GetWidth() * 2.0f * mapScale), fh = (int)(img->GetHeight() * 2.0f * mapScale);
        int dx = (int)((m_x - camX) * mapScale), dy = (int)((m_y - camY) * mapScale);
        if (m_state == ItemState::THROWN) {
            Gdiplus::Bitmap bmp(img->GetWidth(), img->GetHeight(), img->GetPitch(), PixelFormat32bppARGB, (BYTE*)img->GetBits());
            g->TranslateTransform((float)dx + fw / 2.0f, (float)dy + fh / 2.0f);
            g->RotateTransform(m_rotation * 180.0f / 3.141592f);
            g->DrawImage(&bmp, -fw / 2.0f, -fh / 2.0f, (float)fw, (float)fh);
            g->ResetTransform();
        } else img->Draw(hdc, dx, dy, fw, fh);
    }
    if (m_state == ItemState::ON_GROUND && m_showIndicator) {
        CImage& arrowImg = m_imgArrow[m_arrowFrame];
        if (!arrowImg.IsNull()) {
            int aw = (int)(arrowImg.GetWidth() * 2.0f * mapScale), ah = (int)(arrowImg.GetHeight() * 2.0f * mapScale);
            int ax = (int)((m_x + m_width / 2.0f - camX) * mapScale) - aw / 2, ay = (int)((m_y - 40.0f - camY) * mapScale);
            arrowImg.Draw(hdc, ax, ay, aw, ah);
        }
    }
}
