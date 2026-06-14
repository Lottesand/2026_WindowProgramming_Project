#include "StageManager.h"
#include "../Effects/EffectManager.h"
#include <algorithm>
#include <atomic>

// ==========================================
// [ITEM SPAWN COORDINATES]
// ==========================================

// Stage 1
float S1_BEER_X = 300.0f,  S1_BEER_Y = 400.0f;
float S1_KNIFE_X = 700.0f, S1_KNIFE_Y = 400.0f;
float S1_VIAL_X = 500.0f,  S1_VIAL_Y = 400.0f;
float S1_FLAME_X = 150.0f, S1_FLAME_Y = 400.0f; 

// Stage 2
float S2_BUST_X = 570.0f,    S2_BUST_Y = 640.0f;
float S2_BUTCHER_X = 700.0f, S2_BUTCHER_Y = 990.0f;

// Stage 3
float S3_PLANT_X = 600.0f, S3_PLANT_Y = 1010.0f;

// ==========================================

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
std::vector<OilDrum>* StageManager::m_pCurrentOilDrums = nullptr;
POINT StageManager::m_playerStart = { 0, 0 };
std::vector<RECT> StageManager::m_clearZones;
float StageManager::m_stageLimitTime = 0.0f;
int StageManager::m_currentStage = 1;

void StageManager::Init() {
    EffectManager::Init();
    OilDrum::LoadAssets();
}

void StageManager::PopulateDynamicObjects(int stage, StageData& data) {
    data.items.clear();
    data.oilDrums.clear();

    if (stage == 1) {
        data.items.push_back(Item(ItemType::BEER_BOTTLE, S1_BEER_X, S1_BEER_Y));
        data.items.push_back(Item(ItemType::KNIFE, S1_KNIFE_X, S1_KNIFE_Y));
        data.items.push_back(Item(ItemType::EXPLOSIVE_VIAL, S1_VIAL_X, S1_VIAL_Y));
        data.items.push_back(Item(ItemType::FLAMETHROWER, S1_FLAME_X, S1_FLAME_Y));
        data.items.push_back(Item(ItemType::SMOKE_BOMB, 1400.0f, 400.0f));

        data.oilDrums.emplace_back(800.0f, 435.0f);
        data.oilDrums.emplace_back(840.0f, 435.0f);
        data.oilDrums.emplace_back(880.0f, 435.0f);
        data.oilDrums.emplace_back(1100.0f, 435.0f);
        data.oilDrums.emplace_back(1140.0f, 435.0f);
    }
    else if (stage == 2) {
        data.items.push_back(Item(ItemType::BUST, S2_BUST_X, S2_BUST_Y));
        data.items.push_back(Item(ItemType::BUTCHER_KNIFE, S2_BUTCHER_X, S2_BUTCHER_Y));
    }
    else if (stage == 3) {
        data.items.push_back(Item(ItemType::POTTED_PLANT, S3_PLANT_X, S3_PLANT_Y));
    }
}

