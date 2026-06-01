#include "Game.h"
#include "Input.h"
#include "../SceneAndMap/Camera.h"
#include "../SceneAndMap/StageManager.h"
#include "../Effects/EffectManager.h"
#include "../UI/UIManager.h"
#include <time.h>
#include <gdiplus.h>
#include <algorithm>
#include <cmath>

Game::Game() : m_hWnd(NULL), m_hInst(NULL), m_winWidth(1280), m_winHeight(720),
    m_isTimePaused(false), m_showDebugRect(false), m_showGrid(false), m_isFullMapView(false),
    m_renderMapScale(1.0f), m_renderPlayerScale(2.0f), m_mapOffsetX(0.0f), m_mapOffsetY(0.0f),
    m_prevTime(0), m_currentStage(1), m_isStageCleared(false), m_stageTimer(0.0f), m_bGameStarted(false) {
}

Game::~Game() {
    for (auto& e : m_enemies) if (e) delete e;
    m_enemies.clear();
}

void Game::Init(HWND hWnd, HINSTANCE hInst) {
    m_hWnd = hWnd; m_hInst = hInst; m_prevTime = GetTickCount();
    EffectManager::LoadAssets(); UIManager::LoadAssets();
    m_player.Init(); m_player.SetMaxHistory(m_maxRewindTime);
    
    LoadStage(1);
}

void Game::LoadStage(int stage) {
    m_currentStage = stage;
    m_isStageCleared = false; // Reset clear flag
    m_bGameStarted = false;   // 스테이지 로드 시 게임 시작 대기 상태로 설정
    StageManager::LoadAssets(m_currentStage);
    
    // Stage-specific Rewind Time (Control rewind time here)
    if (m_currentStage == 1) {
        m_maxRewindTime = 20; // 10 seconds for Stage 1
    } else {
        m_maxRewindTime = 5;  // 5 seconds for Stage 2+
    }
    
    // Verify critical assets
    if (StageManager::GetMap().IsNull() || StageManager::GetColMap().IsNull()) {
        if (stage == 1) {
            MessageBox(m_hWnd, TEXT("Stage 1 assets failed to load!"), TEXT("Error"), MB_ICONERROR);
        }
        return; 
    }

    StageManager::Init();
    SpawnEnemies();
    
    m_stageTimer = StageManager::GetStageLimitTime();
    m_player.SetMaxHistory((int)m_stageTimer); // 리와인드 저장 시간을 스테이지 제한 시간으로 설정
    m_player.SetPos(StageManager::GetPlayerStartX(), StageManager::GetPlayerStartY() - m_player.GetColH());
    Camera::Init();
    Camera::Update(m_player.GetX(), m_player.GetY(), m_player.GetColW(), m_player.GetColH(), Input::GetMouseX(), Input::GetMouseY(), m_renderMapScale, StageManager::GetMapWidth(), StageManager::GetMapHeight(), m_isFullMapView);
}


void Game::SpawnEnemies() {
    for (auto& e : m_enemies) if (e) delete e;
    m_enemies.clear();
    
    if (m_currentStage == 1) {
        Enemy* n = new Pomp(800.0f, 200.0f);
        if (n) { n->Init(); m_enemies.push_back(n); }
    } else {
        Enemy* n = new Pomp(500.0f, 200.0f);
        if (n) { n->Init(); m_enemies.push_back(n); }
        Enemy* n2 = new Pomp(900.0f, 300.0f);
        if (n2) { n2->Init(); m_enemies.push_back(n2); }
    }
}

