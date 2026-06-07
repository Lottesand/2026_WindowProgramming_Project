#pragma once
#include <windows.h>
#include <atlimage.h>
#include <vector>

enum class DoorState {
    DS_CLOSED,
    DS_OPENING,
    DS_OPENED,
    DS_BREAKING,
    DS_BROKEN
};

enum class DoorOpenEvent {
    DOE_NONE,
    DOE_OPEN_BY_ATTACK,
    DOE_OPEN_BY_WALK
};

class Door {
public:
    Door(float x, float y, float w, float h);
    ~Door();

    static void LoadAssets();
    static void ReleaseAssets();
    
    DoorOpenEvent Update(float playerX, float playerY, float playerW, float playerH, 
                bool isA, bool isD, bool isAttacking, 
                float attackHitX, float attackHitY, float attackHitW, float attackHitH,
                DWORD currentTime, float timeScale);

    void Reset(); 
    void Open(bool byAttack, DWORD currentTime);

    void Render(HDC hDC, float camX, float camY, float mapScale, bool isFullMapView, float pFS, float pFX, float pFY);


    float GetX() const { return m_x; }
    float GetY() const { return m_y; }
    float GetW() const { return m_w; }
    float GetH() const { return m_h; }
    bool IsClosed() const { return m_state == DoorState::DS_CLOSED || m_state == DoorState::DS_OPENING || m_state == DoorState::DS_BREAKING; }
    bool IsFullyOpened() const { return m_state == DoorState::DS_OPENED || m_state == DoorState::DS_BROKEN; }
    int GetCurrentFrame() const { return m_currentFrame; }

private:
    float m_x, m_y;
    float m_w, m_h;
    DoorState m_state;
    
    int m_currentFrame;
    DWORD m_lastFrameTime;
    bool m_isFullBreak;

    static CImage m_imgDoor[20];
    static CImage m_imgGlow[4];
    static CImage m_imgBreak[6];
    static CImage m_imgBreakFull[10];

    const int m_aniDelay = 40;
    const int m_glowDelay = 100;
};

