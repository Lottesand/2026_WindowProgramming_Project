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

CImage Item::m_imgBeerBottle[2];
CImage Item::m_imgButcherKnife[2];
CImage Item::m_imgBust[2];
CImage Item::m_imgPottedPlant[2];
CImage Item::m_imgKnife[2];
CImage Item::m_imgExplosiveVial[2];
CImage Item::m_imgFlamethrower[2];
CImage Item::m_imgArrow[8];
std::map<ItemType, CImage> Item::m_hudImages;

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
    if (!m_imgBeerBottle[0].IsNull()) return;
    
    // Beer Bottle
    m_imgBeerBottle[0].Load(TEXT("assets/spr_beer_bottle_3/spr_beer_bottle_3_0.png"));
    m_imgBeerBottle[1].Load(TEXT("assets/spr_beer_bottle_3/spr_beer_bottle_3_1.png"));
    
    // Butcher Knife
    m_imgButcherKnife[0].Load(TEXT("assets/spr_butcher_knife/spr_butcher_knife_0.png"));
    m_imgButcherKnife[1].Load(TEXT("assets/spr_butcher_knife/spr_butcher_knife_1.png"));
    
    // Bust
    m_imgBust[0].Load(TEXT("assets/spr_bust/spr_bust_0.png"));
    m_imgBust[1].Load(TEXT("assets/spr_bust/spr_bust_1.png"));
    
    // Potted Plant
    m_imgPottedPlant[0].Load(TEXT("assets/spr_potted_plant/spr_potted_plant_0.png"));
    m_imgPottedPlant[1].Load(TEXT("assets/spr_potted_plant/spr_potted_plant_1.png"));
    
    // Knife
    m_imgKnife[0].Load(TEXT("assets/spr_knife/spr_knife_0.png"));
    m_imgKnife[1].Load(TEXT("assets/spr_knife/spr_knife_1.png"));
    
    // Explosive Vial
    m_imgExplosiveVial[0].Load(TEXT("assets/spr_explosive_vial/spr_explosive_vial_0.png"));
    m_imgExplosiveVial[1].Load(TEXT("assets/spr_explosive_vial/spr_explosive_vial_1.png"));
    
    // Flamethrower
    m_imgFlamethrower[0].Load(TEXT("assets/spr_flamethrower/spr_flamethrower_0.png"));
    m_imgFlamethrower[1].Load(TEXT("assets/spr_flamethrower/spr_flamethrower_1.png"));
    
    // HUD 아이콘 로드
    m_hudImages[ItemType::BEER_BOTTLE].Load(TEXT("assets/hud/inven_beer_bottle.png"));
    m_hudImages[ItemType::BUTCHER_KNIFE].Load(TEXT("assets/hud/inven_butcher_knife.png"));
    m_hudImages[ItemType::BUST].Load(TEXT("assets/hud/inven_bust.png"));
    m_hudImages[ItemType::POTTED_PLANT].Load(TEXT("assets/hud/inven_potted_plant.png"));
    m_hudImages[ItemType::KNIFE].Load(TEXT("assets/hud/inven_knife.png"));

    TCHAR path[256];
    for (int i = 0; i < 8; i++) {
        wsprintf(path, TEXT("assets/arrow/%d.png"), i);
        m_imgArrow[i].Load(path);
    }
}

void Item::ReleaseAssets() {
    for (int i = 0; i < 2; i++) {
        m_imgBeerBottle[i].Destroy();
        m_imgButcherKnife[i].Destroy();
        m_imgBust[i].Destroy();
        m_imgPottedPlant[i].Destroy();
        m_imgKnife[i].Destroy();
        m_imgExplosiveVial[i].Destroy();
        m_imgFlamethrower[i].Destroy();
    }
    for (auto& pair : m_hudImages) {
        pair.second.Destroy();
    }
    m_hudImages.clear();
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
    int idx = (index < 0 || index > 1) ? 0 : index;
    switch (type) {
    case ItemType::BEER_BOTTLE: return m_imgBeerBottle[idx];
    case ItemType::BUTCHER_KNIFE: return m_imgButcherKnife[idx];
    case ItemType::BUST: return m_imgBust[idx];
    case ItemType::POTTED_PLANT: return m_imgPottedPlant[idx];
    case ItemType::KNIFE: return m_imgKnife[idx];
    case ItemType::EXPLOSIVE_VIAL: return m_imgExplosiveVial[idx];
    case ItemType::FLAMETHROWER: return m_imgFlamethrower[idx];
    }
    return m_imgBeerBottle[idx];
}