void StageManager::ProcessStage(int stage) {
    StageData& data = m_stageDataMap[stage];
    CImage& colMap = m_colMapImages[stage];
    CImage& objMap = m_objMapImages[stage];

    data.doors.clear();
    data.glassDomes.clear();
    data.clearZones.clear();
    data.enemySpawns.clear();
    data.playerStart = { 100, 100 };
    data.stageLimitTime = (stage == 1) ? 30.0f : 60.0f;

    PopulateDynamicObjects(stage, data);

    auto IsColorMatch = [](BYTE r, BYTE g, BYTE b, int tr, int tg, int tb) {
        return abs((int)r - tr) < 60 && abs((int)g - tg) < 60 && abs((int)b - tb) < 60;
    };

    if (!colMap.IsNull() && colMap.IsDIBSection()) {
        int w = colMap.GetWidth(), h = colMap.GetHeight(), pitch = colMap.GetPitch(), bpp = colMap.GetBPP() / 8;
        BYTE* pBits = (BYTE*)colMap.GetBits(); std::vector<bool> visited(w * h, false);
        for (int y = 0; y < h; y++) {
            BYTE* pRow = pBits + (y * pitch);
            for (int x = 0; x < w; x++) {
                if (visited[y * w + x]) continue;
                BYTE* pPixel = pRow + (x * bpp); BYTE b = pPixel[0], g = pPixel[1], r = pPixel[2];
                if (IsColorMatch(r, g, b, 255, 255, 255)) { data.playerStart = { x, y }; visited[y * w + x] = true; }
                else if (IsColorMatch(r, g, b, 0, 255, 255)) {
                    int rectW = 0, rectH = 0;
                    while (x + rectW < w) { BYTE* pN = pRow + ((x + rectW) * bpp); if (IsColorMatch(pN[2], pN[1], pN[0], 0, 255, 255)) rectW++; else break; }
                    while (y + rectH < h) { BYTE* pNRow = pBits + ((y + rectH) * pitch) + (x * bpp); if (IsColorMatch(pNRow[2], pNRow[1], pNRow[0], 0, 255, 255)) rectH++; else break; }
                    if (rectW > 0 && rectH > 0) {
                        data.clearZones.push_back({ x, y, x + rectW, y + rectH });
                        for (int ry = y; ry < y + rectH; ry++) for (int rx = x; rx < x + rectW; rx++) visited[ry * w + rx] = true;
                    }
                }
            }
        }
    }

    if (!objMap.IsNull() && objMap.IsDIBSection()) {
        int w = objMap.GetWidth(), h = objMap.GetHeight(), pitch = objMap.GetPitch(), bpp = objMap.GetBPP() / 8;
        BYTE* pBits = (BYTE*)objMap.GetBits(); std::vector<bool> visited(w * h, false);
        for (int y = 0; y < h; y++) {
            BYTE* pRow = pBits + (y * pitch);
            for (int x = 0; x < w; x++) {
                if (visited[y * w + x]) continue;
                BYTE* pPixel = pRow + (x * bpp); BYTE b = pPixel[0], g = pPixel[1], r = pPixel[2];
                bool isEnemyCategory = IsColorMatch(r, g, b, 255, 255, 0) || IsColorMatch(r, g, b, 255, 255, 255);
                if (isEnemyCategory && y + 1 < h) {
                    BYTE* pSubRow = pBits + ((y + 1) * pitch) + (x * bpp); BYTE sb = pSubRow[0], sg = pSubRow[1], sr = pSubRow[2];
                    int enemyType = -1;
                    if (IsColorMatch(sr, sg, sb, 255, 0, 0)) enemyType = 1; else if (IsColorMatch(sr, sg, sb, 0, 255, 0)) enemyType = 0;
                    else if (IsColorMatch(sr, sg, sb, 0, 0, 255)) enemyType = 2; else if (IsColorMatch(sr, sg, sb, 128, 0, 128)) enemyType = 3;
                    if (enemyType != -1) {
                        int rectW = 0, rectH = 1;
                        while (x + rectW < w) { BYTE* pN = pRow + ((x + rectW) * bpp); if (IsColorMatch(pN[2], pN[1], pN[0], r, g, b)) rectW++; else break; }
                        while (y + rectH < h) { BYTE* pNRow = pBits + ((y + rectH) * pitch) + (x * bpp); if (IsColorMatch(pNRow[2], pNRow[1], pNRow[0], sr, sg, sb)) rectH++; else break; }
                        data.enemySpawns.push_back({ (float)x, (float)y, (float)rectW, enemyType });
                        for (int ry = y; ry < y + rectH; ry++) for (int rx = x; rx < x + rectW; rx++) visited[ry * w + rx] = true;
                    }
                }
                else if (IsColorMatch(r, g, b, 255, 0, 255)) {
                    int rectW = 0, rectH = 1; while (x + rectW < w) { BYTE* pN = pRow + ((x + rectW) * bpp); if (IsColorMatch(pN[2], pN[1], pN[0], 255, 0, 255)) rectW++; else break; }
                    bool foundType = false;
                    if (y + 1 < h) {
                        for (int rx = x; rx < x + rectW; rx++) {
                            BYTE* pSubPixel = pBits + ((y + 1) * pitch) + (rx * bpp); BYTE sb2 = pSubPixel[0], sg2 = pSubPixel[1], sr2 = pSubPixel[2];
                            if (IsColorMatch(sr2, sg2, sb2, 255, 0, 0)) {
                                while (y + rectH < h) { BYTE* pNRow = pBits + ((y + rectH) * pitch) + (rx * bpp); if (IsColorMatch(pNRow[2], pNRow[1], pNRow[0], 255, 0, 0)) rectH++; else break; }
                                data.doors.emplace_back((float)x, (float)y, (float)rectW, (float)rectH); foundType = true; break;
                            } else if (IsColorMatch(sr2, sg2, sb2, 0, 0, 255)) {
                                while (y + rectH < h) { BYTE* pNRow = pBits + ((y + rectH) * pitch) + (rx * bpp); if (IsColorMatch(pNRow[2], pNRow[1], pNRow[0], 0, 0, 255)) rectH++; else break; }
                                data.glassDomes.emplace_back((float)x, (float)y, (float)rectW, (float)rectH); foundType = true; break;
                            }
                        }
                    }
                    if (foundType) for (int ry = y; ry < y + rectH; ry++) for (int rx = x; rx < x + rectW; rx++) visited[ry * w + rx] = true;
                    else for (int rx = x; rx < x + rectW; rx++) visited[y * w + rx] = true;
                }
            }
        }
    }
}

