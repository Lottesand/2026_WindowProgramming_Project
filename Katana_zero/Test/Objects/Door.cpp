#include "Door.h"
#include <math.h>
#include <algorithm>

CImage Door::m_imgDoor[20];
CImage Door::m_imgGlow[4];
CImage Door::m_imgBreak[6];
CImage Door::m_imgBreakFull[10];

Door::Door(float x, float y, float w, float h) {
    m_x = x; m_y = y; m_w = w; m_h = h;
    m_state = DoorState::CLOSED;
    m_currentFrame = 0;
    m_lastFrameTime = GetTickCount();
    m_isFullBreak = false;
}

Door::~Door() {
}

void Door::Reset() {
    m_state = DoorState::CLOSED;
    m_currentFrame = 0;
    m_lastFrameTime = GetTickCount();
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
    if (m_state == DoorState::BROKEN) return DoorOpenEvent::NONE;

    bool playerNear = (playerX + playerW >= m_x - 20.0f && playerX <= m_x + m_w + 20.0f &&
                       playerY + playerH >= m_y && playerY <= m_y + m_h);

    if (m_state == DoorState::CLOSED) {
        if (isAttacking) {
            bool attackHit = (attackHitX < m_x + m_w && attackHitX + attackHitW > m_x &&
                              attackHitY < m_y + m_h && attackHitY + attackHitH > m_y);
            if (attackHit) {
                m_state = DoorState::OPENING;
                m_currentFrame = 0;
                m_lastFrameTime = currentTime;
                return DoorOpenEvent::OPEN_BY_ATTACK;
            }
        }
        if (playerNear && ((isA && playerX > m_x) || (isD && playerX < m_x))) {
            m_state = DoorState::OPENING;
            m_currentFrame = 0;
            m_lastFrameTime = currentTime;
            return DoorOpenEvent::OPEN_BY_WALK;
        }
    }

    if (m_state == DoorState::OPENING || m_state == DoorState::BREAKING) {
        if (currentTime - m_lastFrameTime >= (DWORD)(m_aniDelay / timeScale)) {
            m_currentFrame++;
            m_lastFrameTime = currentTime;
            if (m_state == DoorState::OPENING && m_currentFrame >= 20) {
                m_state = DoorState::OPENED;
                m_currentFrame = 19;
            }
            if (m_state == DoorState::BREAKING) {
                int maxF = m_isFullBreak ? 10 : 6;
                if (m_currentFrame >= maxF) {
                    m_state = DoorState::BROKEN;
                }
            }
        }
    }

    return DoorOpenEvent::NONE;
}

void Door::Render(HDC hDC, float camX, float camY, float mapScale, bool isFullMapView, float cFS, float cFX, float cFY) {
    if (!hDC) return;
    float pFS = isFullMapView ? cFS : mapScale;
    float dX, dY;
    if (isFullMapView) { dX = m_x * cFS + cFX; dY = m_y * cFS + cFY; }
    else { dX = (m_x - camX) * mapScale; dY = (m_y - camY) * mapScale; }

    if (m_state == DoorState::BROKEN) return;
    CImage* img = nullptr;
    int safeFrame = (std::max)(0, m_currentFrame);
    
    if (m_state == DoorState::OPENING || m_state == DoorState::CLOSED || m_state == DoorState::OPENED) {
        if (m_state == DoorState::OPENED) img = &m_imgDoor[19];
        else if (safeFrame >= 0 && safeFrame < 20) img = &m_imgDoor[safeFrame];
    } else if (m_state == DoorState::BREAKING) {
        if (m_isFullBreak) { if (safeFrame < 10) img = &m_imgBreakFull[safeFrame]; }
        else { if (safeFrame < 6) img = &m_imgBreak[safeFrame]; }
    }

    if (img && !img->IsNull()) {
        if (m_imgDoor[0].IsNull()) {
            Rectangle(hDC, (int)dX, (int)dY, (int)(dX + m_w * pFS), (int)(dY + m_h * pFS));
            return;
        }

        float baseImgH = (float)m_imgDoor[0].GetHeight();
        float scaleY = m_h / (baseImgH > 0 ? baseImgH : 1.0f);
        float scaleX = scaleY;
        float drawX = dX;
        
        if (m_state == DoorState::CLOSED && safeFrame == 0) {
            float baseImgW = (float)m_imgDoor[0].GetWidth();
            float targetWidth = 160.0f; // 스프라이트 여백을 고려하여 64px로 대폭 증가
            scaleX = targetWidth / (baseImgW > 0 ? baseImgW : 1.0f);
            
            // 두께가 커진 만큼 오른쪽 오프셋도 약간 조정 (원래 40이었으나 사각형 두께만큼 왼쪽으로 이동 요청으로 0으로 변경했다가 다시 40으로 원복)
            float offsetX = 40.0f;
            drawX = dX + ((m_w - targetWidth) / 2.0f + offsetX) * pFS;
        }

        float drawW = img->GetWidth() * scaleX * pFS - 20.0f;
        float drawH = img->GetHeight() * scaleY * pFS;
        float drawY = dY + (m_h * pFS) - drawH;
        img->Draw(hDC, (int)drawX, (int)drawY, (int)drawW, (int)drawH);
        if (m_state == DoorState::CLOSED) {
            // 더 밝은 하늘색 (Sky Blue: 135, 206, 235)
            COLORREF skyBlue = RGB(135, 206, 235);
            HGDIOBJ hOldBrush = SelectObject(hDC, GetStockObject(NULL_BRUSH));

            // [너비 컨트롤 가이드]
            // 1. actualDoorW: 글로우가 시작되는 '문의 실제 폭'. (현재 0.15f = 15%)
            //    이 값을 줄이면 좌우 선이 서로 가까워져 문에 더 밀착됩니다.
            // 2. 루프 횟수 (i < 10): 글로우가 바깥으로 퍼지는 '단계/두께'.
            //    이 숫자를 줄이면 번짐 효과의 전체 너비가 줄어듭니다.
            float actualDoorW = drawW * 0.15f; 
            float centerX = drawX + drawW / 2.0f - 35.0f;
            int rectL = (int)(centerX - actualDoorW / 2.0f);
            int rectR = (int)(centerX + actualDoorW / 2.0f);
            int rectT = (int)drawY;
            int rectB = (int)(drawY + drawH);

            // 10단계 그라데이션, 위아래 선 없이 좌우 수직선만 그림
            for (int i = 0; i < 10; i++) {
                int intensity = 255 - (i * 25); 
                if (intensity < 0) intensity = 0;

                COLORREF layerColor = RGB(
                    (GetRValue(skyBlue) * intensity) / 255,
                    (GetGValue(skyBlue) * intensity) / 255,
                    (GetBValue(skyBlue) * intensity) / 255
                );

                HPEN hPen = CreatePen(PS_SOLID, 1, layerColor);
                HGDIOBJ hOldPen = SelectObject(hDC, hPen);
                
                // 왼쪽 수직선
                MoveToEx(hDC, rectL - i, rectT, NULL);
                LineTo(hDC, rectL - i, rectB);
                
                // 오른쪽 수직선
                MoveToEx(hDC, rectR + i, rectT, NULL);
                LineTo(hDC, rectR + i, rectB);

                SelectObject(hDC, hOldPen);
                DeleteObject(hPen);
            }

            SelectObject(hDC, hOldBrush);
        }
    }
}
