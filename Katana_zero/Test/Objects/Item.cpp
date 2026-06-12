#include "Item.h"
#include "Player.h"
#include "Enemy.h"
#include "Physics.h"
#include "../Effects/EffectManager.h"
#include "../SceneAndMap/StageManager.h"
#include "../SceneAndMap/Camera.h"
#include <cmath>
#include <gdiplus.h>

// 아이템 렌더링 배율 설정 (플레이어 대비 크기 조절)
const float ITEM_RENDER_SCALE = 1.0f;

std::map<ItemType, std::vector<CImage>> Item::m_itemImages;
std::vector<CImage> Item::m_arrowImages;
std::map<ItemType, CImage> Item::m_hudImages;

Item::Item(ItemType type, float x, float y) : m_type(type), m_x(x), m_y(y) {
    m_state = ItemState::ON_GROUND;
    m_vx = 0; m_vy = 0;
    m_isActive = true;
    m_angle = 0;
    m_rotationSpeed = 0;
    m_showIndicator = false;
    m_arrowFrame = 0;
    m_arrowLastTime = GetTickCount();
    
    // 타입별 속성 및 인디케이터 오프셋 설정
    m_colW = 20.0f;
    m_colH = 20.0f;
    m_indicatorOffsetX = 10.0f; 
    m_indicatorOffsetY = 0.0f;

    if (m_type == ItemType::BEER_BOTTLE) {
        m_indicatorOffsetX = 5.0f;
    }
    else if (m_type == ItemType::KNIFE) {
        m_indicatorOffsetX = 20.0f;
    }
}

Item::~Item() {}

void Item::LoadAssets() {
    // 맥주병 이미지 로드
    auto& beerImages = m_itemImages[ItemType::BEER_BOTTLE];
    if (beerImages.empty()) {
        beerImages.resize(2);
        beerImages[0].Load(TEXT("assets/spr_beer_bottle_3/spr_beer_bottle_3_0.png"));
        beerImages[1].Load(TEXT("assets/spr_beer_bottle_3/spr_beer_bottle_3_1.png"));
    }

    // 푸줏간 칼 이미지 로드
    auto& knifeImages = m_itemImages[ItemType::BUTCHER_KNIFE];
    if (knifeImages.empty()) {
        knifeImages.resize(2);
        knifeImages[0].Load(TEXT("assets/spr_butcher_knife/spr_butcher_knife_0.png"));
        knifeImages[1].Load(TEXT("assets/spr_butcher_knife/spr_butcher_knife_1.png"));
    }

    // BUST 이미지 로드
    auto& bustImages = m_itemImages[ItemType::BUST];
    if (bustImages.empty()) {
        bustImages.resize(2);
        bustImages[0].Load(TEXT("assets/spr_bust/spr_bust_0.png"));
        bustImages[1].Load(TEXT("assets/spr_bust/spr_bust_1.png"));
    }

    // POTTED_PLANT 이미지 로드
    auto& plantImages = m_itemImages[ItemType::POTTED_PLANT];
    if (plantImages.empty()) {
        plantImages.resize(2);
        plantImages[0].Load(TEXT("assets/spr_potted_plant/spr_potted_plant_0.png"));
        plantImages[1].Load(TEXT("assets/spr_potted_plant/spr_potted_plant_1.png"));
    }

    // KNIFE 이미지 로드
    auto& knifeItemImages = m_itemImages[ItemType::KNIFE];
    if (knifeItemImages.empty()) {
        knifeItemImages.resize(2);
        knifeItemImages[0].Load(TEXT("assets/spr_knife/spr_knife_0.png"));
        knifeItemImages[1].Load(TEXT("assets/spr_knife/spr_knife_1.png"));
    }

    // HUD 아이콘 로드
    m_hudImages[ItemType::BEER_BOTTLE].Load(TEXT("assets/hud/inven_beer_bottle.png"));
    m_hudImages[ItemType::BUTCHER_KNIFE].Load(TEXT("assets/hud/inven_butcher_knife.png"));
    m_hudImages[ItemType::BUST].Load(TEXT("assets/hud/inven_bust.png"));
    m_hudImages[ItemType::POTTED_PLANT].Load(TEXT("assets/hud/inven_potted_plant.png"));
    m_hudImages[ItemType::KNIFE].Load(TEXT("assets/hud/inven_knife.png"));

    // 화살표 이미지 로드 (assets/arrow/0.png ~ 7.png)
    if (m_arrowImages.empty()) {
        m_arrowImages.resize(8);
        for (int i = 0; i < 8; ++i) {
            TCHAR path[256];
            wsprintf(path, TEXT("assets/arrow/%d.png"), i);
            if (FAILED(m_arrowImages[i].Load(path))) {
                OutputDebugString(TEXT("[Item] Failed to load arrow image\n"));
            }
        }
    }
}

