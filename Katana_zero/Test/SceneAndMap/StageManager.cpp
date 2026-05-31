#include "StageManager.h"
#include <algorithm>

CImage StageManager::m_imgMap;
CImage StageManager::m_imgColMap;
CImage StageManager::m_imgSkylineBlack;
CImage StageManager::m_imgSkylineClouds;
std::vector<Door> StageManager::m_doors;

void StageManager::Init() {
    m_doors.clear();
    
    if (!m_imgColMap.IsNull()) {
        int w = m_imgColMap.GetWidth();
        int h = m_imgColMap.GetHeight();

        // ?묓겕??R:255, G:0, B:255) ?곸뿭 ?ㅼ틪
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                COLORREF color = m_imgColMap.GetPixel(x, y);
                if (GetRValue(color) == 255 && GetGValue(color) == 0 && GetBValue(color) == 255) {
                    // ?대? ?대떦 ?꾩튂媛 湲곗〈 臾??곸뿭???ы븿?섎뒗吏 ?뺤씤
                    bool alreadyCovered = false;
                    for (const auto& d : m_doors) {
                        if (x >= d.GetX() && x < d.GetX() + d.GetW() &&
                            y >= d.GetY() && y < d.GetY() + d.GetH()) {
                            alreadyCovered = true;
                            break;
                        }
                    }

                    if (!alreadyCovered) {
                        // ?덈줈???묓겕 ?곸뿭 諛쒓껄 - 媛濡??몃줈 ?ш린 痢≪젙
                        int rectW = 0;
                        int rectH = 0;

                        // 媛濡??ш린 痢≪젙
                        while (x + rectW < w) {
                            COLORREF c = m_imgColMap.GetPixel(x + rectW, y);
                            if (GetRValue(c) == 255 && GetGValue(c) == 0 && GetBValue(c) == 255) rectW++;
                            else break;
                        }

                        // ?몃줈 ?ш린 痢≪젙
                        while (y + rectH < h) {
                            COLORREF c = m_imgColMap.GetPixel(x, y + rectH);
                            if (GetRValue(c) == 255 && GetGValue(c) == 0 && GetBValue(c) == 255) rectH++;
                            else break;
                        }

                        if (rectW > 0 && rectH > 0) {
                            m_doors.emplace_back((float)x, (float)y, (float)rectW, (float)rectH);
                        }
                    }
                }
            }
        }
    }
}

void StageManager::Reset() {
    for (auto& d : m_doors) {
        d.Reset();
    }
}

void StageManager::LoadAssets() {
    m_imgMap.Load(TEXT("assets/map.png"));
    m_imgColMap.Load(TEXT("assets/colmap.png"));
    m_imgSkylineBlack.Load(TEXT("assets/spr_skyline_black.png"));
    m_imgSkylineClouds.Load(TEXT("assets/spr_skyline_clouds.png"));
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
                tMap->Draw(hDC, 0, 0, virtualWidth, virtualHeight, (int)camX, (int)camY, (int)(virtualWidth / renderMapScale), (int)(virtualHeight / renderMapScale));
            }
        }
    }


    // --- 4. Doors ---
    for (auto& d : m_doors) {
        float cFS = 0, cFX = 0, cFY = 0; // FullMapView 怨꾩궛??(Game ?대옒?ㅼ뿉???섍꺼諛쏆븘???섏?留??쇰떒 StageManager???꾩슂??媛믩뱾 ?덉쓬)
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

