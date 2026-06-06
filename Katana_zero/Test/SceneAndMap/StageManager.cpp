#include "StageManager.h"
#include <algorithm>
#include <atomic>

std::map<int, CImage> StageManager::m_mapImages;
std::map<int, CImage> StageManager::m_colMapImages;
std::map<int, StageManager::StageData> StageManager::m_stageDataMap;

CImage* StageManager::m_imgMap = nullptr;
CImage* StageManager::m_imgColMap = nullptr;

CImage StageManager::m_imgSkylineBlack;
CImage StageManager::m_imgSkylineClouds;
std::vector<Door>* StageManager::m_pCurrentDoors = nullptr;
POINT StageManager::m_playerStart = { 0, 0 };
std::vector<RECT> StageManager::m_clearZones;
float StageManager::m_stageLimitTime = 60.0f;

void StageManager::Init() {
}

void StageManager::ProcessStage(int stage) {
    StageData& data = m_stageDataMap[stage];
    CImage& colMap = m_colMapImages[stage];

    data.doors.clear();
    data.clearZones.clear();
    data.playerStart = { 100, 100 };
    data.stageLimitTime = (stage == 1) ? 30.0f : 60.0f;

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
                BYTE b = pPixel[0];
                BYTE g = pPixel[1];
                BYTE r = pPixel[2];

                if (r == 255 && g == 0 && b == 255) {
                    int rectW = 0, rectH = 0;
                    while (x + rectW < w) {
                        BYTE* pNext = pRow + ((x + rectW) * bpp);
                        if (pNext[2] == 255 && pNext[1] == 0 && pNext[0] == 255) rectW++;
                        else break;
                    }
                    while (y + rectH < h) {
                        BYTE* pNextRow = pBits + ((y + rectH) * pitch) + (x * bpp);
                        if (pNextRow[2] == 255 && pNextRow[1] == 0 && pNextRow[0] == 255) rectH++;
                        else break;
                    }

                    if (rectW > 0 && rectH > 0) {
                        data.doors.emplace_back((float)x, (float)y, (float)rectW, (float)rectH);
                        for (int ry = y; ry < y + rectH; ry++) {
                            for (int rx = x; rx < x + rectW; rx++) {
                                visited[ry * w + rx] = true;
                            }
                        }
                    }
                }
                else if (r == 255 && g == 255 && b == 255) {
                    data.playerStart = { x, y };
                    visited[y * w + x] = true;
                }
                else if (r == 0 && g == 255 && b == 255) {
                    int rectW = 0, rectH = 0;
                    while (x + rectW < w) {
                        BYTE* pNext = pRow + ((x + rectW) * bpp);
                        if (pNext[2] == 0 && pNext[1] == 255 && pNext[0] == 255) rectW++;
                        else break;
                    }
                    while (y + rectH < h) {
                        BYTE* pNextRow = pBits + ((y + rectH) * pitch) + (x * bpp);
                        if (pNextRow[2] == 0 && pNextRow[1] == 255 && pNextRow[0] == 255) rectH++;
                        else break;
                    }

                    if (rectW > 0 && rectH > 0) {
                        data.clearZones.push_back({ x, y, x + rectW, y + rectH });
                        for (int ry = y; ry < y + rectH; ry++) {
                            for (int rx = x; rx < x + rectW; rx++) {
                                visited[ry * w + rx] = true;
                            }
                        }
                    }
                }
                else if ((r >= 240 && g >= 240 && b <= 50) || // ?몃옉 (Grunt)
                         (r >= 240 && g >= 140 && g <= 170 && b <= 50) || // 二쇳솴 (Gangster)
                         (r >= 190 && r <= 210 && g <= 50 && b >= 190 && b <= 210) || // 蹂대씪 (Pomp)
                         (r >= 140 && r <= 160 && g >= 140 && g <= 160 && b >= 140 && b <= 160)) // ?뚯깋 (ShieldCop)
                {
                    int enemyType = 1; // 湲곕낯媛?Grunt
                    if (r >= 240 && g >= 240 && b <= 50) enemyType = 1;      // ?몃옉: Grunt
                    else if (r >= 240 && g >= 140 && g <= 170 && b <= 50) enemyType = 0; // 二쇳솴: Gangster
                    else if (r >= 190 && r <= 210 && g <= 50 && b >= 190 && b <= 210) enemyType = 2; // 蹂대씪: Pomp
                    else if (r >= 140 && r <= 160 && g >= 140 && g <= 160 && b >= 140 && b <= 160) enemyType = 3; // ?뚯깋: ShieldCop

                    int rectW = 0, rectH = 0;
                    BYTE targetR = r, targetG = g, targetB = b;

                    // 媛濡?湲몄씠 痢≪젙 (?숈씪 ?됱긽 踰붿쐞 痢≪젙)
                    while (x + rectW < w) {
                        BYTE* pN = pRow + ((x + rectW) * bpp);
                        if (abs((int)pN[2] - (int)targetR) < 20 && abs((int)pN[1] - (int)targetG) < 20 && abs((int)pN[0] - (int)targetB) < 20) rectW++;
                        else break;
                    }
                    // ?몃줈 湲몄씠 痢≪젙
                    while (y + rectH < h) {
                        BYTE* pN = pBits + ((y + rectH) * pitch) + (x * bpp);
                        if (abs((int)pN[2] - (int)targetR) < 20 && abs((int)pN[1] - (int)targetG) < 20 && abs((int)pN[0] - (int)targetB) < 20) rectH++;
                        else break;
                    }

                    if (rectW > 0 && rectH > 0) {
                        // ?곸쓽 ?뚰솚 ?꾩튂瑜?諛뺤뒪??以묒븰 ?섎떒?쇰줈 ?ㅼ젙 (??吏곴???
                        float spawnX = (float)x + (float)rectW / 2.0f - 20.0f; // 20.0f?????덈컲 ?덈퉬
                        float spawnY = (float)y + (float)rectH - 60.0f;       // 60.0f?????믪씠
                        data.enemySpawns.push_back({ spawnX, spawnY, (float)rectW, enemyType });
                        
                        for (int ry = y; ry < y + rectH; ry++) {
                            for (int rx = x; rx < x + rectW; rx++) {
                                visited[ry * w + rx] = true;
                            }
                        }
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
}

void StageManager::LoadAllStages(std::atomic<int>* pProgress) {
    for (int i = 1; i <= 2; ++i) {
        TCHAR mapPath[256], colPath[256];
        wsprintf(mapPath, TEXT("assets/stage%d/map_stage%d.png"), i, i);
        wsprintf(colPath, TEXT("assets/stage%d/colmap_stage%d.png"), i, i);

        m_mapImages[i].Load(mapPath);
        m_colMapImages[i].Load(colPath);
        
        ProcessStage(i);

        if (i == 2) {
            StageData& data = m_stageDataMap[2];
            data.camFixedY = 0.0f; 
            data.mapScale = 1.0f; 
            data.mapRenderOffsetY = 0.0f;
        }

        if (pProgress) {
            *pProgress = 60 + (i * 10);
            Sleep(100);
        }
    }

    if (m_imgSkylineBlack.IsNull()) m_imgSkylineBlack.Load(TEXT("assets/spr_skyline_black.png"));
    if (m_imgSkylineClouds.IsNull()) m_imgSkylineClouds.Load(TEXT("assets/spr_skyline_clouds.png"));
    
    if (pProgress) {
        *pProgress = 85;
        Sleep(100);
    }

    Door::LoadAssets();

    if (pProgress) {
        *pProgress = 90;
        Sleep(100);
    }
}

void StageManager::LoadAssets(int stage) {
    if (m_mapImages.count(stage)) {
        m_imgMap = &m_mapImages[stage];
        m_imgColMap = &m_colMapImages[stage];
        
        StageData& data = m_stageDataMap[stage];
        m_pCurrentDoors = &data.doors;
        m_playerStart = data.playerStart;
        m_clearZones = data.clearZones;
        m_stageLimitTime = data.stageLimitTime;
    }
}

void StageManager::ReleaseAssets() {
    for (auto& pair : m_mapImages) pair.second.Destroy();
    for (auto& pair : m_colMapImages) pair.second.Destroy();
    m_mapImages.clear();
    m_colMapImages.clear();
    m_stageDataMap.clear();
    
    m_imgSkylineBlack.Destroy();
    m_imgSkylineClouds.Destroy();
    Door::ReleaseAssets();
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

void StageManager::Render(HDC hDC, bool isFullMapView, bool showDebugRect, float mapScale, float renderMapScale, float mapOffsetX, float mapOffsetY, float camX, float camY, int virtualWidth, int virtualHeight, bool isSlowMo) {
    if (!hDC) return;

    if (isSlowMo) {
        // ?щ줈??紐⑤뱶 ??諛곌꼍??寃??됱쑝濡?梨꾩?
        HBRUSH hBlack = CreateSolidBrush(RGB(0, 0, 0));
        RECT rect = { 0, 0, virtualWidth, virtualHeight };
        FillRect(hDC, &rect, hBlack);
        DeleteObject(hBlack);
        
        // 臾?Door)???뚮뜑留?(?꾩슂?섎떎硫??ш린??臾몃룄 寃寃?泥섎━?????덉?留? ?쇰떒 ?뚮뜑留곷쭔 嫄대꼫?곌굅??湲곕낯 ?뚮뜑留??좎?)
        // ?곷뱾泥섎읆 ?ㅼ삩 ?④낵瑜?二쇱? ?딆쑝誘濡??쇰떒 諛곌꼍怨??④퍡 寃寃?泥섎━?섍린 ?꾪빐 ?뚮뜑留??앸왂
        return; 
    }

    if (!isFullMapView) {
        if (!m_imgSkylineClouds.IsNull()) {
            float cloudScaleX = 3.0f, cloudScaleY = 1.5f; 
            int cW = (int)(m_imgSkylineClouds.GetWidth() * cloudScaleX), cH = (int)(m_imgSkylineClouds.GetHeight() * cloudScaleY);
            int mapW = GetMapWidth(); float maxCamX = (float)(mapW - (virtualWidth / renderMapScale)), ratioX = (maxCamX > 0) ? (camX / maxCamX) : 0, pX = -ratioX * (cW - virtualWidth);
            if (cW > 0 && cH > 0 && m_imgSkylineClouds.GetWidth() > 0 && m_imgSkylineClouds.GetHeight() > 0) {
                m_imgSkylineClouds.Draw(hDC, (int)pX, 0, cW, cH, 0, 0, m_imgSkylineClouds.GetWidth(), m_imgSkylineClouds.GetHeight());
            }
        }
        if (!m_imgSkylineBlack.IsNull()) {
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


    if (m_pCurrentDoors) {
        for (auto& d : *m_pCurrentDoors) {
            float cFS = 0, cFX = 0, cFY = 0;
            if (isFullMapView) {
                int mapW = GetMapWidth(), mapH = GetMapHeight();
                cFS = (std::min)((float)virtualWidth / mapW, (float)virtualHeight / mapH);
                cFX = (virtualWidth - mapW * cFS) / 2.0f; cFY = (virtualHeight - mapH * cFS) / 2.0f;
            }
            d.Render(hDC, camX, camY, mapScale, isFullMapView, cFS, cFX, cFY);
        }
    }
}
