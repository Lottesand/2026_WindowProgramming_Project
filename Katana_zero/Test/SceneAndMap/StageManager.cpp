#include "StageManager.h"
#include <algorithm>

CImage StageManager::m_imgMap;
CImage StageManager::m_imgColMap;
CImage StageManager::m_imgSkylineBlack;
CImage StageManager::m_imgSkylineClouds;
std::vector<Door> StageManager::m_doors;
POINT StageManager::m_playerStart = { 0, 0 };
std::vector<RECT> StageManager::m_clearZones;
float StageManager::m_stageLimitTime = 60.0f;

void StageManager::Init() {
    m_doors.clear();
    m_clearZones.clear();
    m_playerStart = { 100, 100 }; 
    
    if (!m_imgColMap.IsNull()) {
        int w = m_imgColMap.GetWidth();
        int h = m_imgColMap.GetHeight();

        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                COLORREF color = m_imgColMap.GetPixel(x, y);
                BYTE r = GetRValue(color);
                BYTE g = GetGValue(color);
                BYTE b = GetBValue(color);

                if (r == 255 && g == 0 && b == 255) {
                    bool alreadyCovered = false;
                    for (const auto& d : m_doors) {
                        if (x >= d.GetX() && x < d.GetX() + d.GetW() &&
                            y >= d.GetY() && y < d.GetY() + d.GetH()) {
                            alreadyCovered = true;
                            break;
                        }
                    }

                    if (!alreadyCovered) {
                        int rectW = 0, rectH = 0;
                        while (x + rectW < w && (m_imgColMap.GetPixel(x + rectW, y) & 0x00FFFFFF) == 0x00FF00FF) rectW++;
                        while (y + rectH < h && (m_imgColMap.GetPixel(x, y + rectH) & 0x00FFFFFF) == 0x00FF00FF) rectH++;
                        if (rectW > 0 && rectH > 0) m_doors.emplace_back((float)x, (float)y, (float)rectW, (float)rectH);
                    }
                }
                // White (255, 255, 255) - Player Start
                else if (r == 255 && g == 255 && b == 255) {
                    if (m_playerStart.x == 100 && m_playerStart.y == 100) { // Only set if still default
                        m_playerStart = { x, y };
                    }
                }
                // Cyan (0, 255, 255) - Clear Zone
                else if (r == 0 && g == 255 && b == 255) {
                    bool alreadyCovered = false;
                    for (const auto& rect : m_clearZones) {
                        if (x >= rect.left && x < rect.right && y >= rect.top && y < rect.bottom) {
                            alreadyCovered = true;
                            break;
                        }
                    }
                    if (!alreadyCovered) {
                        int rectW = 0, rectH = 0;
                        while (x + rectW < w && (m_imgColMap.GetPixel(x + rectW, y) & 0x00FFFFFF) == 0x00FFFF00) rectW++;
                        while (y + rectH < h && (m_imgColMap.GetPixel(x, y + rectH) & 0x00FFFFFF) == 0x00FFFF00) rectH++;
                        if (rectW > 0 && rectH > 0) m_clearZones.push_back({ x, y, x + rectW, y + rectH });
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
    for (auto& d : m_doors) {
        d.Reset();
    }
}

void StageManager::LoadAssets(int stage) {
    TCHAR mapPath[256], colPath[256];
    wsprintf(mapPath, TEXT("assets/stage%d/map_stage%d.png"), stage, stage);
    wsprintf(colPath, TEXT("assets/stage%d/colmap_stage%d.png"), stage, stage);

    m_imgMap.Destroy();
    m_imgColMap.Destroy();
    
    m_imgMap.Load(mapPath);
    m_imgColMap.Load(colPath);
    
    // 스테이지별 제한 시간 설정
    if (stage == 1) m_stageLimitTime = 30.0f; // 1스테이지 30초
    else m_stageLimitTime = 60.0f;

    if (m_imgSkylineBlack.IsNull()) m_imgSkylineBlack.Load(TEXT("assets/spr_skyline_black.png"));
    if (m_imgSkylineClouds.IsNull()) m_imgSkylineClouds.Load(TEXT("assets/spr_skyline_clouds.png"));
    Door::LoadAssets();
}

void StageManager::ReleaseAssets() {
    m_imgMap.Destroy();
    m_imgColMap.Destroy();
    m_imgSkylineBlack.Destroy();
    m_imgSkylineClouds.Destroy();
    Door::ReleaseAssets();
}

DoorOpenEvent StageManager::UpdateDoors(float playerX, float playerY, float playerW, float playerH, bool isA, bool isD, bool isAttacking, float attackHitX, float attackHitY, float attackHitW, float attackHitH, DWORD currentTime, float timeScale) {
    DoorOpenEvent result = DoorOpenEvent::NONE;
    for (auto& d : m_doors) {
        DoorOpenEvent e = d.Update(playerX, playerY, playerW, playerH, isA, isD, isAttacking, attackHitX, attackHitY, attackHitW, attackHitH, currentTime, timeScale);
        if (e != DoorOpenEvent::NONE) result = e;
    }
    return result;
}

void StageManager::Render(HDC hDC, bool isFullMapView, bool showDebugRect, float mapScale, float renderMapScale, float mapOffsetX, float mapOffsetY, float camX, float camY, int virtualWidth, int virtualHeight) {
    if (!isFullMapView) {
        // --- 1. Cloud Layer ---
        if (!m_imgSkylineClouds.IsNull()) {
            float cloudScaleX = 3.0f;
            float cloudScaleY = 1.5f; 
            int cW = (int)(m_imgSkylineClouds.GetWidth() * cloudScaleX);
            int cH = (int)(m_imgSkylineClouds.GetHeight() * cloudScaleY);
            int mapW = GetMapWidth();
            float maxCamX = (float)(mapW - (virtualWidth / renderMapScale));
            float ratioX = (maxCamX > 0) ? (camX / maxCamX) : 0;
            float pX = -ratioX * (cW - virtualWidth);
            m_imgSkylineClouds.Draw(hDC, (int)pX, 0, cW, cH, 0, 0, m_imgSkylineClouds.GetWidth(), m_imgSkylineClouds.GetHeight());
        }

        // --- 2. Skyline Black Layer ---
        if (!m_imgSkylineBlack.IsNull()) {
            int mapW = GetMapWidth();
            float skylineScale = 3.0f; 
            int sW = (int)(m_imgSkylineBlack.GetWidth() * skylineScale);
            int sH = (int)(m_imgSkylineBlack.GetHeight() * skylineScale);
            float maxCamX = (float)(mapW - (virtualWidth / renderMapScale));
            float ratioX = (maxCamX > 0) ? (camX / maxCamX) : 0;
            float pX = -ratioX * (sW - virtualWidth);
            int pY = (virtualHeight / 2) - (sH / 2) - 150;
            m_imgSkylineBlack.Draw(hDC, (int)pX, pY, sW, sH, 0, 0, m_imgSkylineBlack.GetWidth(), m_imgSkylineBlack.GetHeight());
        }
    }

    // --- 3. Map Layer ---
    if (!hDC) return;
    if (!m_imgMap.IsNull()) {
        CImage* tMap = showDebugRect ? &m_imgColMap : &m_imgMap;
        if (tMap && !tMap->IsNull()) {
            int cMW = tMap->GetWidth();
            int cMH = tMap->GetHeight();
            if (isFullMapView) {
                tMap->Draw(hDC, (int)mapOffsetX, (int)mapOffsetY, (int)(cMW * renderMapScale), (int)(cMH * renderMapScale), 0, 0, cMW, cMH);
            } else {
                if (camX >= 0 && camY >= 0 && camX + (virtualWidth / renderMapScale) <= cMW && camY + (virtualHeight / renderMapScale) <= cMH) {
                    tMap->Draw(hDC, 0, 0, virtualWidth, virtualHeight, (int)camX, (int)camY, (int)(virtualWidth / renderMapScale), (int)(virtualHeight / renderMapScale));
                } else {
                    tMap->Draw(hDC, 0, 0, virtualWidth, virtualHeight, (int)camX, (int)camY, (int)(virtualWidth / renderMapScale), (int)(virtualHeight / renderMapScale));
                }
            }
        }
    }


    // --- 4. Doors ---
    for (auto& d : m_doors) {
        float cFS = 0, cFX = 0, cFY = 0;
        if (isFullMapView) {
            int mapW = GetMapWidth();
            int mapH = GetMapHeight();
            cFS = (std::min)((float)virtualWidth / mapW, (float)virtualHeight / mapH);
            cFX = (virtualWidth - mapW * cFS) / 2.0f;
            cFY = (virtualHeight - mapH * cFS) / 2.0f;
        }
        d.Render(hDC, camX, camY, mapScale, isFullMapView, cFS, cFX, cFY);
    }
}