CImage& Item::GetHUDImage(ItemType type) {
    if (m_hudImages.find(type) != m_hudImages.end() && !m_hudImages[type].IsNull()) {
        return m_hudImages[type];
    }
    return GetItemImage(type, 0);
}

void Item::Update(float ts, Player& player, const std::vector<Enemy*>& enemies) {
    if (!m_isActive || m_state == ItemState::HELD) return;

    if (m_state == ItemState::ON_GROUND) {
        if (m_showIndicator) {
            DWORD ct = GetTickCount();
            if (ct - m_arrowLastTime >= 80) {
                m_arrowFrame = (m_arrowFrame + 1) % 8;
                m_arrowLastTime = ct;
            }
        }
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
                    // Add Neon Trail on impact (mimicking player attack/damage logic)
                    float speed = sqrt(m_vx * m_vx + m_vy * m_vy);
                    if (speed > 0) {
                        float ux = m_vx / speed;
                        float uy = m_vy / speed;
                        float angle = atan2(uy, ux);
                        EffectManager::AddNeonTrail(m_x + m_width / 2.0f, m_y + m_height / 2.0f, ux, uy, angle);
                    }

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
    
    // 아이템 렌더링 배율 설정 (플레이어 대비 크기 조절)
    const float ITEM_RENDER_SCALE = 1.0f;

    CImage* img = &GetItemImage(m_type, 0);
    if (img && !img->IsNull()) {
        int fw = (int)(img->GetWidth() * ITEM_RENDER_SCALE * mapScale);
        int fh = (int)(img->GetHeight() * ITEM_RENDER_SCALE * mapScale);
        int dx = (int)((m_x - camX) * mapScale), dy = (int)((m_y - camY) * mapScale);
        
        if (m_state == ItemState::THROWN || (m_state == ItemState::ON_GROUND && m_type == ItemType::KNIFE)) {
            float rad = (m_state == ItemState::THROWN) ? m_rotation : (3.141592f / 2.0f);
            Gdiplus::Bitmap bmp(img->GetWidth(), img->GetHeight(), img->GetPitch(), PixelFormat32bppARGB, (BYTE*)img->GetBits());
            g->TranslateTransform((float)dx + fw / 2.0f, (float)dy + fh / 2.0f);
            g->RotateTransform(rad * 180.0f / 3.141592f);
            g->DrawImage(&bmp, -fw / 2.0f, -fh / 2.0f, (float)fw, (float)fh);
            g->ResetTransform();
        } else {
            img->Draw(hdc, dx, dy, fw, fh);
        }
    }

    if (m_state == ItemState::ON_GROUND && m_showIndicator) {
        CImage& arrowImg = m_imgArrow[m_arrowFrame];
        if (!arrowImg.IsNull()) {
            int aw = (int)(arrowImg.GetWidth() * mapScale * 1.5f);
            int ah = (int)(arrowImg.GetHeight() * mapScale * 1.5f);
            int ax = (int)((m_x + m_width / 2.0f - camX) * mapScale) - aw / 2;
            float bobbingOffset = sin(GetTickCount() * 0.005f) * 5.0f;
            int ay = (int)((m_y - camY) * mapScale) - ah - (int)(10.0f * mapScale) + (int)(bobbingOffset * mapScale);
            
            void* bits = arrowImg.GetBits();
            if (bits) {
                Gdiplus::Bitmap bmp(arrowImg.GetWidth(), arrowImg.GetHeight(), arrowImg.GetPitch(), PixelFormat32bppARGB, (BYTE*)bits);
                g->DrawImage(&bmp, (float)ax, (float)ay, (float)aw, (float)ah);
            } else {
                arrowImg.Draw(hdc, ax, ay, aw, ah);
            }
        }
    }
}
