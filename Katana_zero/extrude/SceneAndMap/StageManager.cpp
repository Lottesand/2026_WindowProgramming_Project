#include "StageManager.h"
#include "../Effects/EffectManager.h"
#include <algorithm>
#include <atomic>

std::map<int, CImage> StageManager::m_mapImages;
std::map<int, CImage> StageManager::m_colMapImages;
std::map<int, CImage> StageManager::m_objMapImages;
std::map<int, StageManager::StageData> StageManager::m_stageDataMap;

CImage* StageManager::m_imgMap = nullptr;
CImage* StageManager::m_imgColMap = nullptr;
CImage* StageManager::m_imgObjMap = nullptr;

CImage StageManager::m_imgSkylineBlack;
CImage StageManager::m_imgSkylineClouds;
std::vector<Door>* StageManager::m_pCurrentDoors = nullptr;
std::vector<GlassDome>* StageManager::m_pCurrentGlassDomes = nullptr;
std::vector<Item>* StageManager::m_pCurrentItems = nullptr;
std::vector<Item> StageManager::m_activeItems; // 정의 추가
POINT StageManager::m_playerStart = { 0, 0 };
std::vector<RECT> StageManager::m_clearZones;
std::vector<class Enemy*> StageManager::m_currentEnemies;
float StageManager::m_stageLimitTime = 0.0f;
int StageManager::m_currentStage = 1;

void StageManager::SetCurrentEnemies(const std::vector<class Enemy*>& enemies) {
    m_currentEnemies = enemies;
}

void StageManager::LoadAllStages(std::atomic<int>* pProgress) {
    for (int i = 1; i <= 5; ++i) {
        TCHAR mapPath[256], colPath[256], objPath[256];
        wsprintf(mapPath, TEXT("assets/stage%d/map_stage%d.png"), i, i);
        wsprintf(colPath, TEXT("assets/stage%d/colmap_stage%d.png"), i, i);
        wsprintf(objPath, TEXT("assets/stage%d/objmap_stage%d.png"), i, i);

        // Map과 Colmap은 필수, Objmap은 선택 사항으로 로딩
        bool mapLoaded = SUCCEEDED(m_mapImages[i].Load(mapPath));
        bool colLoaded = SUCCEEDED(m_colMapImages[i].Load(colPath));
        // Objmap은 로드 시도만 하고 결과는 무시 (실패해도 진행)
        m_objMapImages[i].Load(objPath);
        bool objLoaded = !m_objMapImages[i].IsNull();

        if (mapLoaded && colLoaded) {
            ProcessStage(i);
        } else {
            TCHAR debugMsg[512];
            wsprintf(debugMsg, TEXT("[StageManager] Failed to load essential assets for stage %d: Map=%d, Col=%d\n"), i, mapLoaded, colLoaded);
            OutputDebugString(debugMsg);
        }

        if (pProgress) {
            *pProgress = 60 + (i * 7); 
            Sleep(50);
        }
    }

    if (m_imgSkylineBlack.IsNull()) m_imgSkylineBlack.Load(TEXT("assets/spr_skyline_black.png"));
    if (m_imgSkylineClouds.IsNull()) m_imgSkylineClouds.Load(TEXT("assets/spr_skyline_clouds.png"));
    
    if (pProgress) {
        *pProgress = 95;
    }
}

void StageManager::Init() {
    EffectManager::Init();
}

