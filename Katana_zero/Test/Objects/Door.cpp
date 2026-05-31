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
    for (int i = 0; i < 20; i++) { wsprintf(path, TEXT("assets/spr_door_animation/%d.png"), i); m_imgDoor[i].Load(path); }
    for (int i = 0; i < 4; i++) { wsprintf(path, TEXT("assets/spr_door_glow/%d.png"), i); m_imgGlow[i].Load(path); }
    for (int i = 0; i < 6; i++) { wsprintf(path, TEXT("assets/spr_doorbreak/%d.png"), i); m_imgBreak[i].Load(path); }
    for (int i = 0; i < 10; i++) { wsprintf(path, TEXT("assets/spr_doorbreak_full/%d.png"), i); m_imgBreakFull[i].Load(path); }
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
                m_state = DoorState::BREAKING;
                m_currentFrame = 0;
                m_lastFrameTime = currentTime;
                m_isFullBreak = true;
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
        if (m_state == DoorState::CLOSED && safeFrame == 0) {
            float baseImgW = (float)m_imgDoor[0].GetWidth();
            scaleX = m_w / (baseImgW > 0 ? baseImgW : 1.0f);
        }
        float drawW = img->GetWidth() * scaleX * pFS;
        float drawH = img->GetHeight() * scaleY * pFS;
        float drawY = dY + (m_h * pFS) - drawH;
        img->Draw(hDC, (int)dX, (int)drawY, (int)drawW, (int)drawH);

        if (m_state == DoorState::CLOSED) {
            CImage* glowImg = &m_imgGlow[(GetTickCount() / m_glowDelay) % 4];
            if (glowImg && !glowImg->IsNull()) {
                Gdiplus::Graphics graphics(hDC);
                Gdiplus::Bitmap gdiGlow(glowImg->GetWidth(), glowImg->GetHeight(), glowImg->GetPitch(), PixelFormat32bppARGB, (BYTE*)glowImg->GetBits());
                Gdiplus::ImageAttributes attr;
                Gdiplus::ColorMatrix matrix = {
                    0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                    0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
                    1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                    0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                    0.0f, 0.0f, 0.0f, 0.0f, 1.0f
                };
                attr.SetColorMatrix(&matrix, Gdiplus::ColorMatrixFlagsDefault, Gdiplus::ColorAdjustTypeBitmap);
                float gW = glowImg->GetWidth() * scaleX * pFS;
                float gH = glowImg->GetHeight() * scaleY * pFS;
                graphics.DrawImage(&gdiGlow, Gdiplus::RectF(dX, dY + (m_h * pFS) - gH, gW, gH), 0, 0, (float)glowImg->GetWidth(), (float)glowImg->GetHeight(), Gdiplus::UnitPixel, &attr);
            }
        }
    }
}