void Item::ReleaseAssets() {
    for (auto& pair : m_itemImages) {
        for (auto& img : pair.second) {
            if (!img.IsNull()) img.Destroy();
        }
    }
    m_itemImages.clear();

    for (auto& img : m_arrowImages) {
        if (!img.IsNull()) img.Destroy();
    }
    m_arrowImages.clear();

    for (auto& pair : m_hudImages) {
        if (!pair.second.IsNull()) pair.second.Destroy();
    }
    m_hudImages.clear();
}

void Item::OnPickUp() {
    m_state = ItemState::HELD;
    m_showIndicator = false;
}

void Item::OnThrow(float x, float y, float vx, float vy) {
    m_x = x;
    m_y = y;
    m_vx = vx;
    m_vy = vy;
    m_state = ItemState::THROWN;
    m_rotationSpeed = 0.3f; // 회전 속도 조정
    m_isActive = true;
    m_showIndicator = false;
}

void Item::Update(float ts, Player& player, const std::vector<class Enemy*>& enemies) {
    if (!m_isActive || m_state == ItemState::HELD) return;

    // 수동 습득 가능 거리 체크 (화살표 표시용)
    if (m_state == ItemState::ON_GROUND) {
        if (m_showIndicator) {
            DWORD ct = GetTickCount();
            if (ct - m_arrowLastTime >= 80) { // 화살표 애니메이션 속도
                m_arrowFrame = (m_arrowFrame + 1) % 8;
                m_arrowLastTime = ct;
            }
        }
    } else {
        m_showIndicator = false;
    }

    if (m_state == ItemState::THROWN) {
        m_angle += m_rotationSpeed * ts * 20.0f; // Rotation

        float nx = m_x + m_vx * ts;
        float ny = m_y + m_vy * ts;

        // Check enemy collision (using center-based or bounding box)
        if (!enemies.empty()) {
            float itemCX = nx + m_colW / 2.0f;
            float itemCY = ny + m_colH / 2.0f;

            for (auto& enemy : enemies) {
                if (enemy && enemy->GetIsAlive()) {
                    float ex = enemy->GetX(), ey = enemy->GetY(), ew = enemy->GetColW(), eh = enemy->GetColH();
                    
                    // Simple AABB collision
                    bool hit = (nx < ex + ew && nx + m_colW > ex && ny < ey + eh && ny + m_colH > ey);

                    if (hit) {
                        // Kill enemy
                        enemy->OnTakeDamage(m_vx * 0.5f, -5.0f);
                        
                        // Add blood and hit line effect
                        float angle = atan2(m_vy, m_vx);
                        EffectManager::AddBloodSplatter(itemCX, itemCY, m_vx * 0.2f, m_vy * 0.2f, angle, GetTickCount());
                        
                        // Add hit sprite and neon trail effect
                        float dist = sqrt(m_vx * m_vx + m_vy * m_vy);
                        float ux = m_vx / dist;
                        float uy = m_vy / dist;
                        EffectManager::AddNeonTrail(itemCX, itemCY, ux, uy, angle);
                        EffectManager::AddHitVFX(itemCX, itemCY, angle, GetTickCount());
                        // EffectManager::AddBluntImpactVFX(itemCX, itemCY, angle, GetTickCount()); // TODO: Add if needed

                        // Add camera shake and feedback (similar to standard attack)
                        float camUx = cos(angle), camUy = sin(angle);
                        Camera::AddPush(camUx * 20.0f, camUy * 20.0f);
                        Camera::AddShake(0.8f);
                        
                        m_isActive = false;
                        // Break effect
                        // EffectManager::AddGlassShards(itemCX, itemCY); // TODO: Add if needed
                        return;
                    }
                }
            }
        }


        // Check map collision (using center and corners for better detection)
        if (CheckMapCollision(nx, ny, m_colW, m_colH) || 
            CheckMapCollision(nx + m_colW, ny, 1, 1) || 
            CheckMapCollision(nx, ny + m_colH, 1, 1) || 
            CheckMapCollision(nx + m_colW, ny + m_colH, 1, 1)) {
            
            m_isActive = false;
            // Break effect at collision point
            // EffectManager::AddGlassShards(nx + m_colW / 2.0f, ny + m_colH / 2.0f); // TODO: Add if needed
        } else {
            m_x = nx;
            m_y = ny;
        }
    }
}