void StageManager::ProcessStage(int stage) {
    StageData& data = m_stageDataMap[stage];
    CImage& colMap = m_colMapImages[stage];
    CImage& objMap = m_objMapImages[stage];

    data.doors.clear();
    data.glassDomes.clear();
    data.items.clear();
    data.clearZones.clear();
    data.enemySpawns.clear();
    data.playerStart = { 100, 100 };
    data.stageLimitTime = (stage == 1) ? 30.0f : 60.0f;

    // 테스트를 위해 Stage 1에 아이템 임의 배치
    if (stage == 1) {
        data.items.push_back(Item(ItemType::BEER_BOTTLE, 400.0f, 400.0f));
        data.items.push_back(Item(ItemType::BUTCHER_KNIFE, 600.0f, 400.0f));
        data.items.push_back(Item(ItemType::BUST, 800.0f, 400.0f));
        data.items.push_back(Item(ItemType::POTTED_PLANT, 1000.0f, 400.0f));
        data.items.push_back(Item(ItemType::KNIFE, 1200.0f, 400.0f));
        data.items.push_back(Item(ItemType::SMOKE_BOMB, 1400.0f, 400.0f));
    }

    auto IsColorMatch = [](BYTE r, BYTE g, BYTE b, int tr, int tg, int tb) {
        return abs((int)r - tr) < 60 && abs((int)g - tg) < 60 && abs((int)b - tb) < 60;
    };

    // --- 1. Process colMap for Player Start and Clear Zones ---
// ... (lines 45-90) ...
    if (!colMap.IsNull() && colMap.IsDIBSection()) {
        int w = colMap.GetWidth();
        int h = colMap.GetHeight();
        int pitch = colMap.GetPitch();
        int bpp = colMap.GetBPP() / 8;
        BYTE* pBits = (BYTE*)colMap.GetBits();
        std::vector<bool> visited(w * h, false);

        for (int y = 0; y < h; y++) {
            BYTE* pRow = pBits + (y * pitch);
            for (int x = 0; x < w; x++) {
                if (visited[y * w + x]) continue;
                BYTE* pPixel = pRow + (x * bpp);
                BYTE b = pPixel[0], g = pPixel[1], r = pPixel[2];

                if (IsColorMatch(r, g, b, 255, 255, 255)) {
                    data.playerStart = { x, y };
                    visited[y * w + x] = true;
                }
                else if (IsColorMatch(r, g, b, 0, 255, 255)) {
                    int rectW = 0, rectH = 0;
                    while (x + rectW < w) {
                        BYTE* pN = pRow + ((x + rectW) * bpp);
                        if (IsColorMatch(pN[2], pN[1], pN[0], 0, 255, 255)) rectW++; else break;
                    }
                    while (y + rectH < h) {
                        BYTE* pNRow = pBits + ((y + rectH) * pitch) + (x * bpp);
                        if (IsColorMatch(pNRow[2], pNRow[1], pNRow[0], 0, 255, 255)) rectH++; else break;
                    }
                    if (rectW > 0 && rectH > 0) {
                        data.clearZones.push_back({ x, y, x + rectW, y + rectH });
                        for (int ry = y; ry < y + rectH; ry++) for (int rx = x; rx < x + rectW; rx++) visited[ry * w + rx] = true;
                    }
                }
            }
        }
    }

    // --- 2. Process objMap for Entities (Enemies, Doors, GlassDomes) ---
    if (!objMap.IsNull() && objMap.IsDIBSection()) {
        int w = objMap.GetWidth();
        int h = objMap.GetHeight();
        int pitch = objMap.GetPitch();
        int bpp = objMap.GetBPP() / 8;
        BYTE* pBits = (BYTE*)objMap.GetBits();
        std::vector<bool> visited(w * h, false);

        for (int y = 0; y < h; y++) {
            BYTE* pRow = pBits + (y * pitch);
            for (int x = 0; x < w; x++) {
                if (visited[y * w + x]) continue;
                BYTE* pPixel = pRow + (x * bpp);
                BYTE b = pPixel[0], g = pPixel[1], r = pPixel[2];

                // Check for Enemy Category (Yellow or White-to-handle-Pomp-marker)
                bool isEnemyCategory = IsColorMatch(r, g, b, 255, 255, 0) || IsColorMatch(r, g, b, 255, 255, 255);
                if (isEnemyCategory) {
                    if (y + 1 < h) {
                        BYTE* pSubRow = pBits + ((y + 1) * pitch) + (x * bpp);
                        BYTE sb = pSubRow[0], sg = pSubRow[1], sr = pSubRow[2];
                        int enemyType = -1;
                        if (IsColorMatch(sr, sg, sb, 255, 0, 0)) enemyType = 1;      // 빨강: Grunt
                        else if (IsColorMatch(sr, sg, sb, 0, 255, 0)) enemyType = 0; // 초록: Gangster
                        else if (IsColorMatch(sr, sg, sb, 0, 0, 255)) enemyType = 2; // 파랑: Pomp

                        if (enemyType != -1) {
                            int rectW = 0, rectH = 1;
                            while (x + rectW < w) {
                                BYTE* pN = pRow + ((x + rectW) * bpp);
                                if (IsColorMatch(pN[2], pN[1], pN[0], r, g, b)) rectW++; else break;
                            }
                            while (y + rectH < h) {
                                BYTE* pNRow = pBits + ((y + rectH) * pitch) + (x * bpp);
                                if (IsColorMatch(pNRow[2], pNRow[1], pNRow[0], sr, sg, sb)) rectH++; else break;
                            }
                            data.enemySpawns.push_back({ (float)x, (float)y, (float)rectW, enemyType });
                            for (int ry = y; ry < y + rectH; ry++) for (int rx = x; rx < x + rectW; rx++) visited[ry * w + rx] = true;
                        }
                    }
                }
                else if (IsColorMatch(r, g, b, 255, 0, 255)) { // Pink: Object
                    // 디버그: 핑크 발견
                    TCHAR buf[256];
                    wsprintf(buf, TEXT("[Debug] Found Pink at %d, %d\n"), x, y);
                    OutputDebugString(buf);

                    int rectW = 0, rectH = 1;
                    while (x + rectW < w) {
                        BYTE* pN = pRow + ((x + rectW) * bpp);
                        if (IsColorMatch(pN[2], pN[1], pN[0], 255, 0, 255)) rectW++; else break;
                    }
                    
                    bool foundType = false;
                    if (y + 1 < h) {
                        // PINK 라인 아래의 전체 너비를 검사하여 타입 확인
                        for (int rx = x; rx < x + rectW; rx++) {
                            BYTE* pSubPixel = pBits + ((y + 1) * pitch) + (rx * bpp);
                            BYTE sb2 = pSubPixel[0], sg2 = pSubPixel[1], sr2 = pSubPixel[2];
                            
                            // 디버그: 아래 픽셀 색상 출력
                            wsprintf(buf, TEXT("[Debug] Checking below Pink at %d, %d: RGB(%d, %d, %d)\n"), rx, y + 1, sr2, sg2, sb2);
                            OutputDebugString(buf);

                            if (IsColorMatch(sr2, sg2, sb2, 255, 0, 0)) { // Door (Red)
                                while (y + rectH < h) {
                                    BYTE* pNRow = pBits + ((y + rectH) * pitch) + (rx * bpp);
                                    if (IsColorMatch(pNRow[2], pNRow[1], pNRow[0], 255, 0, 0)) rectH++; else break;
                                }
                                data.doors.emplace_back((float)x, (float)y, (float)rectW, (float)rectH);
                                foundType = true; break;
                            } else if (IsColorMatch(sr2, sg2, sb2, 0, 0, 255)) { // GlassDome (Blue)
                                while (y + rectH < h) {
                                    BYTE* pNRow = pBits + ((y + rectH) * pitch) + (rx * bpp);
                                    if (IsColorMatch(pNRow[2], pNRow[1], pNRow[0], 0, 0, 255)) rectH++; else break;
                                }
                                data.glassDomes.emplace_back((float)x, (float)y, (float)rectW, (float)rectH);
                                foundType = true; break;
                            }
                        }
                    }

                    if (foundType) {
                        for (int ry = y; ry < y + rectH; ry++) for (int rx = x; rx < x + rectW; rx++) visited[ry * w + rx] = true;
                    } else {
                        // 타입을 찾지 못했더라도 해당 핑크 라인은 건너뜀
                        for (int rx = x; rx < x + rectW; rx++) visited[y * w + rx] = true;
                    }
                }
            }
        }
    }
}