void Game::Update() {
    DWORD ct = GetTickCount();
    if (ct - m_prevTime < 16) return;
    Input::Update(); UpdateScreenScale();

    // 게임 시작 대기 중인 경우
    if (!m_bGameStarted) {
        if (Input::GetKeyDown(VK_LBUTTON)) {
            m_bGameStarted = true;
            m_prevTime = GetTickCount(); // 시작 시점의 시간으로 갱신하여 타이머 급감 방지
        }
        // 시작 전에는 카메라와 기본적인 애니메이션만 업데이트 (필요 시)
        Camera::Update(m_player.GetX(), m_player.GetY(), m_player.GetColW(), m_player.GetColH(), Input::GetMouseX(), Input::GetMouseY(), m_renderMapScale, StageManager::GetMapWidth(), StageManager::GetMapHeight(), m_isFullMapView);
        m_prevTime = ct;
        return;
    }
    
    // 1. Enemy All Dead Check
    if (!m_isStageCleared) {
        bool anyAlive = false;
        for (auto& e : m_enemies) {
            if (e && e->GetIsAlive()) {
                anyAlive = true;
                break;
            }
        }
        if (!anyAlive) {
            m_isStageCleared = true;
        }
    }

    // 2. Stage Clear Zone Check
    if (m_isStageCleared) {
        if (StageManager::IsInClearZone(m_player.GetX(), m_player.GetY(), m_player.GetColW(), m_player.GetColH())) {
            LoadStage(m_currentStage + 1);
            m_prevTime = GetTickCount();
            return;
        }
    }

    if (Input::GetKeyDown('F')) m_isFullMapView = !m_isFullMapView;
    if (Input::GetKeyDown('E')) { m_showDebugRect = !m_showDebugRect; m_showGrid = !m_showGrid; }
    static bool prR = false; bool cuR = GetAsyncKeyState('R') & 0x8000;
    if (cuR && !prR) { 
        m_player.StartRewind(m_rewindSpeed); 
        StageManager::Reset(); 
        m_stageTimer = StageManager::GetStageLimitTime(); // 타이머 리셋
        EffectManager::Init(); // 리와인드 시작 시 기존 이펙트(먼지, 잔상 등) 제거
        for (auto& e : m_enemies) if (e) e->Reset(); 
    }
    prR = cuR;

    // 타이머 업데이트 (리와인드 중이 아니고 슬로우 모션이 아닐 때만 감소)
    if (!m_player.IsRewinding() && !m_player.GetIsSlowMo() && !m_isTimePaused) {
        float dT = (ct - m_prevTime) / 1000.0f;
        m_stageTimer -= dT;
        if (m_stageTimer < 0) m_stageTimer = 0;
    }

    if (m_player.IsRewinding()) {
        m_player.Update(Input::GetMouseX(), Input::GetMouseY(), Camera::GetCamX(), Camera::GetCamY(), m_renderMapScale, m_mapOffsetX, m_mapOffsetY, m_isFullMapView);
        m_player.UpdateAnimation();
        Camera::Update(m_player.GetX(), m_player.GetY(), m_player.GetColW(), m_player.GetColH(), Input::GetMouseX(), Input::GetMouseY(), m_renderMapScale, StageManager::GetMapWidth(), StageManager::GetMapHeight(), m_isFullMapView);
        m_prevTime = ct; return;
    }
    if (!m_isTimePaused) {
        m_player.Update(Input::GetMouseX(), Input::GetMouseY(), Camera::GetCamX(), Camera::GetCamY(), m_renderMapScale, m_mapOffsetX, m_mapOffsetY, m_isFullMapView);
        for (auto& e : m_enemies) if (e) e->Update(m_player.GetIsSlowMo() ? 0.3f : 1.0f);
    }
    float ts = m_player.GetIsSlowMo() ? 0.3f : 1.0f;
    static int laF = -1;
    if (!m_isTimePaused && m_player.GetState() == PlayerState::ATTACK) {
        int cf = m_player.GetCurrentFrame();
        if (cf >= 1 && cf <= 3 && cf != laF) {
            float cX = m_player.GetX() + m_player.GetColW() / 2.0f, cY = m_player.GetY() + m_player.GetColH() / 2.0f;
            float hX = cX + m_player.GetAttackDirX() * 40.0f - 40.0f, hY = cY + m_player.GetAttackDirY() * 40.0f - 30.0f;
            RECT aR = { (int)hX, (int)hY, (int)(hX + 80.0f), (int)(hY + 60.0f) };
            for (auto& e : m_enemies) {
                if (e && e->GetIsAlive()) {
                    RECT eR = e->GetRect(); RECT ol;
                    if (IntersectRect(&ol, &aR, &eR)) {
                        m_isTimePaused = true;
                        float ex = e->GetX() + e->GetColW() / 2.0f, ey = e->GetY() + e->GetColH() / 2.0f;
                        float dx = ex - cX, dy = ey - cY, dist = (std::max)(1.0f, (float)sqrt(dx * dx + dy * dy));
                        float ux = dx / dist, uy = dy / dist;
                        EffectManager::AddNeonTrail(ex, ey, ux, uy, atan2(uy, ux));
                        EffectManager::AddHitVFX(ex, ey, atan2(uy, ux), ct);
                        Camera::AddPush(ux * 30.0f, uy * 30.0f); Camera::AddShake(1.0f);
                        EffectManager::AddPendingHit(e, (m_player.GetAttackDirX() >= 0) ? 45.0f : -45.0f);
                    }
                }
            }
            laF = cf;
        }
    } else if (!m_isTimePaused) laF = -1;
    if (!m_isTimePaused) {
        DoorOpenEvent de = StageManager::UpdateDoors(m_player.GetX(), m_player.GetY(), m_player.GetColW(), m_player.GetColH(), (GetAsyncKeyState('A') & 0x8000) != 0, (GetAsyncKeyState('D') & 0x8000) != 0, m_player.GetState() == PlayerState::ATTACK, m_player.GetAttackHitX(), m_player.GetAttackHitY(), m_player.GetAttackHitW(), m_player.GetAttackHitH(), ct, ts);
        if (de == DoorOpenEvent::OPEN_BY_ATTACK) m_player.SetState(PlayerState::DOOR_KICK);
        else if (de == DoorOpenEvent::OPEN_BY_WALK) m_player.SetState(PlayerState::DOOR_KICK_FULL);
    }
    EffectManager::Update(ts, ct);
    if (m_isTimePaused && !EffectManager::HasActiveHitVFX()) m_isTimePaused = false;
    if (!m_isTimePaused) m_player.UpdateAnimation();
    Camera::Update(m_player.GetX(), m_player.GetY(), m_player.GetColW(), m_player.GetColH(), Input::GetMouseX(), Input::GetMouseY(), m_renderMapScale, StageManager::GetMapWidth(), StageManager::GetMapHeight(), m_isFullMapView);
    m_prevTime = ct;
}

