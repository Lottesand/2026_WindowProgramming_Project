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
float Camera::m_rewindDuration = 1.0f; // 異붽???硫ㅻ쾭 ?좎?
float Camera::m_rewindYOffset = 0.0f;

float Camera::m_camLookAheadX = 150.0f;
float Camera::m_camLerpSpeedX = 0.08f;
float Camera::m_camLerpSpeedY = 0.08f;
float Camera::m_camY_Fixed = 60.0f;

void Camera::Init() {
    m_camX = 0.0f;
    m_camY = 60.0f;
    m_rewindTimer = 0.0f;
    m_rewindDuration = 1.0f;
    m_rewindYOffset = 0.0f;
}

void Camera::Update(float playerX, float playerY, float playerColW, float playerColH, int mouseX, int mouseY, float renderMapScale, int mapWidth, int mapHeight, bool isFullMapView, bool forceSnap) {
    if (isFullMapView) return;

    if (m_rewindTimer > 0) {
        m_rewindTimer -= 0.03f;
        if (m_rewindTimer < 0) m_rewindTimer = 0;
        
        // 시각 효과 적용 (타이머가 활성화된 동안에만)
        m_curShakeX = (float)sin(m_rewindTimer * 40.0f) * 15.0f;
        m_curShakeY = (float)cos(m_rewindTimer * 30.0f) * 15.0f;
        m_rewindYOffset += 60.0f;
    } else {
        m_curShakeX = 0;
        m_curShakeY = 0;
        m_rewindYOffset = 0.0f;
    }

    float mOffX = (float)(mouseX - (VIRTUAL_WIDTH / 2)) / (VIRTUAL_WIDTH / 2);
    float pMidX = playerX + playerColW / 2.0f;
    float vHW = (VIRTUAL_WIDTH / renderMapScale) / 2.0f;
    float tCamX = pMidX - vHW + (mOffX * m_camLookAheadX);
    
    // 만약 m_camY_Fixed 값이 0 미만이라면 플레이어의 Y 좌표를 따라가도록 설정 (Y축 카메라 언락)
    float tCamY = m_camY_Fixed;
    if (m_camY_Fixed < 0.0f) {
        float pMidY = playerY + playerColH / 2.0f;
        float vHH = (VIRTUAL_HEIGHT / renderMapScale) / 2.0f;
        tCamY = pMidY - vHH;
    }
    
    if (forceSnap) {
        m_camX = tCamX;
        m_camY = tCamY;
    } else {
        m_camX += (tCamX - m_camX) * m_camLerpSpeedX;
        m_camY += (tCamY - m_camY) * m_camLerpSpeedY;
    }

    m_camPushX *= m_shakeDecay;
    m_camPushY *= m_shakeDecay;

    if (m_rewindTimer <= 0) {
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

    const float shakeMargin = 40.0f; // 상하좌우 모든 방향으로 여백 확보

    if (mapWidth > 0 && mapHeight > 0) {
        float vW = VIRTUAL_WIDTH / renderMapScale;
        float vH = VIRTUAL_HEIGHT / renderMapScale;
        
        // 가로 제한 계산 (여백 포함)
        float minCX = shakeMargin;
        float maxCX = (float)mapWidth - vW - shakeMargin;
        if (maxCX < minCX) m_camX = (float)mapWidth / 2.0f - vW / 2.0f;
        else {
            if (m_camX < minCX) m_camX = minCX;
            if (m_camX > maxCX) m_camX = maxCX;
        }

        // 세로 제한 계산 (여백 포함)
        float minCY = shakeMargin;
        float maxCY = (float)mapHeight - vH - shakeMargin;
        if (maxCY < minCY) m_camY = (float)mapHeight / 2.0f - vH / 2.0f;
        else {
            if (m_camY < minCY) m_camY = minCY;
            if (m_camY > maxCY) m_camY = maxCY;
        }
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

void Camera::StartRewindEffect(float duration) {
    m_rewindDuration = duration;
    m_rewindTimer = duration; // ?몄옄濡?諛쏆? duration???ъ슜?섏?留??대? 濡쒖쭅? ?덉쟾 諛⑹떇
    m_rewindYOffset = 0.0f;
}