bool StageManager::IsInClearZone(float x, float y, float w, float h) {
    RECT r = { (int)x, (int)y, (int)(x + w), (int)(y + h) };
    for (const auto& zone : m_clearZones) { RECT intersect; if (IntersectRect(&intersect, &r, &zone)) return true; }
    return false;
}

void StageManager::Reset() { ProcessStage(m_currentStage); }

void StageManager::SoftReset() {
    if (m_pCurrentDoors) for (auto& d : *m_pCurrentDoors) d.Reset();
    if (m_pCurrentGlassDomes) for (auto& gd : *m_pCurrentGlassDomes) gd.Reset();
    if (m_stageDataMap.count(m_currentStage)) {
        PopulateDynamicObjects(m_currentStage, m_stageDataMap[m_currentStage]);
    }
}

void StageManager::LoadAllStages(std::atomic<int>* pProgress) {
    for (int i = 1; i <= 5; ++i) {
        TCHAR mapPath[256], colPath[256], objPath[256];
        wsprintf(mapPath, TEXT("assets/stage%d/map_stage%d.png"), i, i);
        wsprintf(colPath, TEXT("assets/stage%d/colmap_stage%d.png"), i, i);
        wsprintf(objPath, TEXT("assets/stage%d/objmap_stage%d.png"), i, i);
        m_mapImages[i].Load(mapPath); m_colMapImages[i].Load(colPath); m_objMapImages[i].Load(objPath);
        ProcessStage(i);
        if (i >= 2 && i <= 5) { StageData& data = m_stageDataMap[i]; data.camFixedY = -1.0f; data.mapScale = 1.1f; data.mapRenderOffsetY = 0.0f; }
        if (pProgress) { *pProgress = 60 + (i * 5); Sleep(50); }
    }
    if (m_imgSkylineBlack.IsNull()) m_imgSkylineBlack.Load(TEXT("assets/spr_skyline_black.png"));
    if (m_imgSkylineClouds.IsNull()) m_imgSkylineClouds.Load(TEXT("assets/spr_skyline_clouds.png"));
    if (pProgress) { *pProgress = 85; Sleep(100); }
    Door::LoadAssets(); GlassDome::LoadAssets(); OilDrum::LoadAssets();
    if (pProgress) { *pProgress = 90; Sleep(100); }
}

void StageManager::LoadAssets(int stage) {
    m_currentStage = stage;
    if (m_mapImages.count(stage)) {
        m_imgMap = &m_mapImages[stage]; m_imgColMap = &m_colMapImages[stage]; m_imgObjMap = &m_objMapImages[stage];
        if (m_imgMap && !m_imgMap->IsNull()) EffectManager::InitBloodLayer(m_imgMap->GetWidth(), m_imgMap->GetHeight());
        StageData& data = m_stageDataMap[stage]; 
        m_pCurrentDoors = &data.doors; 
        m_pCurrentGlassDomes = &data.glassDomes; 
        m_pCurrentItems = &data.items; 
        m_pCurrentOilDrums = &data.oilDrums;
        m_playerStart = data.playerStart; 
        m_clearZones = data.clearZones; 
        m_stageLimitTime = data.stageLimitTime;
    }
}

