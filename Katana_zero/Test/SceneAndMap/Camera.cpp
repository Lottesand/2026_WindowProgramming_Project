#include "Camera.h"
#include <math.h>
#include <stdlib.h>

float Camera::m_camX = 0.0f;
float Camera::m_camY = 60.0f;
float Camera::m_curShakeX = 0.0f;
float Camera::m_curShakeY = 0.0f;
float Camera::m_camPushX = 0.0f;
float Camera::m_camPushY = 0.0f;
float Camera::m_shakeTrauma = 0.0f;
float Camera::m_rewindTimer = 0.0f;
float Camera::m_rewindYOffset = 0.0f;

float Camera::m_camLookAheadX = 150.0f;
float Camera::m_camLerpSpeedX = 0.08f;
float Camera::m_camLerpSpeedY = 0.08f;
float Camera::m_camY_Fixed = 60.0f;

void Camera::Init() {
    m_camX = 0.0f;
    m_camY = 60.0f;
    m_rewindTimer = 0.0f;
    m_rewindYOffset = 0.0f;
}

void Camera::Update(float playerX, float playerY, float playerColW, float playerColH, int mouseX, int mouseY, float renderMapScale, int mapWidth, int mapHeight, bool isFullMapView, bool forceSnap) {
    if (isFullMapView) return;

    // 리와인드 효과 업데이트
    if (m_rewindTimer > 0) {
        m_rewindTimer -= 0.03f; // 효과 지속 시간 조절
        if (m_rewindTimer < 0) m_rewindTimer = 0;

        // 꿀렁거리는 효과 (Sin파 이용)
        m_curShakeX = sin(m_rewindTimer * 40.0f) * 15.0f;
        m_curShakeY = cos(m_rewindTimer * 30.0f) * 15.0f;

        // 위로 샤라락 올라가는 효과 (씬이 위로 올라가려면 카메라 좌표가 커져야 함)
        m_rewindYOffset += 60.0f;
    } else {
        m_rewindYOffset = 0.0f;
    }

    float mOffX = (float)(mouseX - (VIRTUAL_WIDTH / 2)) / (VIRTUAL_WIDTH / 2);
    float pMidX = playerX + playerColW / 2.0f;
    float vHW = (VIRTUAL_WIDTH / renderMapScale) / 2.0f;
    float tCamX = pMidX - vHW + (mOffX * m_camLookAheadX);
    
    if (forceSnap) {
        m_camX = tCamX;
        m_camY = m_camY_Fixed;
    } else {
        m_camX += (tCamX - m_camX) * m_camLerpSpeedX;
        m_camY += (m_camY_Fixed - m_camY) * m_camLerpSpeedY;
    }

    m_camPushX *= m_shakeDecay;
    m_camPushY *= m_shakeDecay;

    if (m_rewindTimer <= 0) { // 리와인드 효과 중이 아닐 때만 일반 쉐이크 적용
        float trSq = m_shakeTrauma * m_shakeTrauma;
        if (trSq > 0.001f) {
            m_curShakeX = ((float)(rand() % 100) / 50.0f - 1.0f) * m_shakeIntensity * trSq;
            m_curShakeY = ((float)(rand() % 100) / 50.0f - 1.0f) * m_shakeIntensity * trSq;
        } else {
            m_curShakeX = 0;
            m_curShakeY = 0;
        }
    }

    m_shakeTrauma *= m_shakeDecay;
    if (m_shakeTrauma < 0.01f) m_shakeTrauma = 0;

    if (m_camX < 0) m_camX = 0;
    if (m_camY < 0) m_camY = 0;

    if (mapWidth > 0 && mapHeight > 0) {
        float vW = VIRTUAL_WIDTH / renderMapScale;
        float vH = VIRTUAL_HEIGHT / renderMapScale;
        float maxCX = (float)mapWidth - vW;
        if (m_camX > maxCX) m_camX = maxCX;
        float maxCY = (float)mapHeight - vH;
        if (m_camY > maxCY) m_camY = maxCY;
        if (m_camX < 0) m_camX = 0;
        if (m_camY < 0) m_camY = 0;
    }
}

void Camera::AddShake(float intensity) {
    m_shakeTrauma = intensity;
}

void Camera::AddPush(float x, float y) {
    m_camPushX = x;
    m_camPushY = y;
}

void Camera::ApplyShake(float& x, float& y) {
    x += (m_curShakeX + m_camPushX);
    y += (m_curShakeY + m_camPushY + m_rewindYOffset);
}

void Camera::StartRewindEffect() {
    m_rewindTimer = 1.0f;
    m_rewindYOffset = 0.0f;
}

