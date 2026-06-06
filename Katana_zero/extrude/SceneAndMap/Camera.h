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

    static void SetLookAheadX(float val) { m_camLookAheadX = val; }
    static void SetFixedY(float val) { m_camY_Fixed = val; }
    static void SetLerpSpeedX(float val) { m_camLerpSpeedX = val; }
    static void SetLerpSpeedY(float val) { m_camLerpSpeedY = val; }

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

    // Configurable parameters
    static float m_camLookAheadX;
    static float m_camLerpSpeedX;
    static float m_camLerpSpeedY;
    static float m_camY_Fixed;

    // Constants
    static constexpr float m_shakeIntensity = 10.0f;
    static constexpr float m_shakeDecay = 0.85f;
    static constexpr int VIRTUAL_WIDTH = 1280;
    static constexpr int VIRTUAL_HEIGHT = 720;
};