bool StageManager::IsInClearZone(float x, float y, float w, float h) {
    RECT r = { (int)x, (int)y, (int)(x + w), (int)(y + h) };
    for (const auto& zone : m_clearZones) {
        RECT intersect;
        if (IntersectRect(&intersect, &r, &zone)) return true;
    }
    return false;
}

void StageManager::Reset() {
    if (m_pCurrentDoors) {
        for (auto& d : *m_pCurrentDoors) d.Reset();
    }
    if (m_pCurrentGlassDomes) {
        for (auto& gd : *m_pCurrentGlassDomes) gd.Reset();
    }
    // 아이템 초기화: 원본 데이터를 사용하여 활성 아이템 벡터 복원
    if (m_stageDataMap.count(m_currentStage)) {
        m_activeItems = m_stageDataMap[m_currentStage].items;
        m_pCurrentItems = &m_activeItems;
    }
}

void StageManager::LoadAssets(int stage) {
    m_currentStage = stage;
    if (m_mapImages.count(stage)) {
        m_imgMap = &m_mapImages[stage];
        m_imgColMap = &m_colMapImages[stage];
        m_imgObjMap = &m_objMapImages[stage];
        
        if (m_imgMap && !m_imgMap->IsNull()) {
            EffectManager::InitBloodLayer(m_imgMap->GetWidth(), m_imgMap->GetHeight());
        }

        StageData& data = m_stageDataMap[stage];
        m_pCurrentDoors = &data.doors;
        m_pCurrentGlassDomes = &data.glassDomes;
        
        // 아이템 로드 시 복사본 생성
        m_activeItems = data.items;
        m_pCurrentItems = &m_activeItems;
        
        TCHAR msg[256];
        wsprintf(msg, TEXT("[Debug] Stage %d loaded with %d doors\n"), stage, (int)m_pCurrentDoors->size());
        OutputDebugString(msg);
        
        m_playerStart = data.playerStart;
        m_clearZones = data.clearZones;
        m_stageLimitTime = data.stageLimitTime;
    }
}