void StageManager::ReleaseAssets() {
    for (auto& pair : m_mapImages) pair.second.Destroy();
    for (auto& pair : m_colMapImages) pair.second.Destroy();
    for (auto& pair : m_objMapImages) pair.second.Destroy();
    m_mapImages.clear(); m_colMapImages.clear(); m_objMapImages.clear(); m_stageDataMap.clear();
    m_imgSkylineBlack.Destroy(); m_imgSkylineClouds.Destroy(); 
    Door::ReleaseAssets(); GlassDome::ReleaseAssets(); OilDrum::ReleaseAssets();
}

int StageManager::UpdateDoors(float playerX, float playerY, float playerW, float playerH, bool isA, bool isD, bool isAttacking, float attackHitX, float attackHitY, float attackHitW, float attackHitH, DWORD currentTime, float timeScale) {
    if (m_pCurrentDoors) { for (int i = 0; i < (int)m_pCurrentDoors->size(); i++) { DoorOpenEvent de = (*m_pCurrentDoors)[i].Update(playerX, playerY, playerW, playerH, isA, isD, isAttacking, attackHitX, attackHitY, attackHitW, attackHitH, currentTime, timeScale); if (de != DoorOpenEvent::DOE_NONE) return i; } }
    return -1;
}

void StageManager::UpdateGlassDomes(bool isAttacking, float attackHitX, float attackHitY, float attackHitW, float attackHitH, DWORD currentTime) {
    if (m_pCurrentGlassDomes) for (auto& gd : *m_pCurrentGlassDomes) gd.Update(attackHitX, attackHitY, attackHitW, attackHitH, isAttacking, currentTime);
}

void StageManager::UpdateOilDrums(float ts, const std::vector<class Enemy*>& enemies, Player* player) {
    if (m_pCurrentOilDrums) {
        for (auto& drum : *m_pCurrentOilDrums) drum.Update(ts, enemies, *m_pCurrentOilDrums, player);
    }
}