void Item::Render(HDC hdc, Gdiplus::Graphics* g, float camX, float camY, float mapScale) {
    if (!m_isActive || m_state == ItemState::HELD) return;

    int dx = (int)((m_x - camX) * mapScale);
    int dy = (int)((m_y - camY) * mapScale);

    // 화살표 인디케이터 렌더링
    if (m_showIndicator && !m_arrowImages.empty()) {
        CImage& arrowImg = m_arrowImages[m_arrowFrame];
        if (!arrowImg.IsNull()) {
            int aw = (int)(arrowImg.GetWidth() * mapScale * 1.5f);
            int ah = (int)(arrowImg.GetHeight() * mapScale * 1.5f);
            // 아이템별 오프셋 적용
            int ax = dx + (int)(m_colW * mapScale) / 2 - aw / 2 + (int)(m_indicatorOffsetX * mapScale);
            // 위아래로 둥실거리는 효과 추가 (사인파 이용)
            float bobbingOffset = sin(GetTickCount() * 0.005f) * 5.0f;
            int ay = dy - ah - (int)(10.0f * mapScale) + (int)(bobbingOffset * mapScale) + (int)(m_indicatorOffsetY * mapScale); // 아이템 위 10px 위치 + 둥실거림 + 오프셋
            
            // GDI+를 사용하여 알파 채널을 올바르게 처리
            void* bits = arrowImg.GetBits();
            if (bits) {
                Gdiplus::Bitmap bmp(arrowImg.GetWidth(), arrowImg.GetHeight(), arrowImg.GetPitch(), PixelFormat32bppARGB, (BYTE*)bits);
                g->DrawImage(&bmp, (float)ax, (float)ay, (float)aw, (float)ah);
            } else {
                // 백업: TransparentBlt (검정색 배경 제거)
                arrowImg.TransparentBlt(hdc, ax, ay, aw, ah, 0, 0, arrowImg.GetWidth(), arrowImg.GetHeight(), RGB(0, 0, 0));
            }
        }
    }

    // 아이템 타입에 따른 이미지 사용
    CImage* img = &GetItemImage(m_type, 0);
    if (img && !img->IsNull()) {
        int fw = (int)(img->GetWidth() * ITEM_RENDER_SCALE * mapScale);
        int fh = (int)(img->GetHeight() * ITEM_RENDER_SCALE * mapScale);
        
        if (m_state == ItemState::THROWN || (m_state == ItemState::ON_GROUND && m_type == ItemType::KNIFE)) {
            // GDI+를 사용하여 투명도 유지 및 회전 렌더링
            float rad = (m_state == ItemState::THROWN) ? m_angle : (3.141592f / 2.0f);
            
            Gdiplus::Bitmap bmp(img->GetWidth(), img->GetHeight(), img->GetPitch(), PixelFormat32bppARGB, (BYTE*)img->GetBits());
            
            g->TranslateTransform((float)dx + fw / 2.0f, (float)dy + fh / 2.0f);
            g->RotateTransform(rad * 180.0f / 3.141592f);
            g->DrawImage(&bmp, -fw / 2.0f, -fh / 2.0f, (float)fw, (float)fh);
            g->ResetTransform();
        } else {
            img->Draw(hdc, dx, dy, fw, fh);
        }
    }
}
