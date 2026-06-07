#pragma once
#include <windows.h>
#include <atlimage.h>
#include <vector>
#include "../Objects/Enemy.h"

struct NeonTrail {
    float x, y;
    float startX, startY;
    float endX, endY;
    float dirX, dirY;
    float angle;
    float length;
    float maxLength;
    int life;
    int maxLife;
};

struct HitVFX {
    float x, y;
    float angle;
    int currentFrame;
    int maxFrame;
    DWORD lastTime;
    bool isSlash;
    bool isGunSpark;
};

struct JumpCloudVFX {
    float x, y;
    float angle;
    int currentFrame;
    int maxFrame;
    DWORD lastTime;
};

struct DustCloudVFX {
    float x, y;
    int currentFrame;
    int maxFrame;
    DWORD lastTime;
    DWORD startTick;
    bool isFacingRight;
};

struct LandCloudVFX {
    float x, y;
    int currentFrame;
    int maxFrame;
    DWORD lastTime;
};

struct BloodSplatterVFX {
    float x, y;
    float vx, vy;
    float angle;
    int imgIndex;
    DWORD startTime;
    bool isBleed;
    bool isDirectional;
};

struct PendingHit {
    class Enemy* target;
    float kvx, kvy;
    int remainingFrames;
};

struct MapBlood {
    float x, y;
    float angle;
    int imgIndex;
    bool isDirectional;
};

class EffectManager {
public:
    static void Init();
    static void LoadAllAssets();
    static void LoadAssets();
    static void ReleaseAssets();
    static void Update(float timeScale, DWORD currentTime);
    static void Render(HDC hDC, float camX, float camY, float mapScale, bool isFullMapView, float cFS, float cFX, float cFY);
    static void RenderMapBlood(HDC hDC, class Gdiplus::Graphics* g, float camX, float camY, float mapScale);
    
    // Call this when the stage changes to resize/clear the blood layer
    static void InitBloodLayer(int mapWidth, int mapHeight);

    static void AddNeonTrail(float x, float y, float ux, float uy, float angle);
    static void AddHitVFX(float x, float y, float angle, DWORD currentTime);
    static void AddGunSparkVFX(float x, float y, float angle, DWORD currentTime);
    static void AddJumpCloudVFX(float x, float y, DWORD currentTime, float angle = 0.0f);
    static void AddDustCloudVFX(float x, float y, bool isFacingRight, DWORD currentTime);
    static void AddLandCloudVFX(float x, float y, DWORD currentTime);
    static void AddPendingHit(class Enemy* target, float kvx, float kvy);
    static void AddBloodSplatter(float x, float y, float vx, float vy, float angle, DWORD currentTime);
    static void AddMapBlood(float x, float y, float angle, int imgIndex);

    static bool HasActiveVFX();
    static bool HasActiveHitVFX();

private:
    static std::vector<NeonTrail> m_neonTrails;
    static std::vector<HitVFX> m_hitVFXs;
    static std::vector<JumpCloudVFX> m_jumpCloudVFXs;
    static std::vector<DustCloudVFX> m_dustCloudVFXs;
    static std::vector<LandCloudVFX> m_landCloudVFXs;
    static std::vector<PendingHit> m_pendingHits;
    static std::vector<BloodSplatterVFX> m_bloodSplatters;
    
    // Blood layer variables
    static HDC m_hBloodLayerDC;
    static HBITMAP m_hBloodLayerBmp;
    static HBITMAP m_hBloodLayerOldBmp;
    static void* m_pBloodLayerBits;
    static int m_bloodLayerWidth;
    static int m_bloodLayerHeight;

    static CImage m_imgVfxSlash[5];
    static CImage m_imgVfxHit[6];
    static CImage m_imgVfxGunSpark[8];
    static CImage m_imgVfxJumpCloud[4];
    static CImage m_imgVfxDustCloud[7];
    static CImage m_imgVfxLandCloud[7];
    static CImage m_imgVfxBloodSplatter[7];
    static CImage m_imgVfxBloodBleed[9];
    static CImage m_imgVfxMapBloodDir[48];
    static CImage m_imgVfxMapBloodStatic[7];

    static constexpr int m_neonTrailLife = 6;
    static constexpr float m_slashWidth = 10.0f;
};