void Game::Render(HDC hDC) {
    HDC hMemDC = CreateCompatibleDC(hDC);
    HBITMAP hMemBmp = CreateCompatibleBitmap(hDC, VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
    HBITMAP hOldBmp = (HBITMAP)SelectObject(hMemDC, hMemBmp);
    PatBlt(hMemDC, 0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, BLACKNESS);
    float cX = Camera::GetCamX(), cY = Camera::GetCamY(); Camera::ApplyShake(cX, cY);
    StageManager::Render(hMemDC, m_isFullMapView, m_showDebugRect, mapScale, m_renderMapScale, m_mapOffsetX, m_mapOffsetY, cX, cY, VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
    if (m_showGrid) {
        HPEN hGP = CreatePen(PS_SOLID, 1, RGB(100, 100, 100)); HPEN hOP = (HPEN)SelectObject(hMemDC, hGP);
        for (int x = 0; x <= VIRTUAL_WIDTH; x += 20) { MoveToEx(hMemDC, x, 0, NULL); LineTo(hMemDC, x, VIRTUAL_HEIGHT); }
        for (int y = 0; y <= VIRTUAL_HEIGHT; y += 20) { MoveToEx(hMemDC, 0, y, NULL); LineTo(hMemDC, VIRTUAL_WIDTH, y); }
        SelectObject(hMemDC, hOP); DeleteObject(hGP);
    }
    for (auto& e : m_enemies) if (e) e->Render(hMemDC, cX, cY, mapScale, m_showDebugRect);
    if (m_player.GetIsSlowMo()) {
        Gdiplus::Graphics g(hMemDC); Gdiplus::Rect fr(0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
        Gdiplus::GraphicsPath p; p.AddRectangle(fr); Gdiplus::PathGradientBrush pgb(&p);
        pgb.SetCenterColor(Gdiplus::Color(0, 0, 0, 0)); pgb.SetCenterPoint(Gdiplus::PointF(VIRTUAL_WIDTH / 2.0f, VIRTUAL_HEIGHT / 2.0f));
        Gdiplus::Color ec[] = { Gdiplus::Color(180, 0, 0, 0) }; int cnt = 1; pgb.SetSurroundColors(ec, &cnt);
        pgb.SetFocusScales(0.2f, 0.2f); g.FillRectangle(&pgb, fr);
    }
    float fW = (float)VIRTUAL_WIDTH, fH = (float)VIRTUAL_HEIGHT, fCW = (float)StageManager::GetMapWidth(), fCH = (float)StageManager::GetMapHeight();
    float scX = fW / fCW, scY = fH / fCH, cFS = (scX < scY) ? scX : scY;
    float cFX = (fW - fCW * cFS) / 2.0f, cFY = (fH - fCH * cFS) / 2.0f;
    EffectManager::Render(hMemDC, cX, cY, mapScale, m_isFullMapView, cFS, cFX, cFY);
    m_player.Render(hMemDC, cX, cY, mapScale, playerScale, m_renderMapScale, m_mapOffsetX, m_mapOffsetY, m_isFullMapView, m_showDebugRect);
    if (!m_player.IsRewinding()) UIManager::Render(hMemDC, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, Input::GetMouseX(), Input::GetMouseY(), m_player.GetBatteryLevel(), m_stageTimer, StageManager::GetStageLimitTime(), m_bGameStarted);
    SetStretchBltMode(hDC, HALFTONE);
    StretchBlt(hDC, 0, 0, m_winWidth, m_winHeight, hMemDC, 0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, SRCCOPY);
    SelectObject(hMemDC, hOldBmp); DeleteObject(hMemBmp); DeleteDC(hMemDC);
}

void Game::UpdateScreenScale() {
    m_renderMapScale = mapScale; m_renderPlayerScale = playerScale;
    m_mapOffsetX = 0.0f; m_mapOffsetY = 0.0f;
    int mapW = StageManager::GetMapWidth(), mapH = StageManager::GetMapHeight();
    if (m_isFullMapView && mapW > 0) {
        float sX = (float)VIRTUAL_WIDTH / mapW, sY = (float)VIRTUAL_HEIGHT / mapH;
        m_renderMapScale = (sX < sY) ? sX : sY;
        m_renderPlayerScale = playerScale * (m_renderMapScale / mapScale);
        m_mapOffsetX = (VIRTUAL_WIDTH - (mapW * m_renderMapScale)) / 2.0f;
        m_mapOffsetY = (VIRTUAL_HEIGHT - (mapH * m_renderMapScale)) / 2.0f;
    }
}
