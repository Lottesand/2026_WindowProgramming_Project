#pragma once
#include <windows.h>
#include <atlimage.h>
#include <vector>
#include <map>
#include <atomic>
#include "../Objects/Door.h"
#include "../Objects/GlassDome.h"
#include "../Objects/Item.h"

class StageManager {
public:
    struct EnemySpawnInfo {
        float x, y;
        float patrolRange;
        int type; 
    };

    struct StageData {
        std::vector<Door> doors;
        std::vector<GlassDome> glassDomes;
        std::vector<Item> items;
        POINT playerStart;
        std::vector<RECT> clearZones;
        std::vector<EnemySpawnInfo> enemySpawns;
        float stageLimitTime;

        // Stage-specific camera settings
        float camLookAheadX = 150.0f;
        float camFixedY = 60.0f;
        float mapScale = 1.1f;
        float camLerpSpeedX = 0.08f;
        float camLerpSpeedY = 0.08f;
        float mapRenderOffsetY = 0.0f;
    };

    static void Init();
    static void Reset();
    static void SoftReset();
    static void LoadAllStages(std::atomic<int>* pProgress = nullptr);
    static void LoadAssets(int stage = 1);
    static void ReleaseAssets();
    static void Render(HDC hDC, class Gdiplus::Graphics* g, bool isFullMapView, bool showDebugRect, float mapScale, float renderMapScale, float mapOffsetX, float mapOffsetY, float camX, float camY, int virtualWidth, int virtualHeight, bool isSlowMo = false);
    static int UpdateDoors(float playerX, float playerY, float playerW, float playerH, bool isA, bool isD, bool isAttacking, float attackHitX, float attackHitY, float attackHitW, float attackHitH, DWORD currentTime, float timeScale);
    static void UpdateGlassDomes(bool isAttacking, float attackHitX, float attackHitY, float attackHitW, float attackHitH, DWORD currentTime);
    static void UpdateItems(float ts, class Player& player, const std::vector<class Enemy*>& enemies);

    static int GetMapWidth() { return m_imgMap == nullptr || m_imgMap->IsNull() ? 0 : m_imgMap->GetWidth(); }
    static int GetMapHeight() { return m_imgMap == nullptr || m_imgMap->IsNull() ? 0 : m_imgMap->GetHeight(); }
    
    static CImage& GetMap() { return *m_imgMap; }
    static CImage& GetColMap() { return *m_imgColMap; }
    static CImage& GetObjMap() { return *m_imgObjMap; }

    static float GetPlayerStartX() { return (float)m_playerStart.x; }
    static float GetPlayerStartY() { return (float)m_playerStart.y; }
    static bool IsInClearZone(float x, float y, float w, float h);

    static float GetStageLimitTime() { return m_stageLimitTime; }
    static StageData& GetStageData(int stage) { return m_stageDataMap[stage]; }
    static std::vector<Door>* GetCurrentDoors() { return m_pCurrentDoors; }
    static std::vector<GlassDome>* GetCurrentGlassDomes() { return m_pCurrentGlassDomes; }
    static std::vector<Item>* GetCurrentItems() { return m_pCurrentItems; }

private:
    static void ProcessStage(int stage);

    static std::map<int, CImage> m_mapImages;
    static std::map<int, CImage> m_colMapImages;
    static std::map<int, CImage> m_objMapImages;
    static std::map<int, StageData> m_stageDataMap;

    static CImage* m_imgMap;
    static CImage* m_imgColMap;
    static CImage* m_imgObjMap;

    static CImage m_imgSkylineBlack;
    static CImage m_imgSkylineClouds;
    
    static std::vector<Door>* m_pCurrentDoors;
    static std::vector<GlassDome>* m_pCurrentGlassDomes;
    static std::vector<Item>* m_pCurrentItems;
    static POINT m_playerStart;
    static std::vector<RECT> m_clearZones;
    static float m_stageLimitTime;
    static int m_currentStage;
};