void StageManager::Render(HDC hDC, Gdiplus::Graphics* g, bool isFullMapView, bool showDebugRect, float mapScale, float renderMapScale, float mapOffsetX, float mapOffsetY, float camX, float camY, int virtualWidth, int virtualHeight, bool isSlowMo) {
    if (!hDC) return;
    if (isSlowMo) { HBRUSH hBlack = CreateSolidBrush(RGB(0, 0, 0)); RECT rect = { 0, 0, virtualWidth, virtualHeight }; FillRect(hDC, &rect, hBlack); DeleteObject(hBlack); return; }
    if (!isFullMapView) {
        if (!m_imgSkylineClouds.IsNull() && (m_currentStage == 1 || m_currentStage == 2)) {
            float cloudScaleX = 3.0f, cloudScaleY = 1.5f; int cW = (int)(m_imgSkylineClouds.GetWidth() * cloudScaleX), cH = (int)(m_imgSkylineClouds.GetHeight() * cloudScaleY);
            int mapW = GetMapWidth(); float maxCamX = (float)(mapW - (virtualWidth / renderMapScale)), ratioX = (maxCamX > 0) ? (camX / maxCamX) : 0, pX = -ratioX * (cW - virtualWidth);
            if (cW > 0 && cH > 0) m_imgSkylineClouds.Draw(hDC, (int)pX, 0, cW, cH, 0, 0, m_imgSkylineClouds.GetWidth(), m_imgSkylineClouds.GetHeight());
        }
        if (!m_imgSkylineBlack.IsNull() && m_currentStage == 1) {
            int mapW = GetMapWidth(); float skylineScale = 3.0f; int sW = (int)(m_imgSkylineBlack.GetWidth() * skylineScale), sH = (int)(m_imgSkylineBlack.GetHeight() * skylineScale);
            float maxCamX = (float)(mapW - (virtualWidth / renderMapScale)), ratioX = (maxCamX > 0) ? (camX / maxCamX) : 0, pX = -ratioX * (sW - virtualWidth);
            int pY = (virtualHeight / 2) - (sH / 2) - 150;
            if (sW > 0 && sH > 0) m_imgSkylineBlack.Draw(hDC, (int)pX, pY, sW, sH, 0, 0, m_imgSkylineBlack.GetWidth(), m_imgSkylineBlack.GetHeight());
        }
    }
    if (m_imgMap && !m_imgMap->IsNull()) {
        CImage* tMap = showDebugRect ? m_imgColMap : m_imgMap;
        if (tMap && !tMap->IsNull()) {
            int cMW = tMap->GetWidth(), cMH = tMap->GetHeight();
            if (isFullMapView) tMap->Draw(hDC, (int)mapOffsetX, (int)mapOffsetY, (int)(cMW * renderMapScale), (int)(cMH * renderMapScale), 0, 0, cMW, cMH);
            else {
                float currentOffset = 0.0f;
                for (auto& pair : m_stageDataMap) { if (&m_mapImages[pair.first] == m_imgMap) { currentOffset = pair.second.mapRenderOffsetY; break; } }
                float vW = virtualWidth / renderMapScale, vH = (virtualHeight - currentOffset) / renderMapScale;
                int drawCamY = (int)camY % cMH; if (drawCamY < 0) drawCamY += cMH;
                int destY = (int)currentOffset, destH = virtualHeight - destY;
                if (drawCamY + vH <= cMH) { if (virtualWidth > 0 && destH > 0 && vW > 0 && vH > 0) tMap->Draw(hDC, 0, destY, virtualWidth, destH, (int)camX, drawCamY, (int)vW, (int)vH); }
                else {
                    int firstPartH = cMH - drawCamY, firstPartDrawH = (int)(firstPartH * (destH / vH));
                    if (virtualWidth > 0 && firstPartDrawH > 0 && vW > 0 && firstPartH > 0) tMap->Draw(hDC, 0, destY, virtualWidth, firstPartDrawH, (int)camX, drawCamY, (int)vW, firstPartH);
                    int secondPartDrawH = destH - firstPartDrawH, secondPartSrcH = (int)vH - firstPartH;
                    if (virtualWidth > 0 && secondPartDrawH > 0 && vW > 0 && secondPartSrcH > 0) tMap->Draw(hDC, 0, destY + firstPartDrawH, virtualWidth, secondPartDrawH, (int)camX, 0, (int)vW, secondPartSrcH);
                }
            }
        }
    }
    if (!isFullMapView) EffectManager::RenderMapBlood(hDC, g, camX, camY, renderMapScale);
    if (m_pCurrentGlassDomes) for (auto& gd : *m_pCurrentGlassDomes) gd.Render(hDC, camX, camY, mapScale, showDebugRect);
    if (m_pCurrentDoors) {
        for (auto& d : *m_pCurrentDoors) {
            float dcFS = 0, dcFX = 0, dcFY = 0;
            if (isFullMapView) {
                int mapW = GetMapWidth(), mapH = GetMapHeight();
                dcFS = (std::min)((float)virtualWidth / mapW, (float)virtualHeight / mapH);
                dcFX = (virtualWidth - mapW * dcFS) / 2.0f; dcFY = (virtualHeight - mapH * dcFS) / 2.0f;
            }
            d.Render(hDC, camX, camY, mapScale, isFullMapView, dcFS, dcFX, dcFY);
        }
    }
    if (m_pCurrentOilDrums) for (auto& drum : *m_pCurrentOilDrums) drum.Render(hDC, g, camX, camY, mapScale);
    if (m_pCurrentItems) for (auto& item : *m_pCurrentItems) item.Render(hDC, g, camX, camY, mapScale);
}

void StageManager::UpdateItems(float ts, Player& player, const std::vector<class Enemy*>& enemies) {
    if (m_pCurrentItems != nullptr) for (auto& item : *m_pCurrentItems) item.Update(ts, player, enemies);
}