void StageManager::ReleaseAssets() {
    for (auto& pair : m_mapImages) pair.second.Destroy();
    for (auto& pair : m_colMapImages) pair.second.Destroy();
    for (auto& pair : m_objMapImages) pair.second.Destroy();
    m_mapImages.clear();
    m_colMapImages.clear();
    m_objMapImages.clear();
    m_stageDataMap.clear();
    
    m_imgSkylineBlack.Destroy();
    m_imgSkylineClouds.Destroy();
    Door::ReleaseAssets();
    GlassDome::ReleaseAssets();
}

int StageManager::UpdateDoors(float playerX, float playerY, float playerW, float playerH, bool isA, bool isD, bool isAttacking, float attackHitX, float attackHitY, float attackHitW, float attackHitH, DWORD currentTime, float timeScale) {
    if (m_pCurrentDoors) {
        for (int i = 0; i < (int)m_pCurrentDoors->size(); i++) {
            Door& d = (*m_pCurrentDoors)[i];
            DoorOpenEvent de = d.Update(playerX, playerY, playerW, playerH, isA, isD, isAttacking, attackHitX, attackHitY, attackHitW, attackHitH, currentTime, timeScale);
            if (de != DoorOpenEvent::DOE_NONE) {
                return i;
            }
        }
    }
    return -1;
}

void StageManager::UpdateGlassDomes(bool isAttacking, float attackHitX, float attackHitY, float attackHitW, float attackHitH, DWORD currentTime) {
    if (m_pCurrentGlassDomes) {
        for (auto& gd : *m_pCurrentGlassDomes) {
            gd.Update(attackHitX, attackHitY, attackHitW, attackHitH, isAttacking, currentTime);
        }
    }
}

