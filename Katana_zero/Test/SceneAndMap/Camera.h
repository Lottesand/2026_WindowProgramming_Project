#pragma once
#include <windows.h>

class Camera {
public:
    static void Init();
    static void Update(float playerX, float playerY, float playerColW, float playerColH, int mouseX, int mouseY, float renderMapScale, int mapWidth, int mapHeight, bool isFullMapView, bool forceSnap = false);
    
    static float GetCamX() { return m_camX; }
    static float GetCamY() { return m_camY; }
    static void AddShake(float intensity);
    static void AddPush(float x, float y);
    static void ApplyShake(float& x, float& y);
    
    static void StartRewindEffect();
    static bool IsRewindEffectActive() { return m_rewindTimer > 0; }
    static float GetRewindYOffset() { return m_rewindYOffset; }

private:
    static float m_camX;
    static float m_camY;
    static float m_curShakeX;
    static float m_curShakeY;
    static float m_camPushX;
    static float m_camPushY;
    static float m_shakeTrauma;
    
    static float m_rewindTimer;
    static float m_rewindYOffset;

    // Constants (from main.cpp)
    static constexpr float m_camLookAheadX = 150.0f;
    static constexpr float m_camLerpSpeedX = 0.08f;
    static constexpr float m_camLerpSpeedY = 0.08f;
    static constexpr float m_camY_Fixed = 60.0f;
    static constexpr float m_shakeIntensity = 10.0f;
    static constexpr float m_shakeDecay = 0.85f;
    static constexpr int VIRTUAL_WIDTH = 1280;
    static constexpr int VIRTUAL_HEIGHT = 720;
};

