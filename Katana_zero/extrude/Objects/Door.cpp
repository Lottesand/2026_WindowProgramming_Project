#include "Door.h"
#include <math.h>
#include <algorithm>

CImage Door::m_imgDoor[20];
CImage Door::m_imgGlow[4];
CImage Door::m_imgBreak[6];
CImage Door::m_imgBreakFull[10];

Door::Door(float x, float y, float w, float h) {
    m_x = x; m_y = y; m_w = w; m_h = h;
    m_state = DoorState::DS_CLOSED;
    m_currentFrame = 0;
    m_lastFrameTime = GetTickCount();
    m_isFullBreak = false;
}

Door::~Door() {}

void Door::Reset() {
    m_state = DoorState::DS_CLOSED;
    m_currentFrame = 0;
    m_lastFrameTime = GetTickCount();
}

void Door::Open(bool byAttack, DWORD currentTime) {
    if (m_state == DoorState::DS_CLOSED) {
        m_state = DoorState::DS_OPENING;
        m_currentFrame = 0;
        m_lastFrameTime = currentTime;
    }
}

void Door::LoadAssets() {
    TCHAR path[256];
    if (m_imgDoor[0].IsNull()) {
        for (int i = 0; i < 20; i++) { wsprintf(path, TEXT("assets/spr_door_animation/%d.png"), i); m_imgDoor[i].Load(path); }
    }
    if (m_imgGlow[0].IsNull()) {
        for (int i = 0; i < 4; i++) { 
            wsprintf(path, TEXT("assets/spr_door_glow/%d.png"), i); 
            m_imgGlow[i].Load(path); 
        }
    }
    if (m_imgBreak[0].IsNull()) {
        for (int i = 0; i < 6; i++) { wsprintf(path, TEXT("assets/spr_doorbreak/%d.png"), i); m_imgBreak[i].Load(path); }
    }
    if (m_imgBreakFull[0].IsNull()) {
        for (int i = 0; i < 10; i++) { wsprintf(path, TEXT("assets/spr_doorbreak_full/%d.png"), i); m_imgBreakFull[i].Load(path); }
    }
}

void Door::ReleaseAssets() {
    for (int i = 0; i < 20; i++) m_imgDoor[i].Destroy();
    for (int i = 0; i < 4; i++) m_imgGlow[i].Destroy();
    for (int i = 0; i < 6; i++) m_imgBreak[i].Destroy();
    for (int i = 0; i < 10; i++) m_imgBreakFull[i].Destroy();
}

DoorOpenEvent Door::Update(float playerX, float playerY, float playerW, float playerH, bool isA, bool isD, bool isAttacking, float attackHitX, float attackHitY, float attackHitW, float attackHitH, DWORD currentTime, float timeScale) {
    if (m_state == DoorState::DS_BROKEN) return DoorOpenEvent::DOE_NONE;

    bool playerNear = (playerX + playerW >= m_x - 40.0f && playerX <= m_x + m_w + 40.0f &&
                       playerY + playerH >= m_y - 10.0f && playerY <= m_y + m_h + 10.0f);

    if (m_state == DoorState::DS_CLOSED) {
        if (isAttacking) {
            bool attackHit = (attackHitX < m_x + m_w && attackHitX + attackHitW > m_x &&
                              attackHitY < m_y + m_h && attackHitY + attackHitH > m_y);
            if (attackHit) {
                m_state = DoorState::DS_OPENING;
                m_currentFrame = 0;
                m_lastFrameTime = currentTime;
                return DoorOpenEvent::DOE_OPEN_BY_ATTACK;
            }
        }
        if (playerNear && (isA || isD)) {
            m_state = DoorState::DS_OPENING;
            m_currentFrame = 0;
            m_lastFrameTime = currentTime;
            return DoorOpenEvent::DOE_OPEN_BY_WALK;
        }
    }

    if (m_state == DoorState::DS_OPENING || m_state == DoorState::DS_BREAKING) {
        if (currentTime - m_lastFrameTime >= (DWORD)(m_aniDelay / timeScale)) {
            m_currentFrame++;
            m_lastFrameTime = currentTime;
            if (m_state == DoorState::DS_OPENING && m_currentFrame >= 20) {
                m_state = DoorState::DS_OPENED;
                m_currentFrame = 19;
            }
            if (m_state == DoorState::DS_BREAKING) {
                int maxF = m_isFullBreak ? 10 : 6;
                if (m_currentFrame >= maxF) {
                    m_state = DoorState::DS_BROKEN;
                }
            }
        }
    }

    return DoorOpenEvent::DOE_NONE;
}

void Door::Render(HDC hDC, float camX, float camY, float mapScale, bool isFullMapView, float cFS, float cFX, float cFY) {
    if (!hDC) return;
    float pFS = isFullMapView ? cFS : mapScale;
    float dX, dY;
    if (isFullMapView) { dX = m_x * cFS + cFX; dY = m_y * cFS + cFY; }
    else { dX = (m_x - camX) * mapScale; dY = (m_y - camY) * mapScale; }

    if (m_state == DoorState::DS_BROKEN) return;
    
    CImage* img = nullptr;
    int safeFrame = (std::max)(0, m_currentFrame);
    
    if (m_state == DoorState::DS_OPENING || m_state == DoorState::DS_CLOSED || m_state == DoorState::DS_OPENED) {
        if (m_state == DoorState::DS_OPENED) img = &m_imgDoor[19];
        else if (safeFrame >= 0 && safeFrame < 20) img = &m_imgDoor[safeFrame];
    } else if (m_state == DoorState::DS_BREAKING) {
        if (m_isFullBreak) { if (safeFrame < 10) img = &m_imgBreakFull[safeFrame]; }
        else { if (safeFrame < 6) img = &m_imgBreak[safeFrame]; }
    }

    if (img && !img->IsNull()) {
        float baseImgW = (float)img->GetWidth();
        float baseImgH = (float)img->GetHeight();
        float scaleY = m_h / (baseImgH > 0 ? baseImgH : 1.0f);
        float scaleX = scaleY;

        float drawW = baseImgW * scaleX * pFS;
        float drawH = baseImgH * scaleY * pFS;
        
        // 오프셋 제거: objmap에서 인식된 좌표(dX, dY)에 정확히 맞춤
        float drawX = dX;
        float drawY = dY + (m_h * pFS - drawH);

        img->Draw(hDC, (int)drawX, (int)drawY, (int)drawW, (int)drawH);
        
        if (m_state == DoorState::DS_CLOSED) {
            int glowFrame = (GetTickCount() / 100) % 4;
            CImage* gImg = &m_imgGlow[glowFrame];
            if (gImg && !gImg->IsNull()) {
                gImg->TransparentBlt(hDC, (int)drawX, (int)drawY, (int)drawW, (int)drawH, 0, 0, gImg->GetWidth(), gImg->GetHeight(), RGB(0, 0, 0));
            }
        }
    }
}