void StageManager::Render(HDC hDC, Gdiplus::Graphics* g, bool isFullMapView, bool showDebugRect, float mapScale, float renderMapScale, float mapOffsetX, float mapOffsetY, float camX, float camY, int virtualWidth, int virtualHeight, bool isSlowMo) {
    if (!hDC) return;

    if (isSlowMo) {
        HBRUSH hBlack = CreateSolidBrush(RGB(0, 0, 0));
        RECT rect = { 0, 0, virtualWidth, virtualHeight };
        FillRect(hDC, &rect, hBlack);
        DeleteObject(hBlack);
        return; 
    }

    if (!isFullMapView) {
        if (!m_imgSkylineClouds.IsNull() && (m_currentStage == 1 || m_currentStage == 2)) {
            float cloudScaleX = 3.0f, cloudScaleY = 1.5f; 
            int cW = (int)(m_imgSkylineClouds.GetWidth() * cloudScaleX), cH = (int)(m_imgSkylineClouds.GetHeight() * cloudScaleY);
            int mapW = GetMapWidth(); float maxCamX = (float)(mapW - (virtualWidth / renderMapScale)), ratioX = (maxCamX > 0) ? (camX / maxCamX) : 0, pX = -ratioX * (cW - virtualWidth);
            if (cW > 0 && cH > 0 && m_imgSkylineClouds.GetWidth() > 0 && m_imgSkylineClouds.GetHeight() > 0) {
                m_imgSkylineClouds.Draw(hDC, (int)pX, 0, cW, cH, 0, 0, m_imgSkylineClouds.GetWidth(), m_imgSkylineClouds.GetHeight());
            }
        }
        if (!m_imgSkylineBlack.IsNull() && m_currentStage == 1) {
            int mapW = GetMapWidth(); float skylineScale = 3.0f; int sW = (int)(m_imgSkylineBlack.GetWidth() * skylineScale), sH = (int)(m_imgSkylineBlack.GetHeight() * skylineScale);
            float maxCamX = (float)(mapW - (virtualWidth / renderMapScale)), ratioX = (maxCamX > 0) ? (camX / maxCamX) : 0, pX = -ratioX * (sW - virtualWidth);
            int pY = (virtualHeight / 2) - (sH / 2) - 150;
            if (sW > 0 && sH > 0 && m_imgSkylineBlack.GetWidth() > 0 && m_imgSkylineBlack.GetHeight() > 0) {
                m_imgSkylineBlack.Draw(hDC, (int)pX, pY, sW, sH, 0, 0, m_imgSkylineBlack.GetWidth(), m_imgSkylineBlack.GetHeight());
            }
        }
    }

    if (!hDC) return;
    if (m_imgMap && !m_imgMap->IsNull()) {
        CImage* tMap = showDebugRect ? m_imgColMap : m_imgMap;
        if (tMap && !tMap->IsNull()) {
            int cMW = tMap->GetWidth(), cMH = tMap->GetHeight();
            if (isFullMapView) {
                tMap->Draw(hDC, (int)mapOffsetX, (int)mapOffsetY, (int)(cMW * renderMapScale), (int)(cMH * renderMapScale), 0, 0, cMW, cMH);
            }
            else {
                float currentOffset = 0.0f;
                for (auto& pair : m_stageDataMap) {
                    if (&m_mapImages[pair.first] == m_imgMap) {
                        currentOffset = pair.second.mapRenderOffsetY;
                        break;
                    }
                }

                float vW = virtualWidth / renderMapScale, vH = (virtualHeight - currentOffset) / renderMapScale;
                int drawCamY = (int)camY % cMH; if (drawCamY < 0) drawCamY += cMH;
                
                int destY = (int)currentOffset;
                int destH = virtualHeight - destY;

                if (drawCamY + vH <= cMH) { 
                    if (virtualWidth > 0 && destH > 0 && vW > 0 && vH > 0) {
                        tMap->Draw(hDC, 0, destY, virtualWidth, destH, (int)camX, drawCamY, (int)vW, (int)vH); 
                    }
                }
                else {
                    int firstPartH = cMH - drawCamY, firstPartDrawH = (int)(firstPartH * (destH / vH));
                    if (virtualWidth > 0 && firstPartDrawH > 0 && vW > 0 && firstPartH > 0) {
                        tMap->Draw(hDC, 0, destY, virtualWidth, firstPartDrawH, (int)camX, drawCamY, (int)vW, firstPartH);
                    }
                    int secondPartDrawH = destH - firstPartDrawH, secondPartSrcH = (int)vH - firstPartH;
                    if (virtualWidth > 0 && secondPartDrawH > 0 && vW > 0 && secondPartSrcH > 0) {
                        tMap->Draw(hDC, 0, destY + firstPartDrawH, virtualWidth, secondPartDrawH, (int)camX, 0, (int)vW, secondPartSrcH);
                    }
                }
            }
        }
    }

    if (!isFullMapView) {
        EffectManager::RenderMapBlood(hDC, g, camX, camY, renderMapScale);
    }

    if (m_pCurrentGlassDomes) {
        for (auto& gd : *m_pCurrentGlassDomes) {
            gd.Render(hDC, camX, camY, mapScale, showDebugRect);
        }
    }

    if (m_pCurrentDoors) {
        TCHAR msg[256];
        wsprintf(msg, TEXT("[Debug] Rendering %d doors\n"), (int)m_pCurrentDoors->size());
        OutputDebugString(msg);
        
        for (auto& d : *m_pCurrentDoors) {
            float cFS = 0, cFX = 0, cFY = 0;
            if (isFullMapView) {
                int mapW = GetMapWidth(), mapH = GetMapHeight();
                cFS = (std::min)((float)virtualWidth / mapW, (float)virtualHeight / mapH);
                cFX = (virtualWidth - mapW * cFS) / 2.0f; cFY = (virtualHeight - mapH * cFS) / 2.0f;
            }
            d.Render(hDC, camX, camY, mapScale, isFullMapView, cFS, cFX, cFY);
        }
    } else {
        OutputDebugString(TEXT("[Debug] m_pCurrentDoors is NULL\n"));
    }

    if (m_pCurrentItems) {
        for (auto& item : *m_pCurrentItems) {
            item.Render(hDC, g, camX, camY, mapScale);
        }
    }
}

void StageManager::UpdateItems(float ts, Player& player) {
    if (m_pCurrentItems != nullptr) {
        for (auto& item : *m_pCurrentItems) {
            item.Update(ts, player, m_currentEnemies);
        }
    }
}