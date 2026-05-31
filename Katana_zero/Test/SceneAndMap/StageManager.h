#pragma once
#include <windows.h>
#include <atlimage.h>
#include <vector>
#include "../Objects/Door.h"

class StageManager {
public:
    static void Init();
    static void Reset(); // 臾????ㅽ뀒?댁? ?붿냼 珥덇린??
    static void LoadAssets();
    static void ReleaseAssets();
    static void Render(HDC hDC, bool isFullMapView, bool showDebugRect, float mapScale, float renderMapScale, float mapOffsetX, float mapOffsetY, float camX, float camY, int virtualWidth, int virtualHeight);
    static DoorOpenEvent UpdateDoors(float playerX, float playerY, float playerW, float playerH, bool isA, bool isD, bool isAttacking, float attackHitX, float attackHitY, float attackHitW, float attackHitH, DWORD currentTime, float timeScale);


    static int GetMapWidth() { return m_imgMap.IsNull() ? 0 : m_imgMap.GetWidth(); }
    static int GetMapHeight() { return m_imgMap.IsNull() ? 0 : m_imgMap.GetHeight(); }
    
    static CImage& GetMap() { return m_imgMap; }
    static CImage& GetColMap() { return m_imgColMap; }

private:
    static CImage m_imgMap;
    static CImage m_imgColMap;
    static CImage m_imgSkylineBlack;
    static CImage m_imgSkylineClouds;
    static std::vector<Door> m_doors;
};

