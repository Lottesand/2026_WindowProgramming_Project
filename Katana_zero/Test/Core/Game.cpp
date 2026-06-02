#include "Game.h"
#include "Input.h"
#include "../SceneAndMap/Camera.h"
#include "../SceneAndMap/StageManager.h"
#include "../Objects/Door.h"
#include "../Effects/EffectManager.h"
#include "../UI/UIManager.h"
#include <time.h>
#include <gdiplus.h>
#include <algorithm>
#include <cmath>
#include <thread>
#include <objbase.h>

Game::Game() : m_hWnd(NULL), m_hInst(NULL), m_winWidth(1280), m_winHeight(720),
    m_isTimePaused(false), m_showDebugRect(false), m_showGrid(false), m_isFullMapView(false),
    m_renderMapScale(1.0f), m_renderPlayerScale(2.0f), m_mapOffsetX(0.0f), m_mapOffsetY(0.0f),
    m_prevTime(0), m_currentStage(1), m_isStageCleared(false), m_stageTimer(0.0f), m_bGameStarted(false) {
    m_isLoaded = false;
    m_loadingProgress = 0;
}

Game::~Game() {
    for (auto& e : m_enemies) if (e) delete e;
    m_enemies.clear();
}

void Game::Init(HWND hWnd, HINSTANCE hInst) {
    m_hWnd = hWnd; m_hInst = hInst; m_prevTime = GetTickCount();
    
    // 백그라운드 스레드에서 모든 에셋 로드 시작
    std::thread loadingThread(LoadingThreadProc, this);
    loadingThread.detach();
}

void Game::LoadingThreadProc(Game* pGame) {
    // COM 초기화 (CImage::Load가 내부적으로 사용)
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    pGame->LoadAllAssets();

    CoUninitialize();
}

void Game::LoadAllAssets() {
    m_loadingProgress = 0;

    // 0. 로딩 화면 이미지 로드
    m_imgLoading.Load(TEXT("assets/loading.png"));
    m_imgReplayUI[0].Load(TEXT("assets/Replay/0.png"));
    m_imgReplayUI[1].Load(TEXT("assets/Replay/1.png"));
    m_imgReplayUI[2].Load(TEXT("assets/Replay/3.png"));
    m_imgReplayUI[3].Load(TEXT("assets/Replay/yes.png"));
    m_loadingProgress = 5;
    Sleep(50); // 진행 상태를 시각적으로 보여주기 위한 짧은 대기

    // 1. UI 에셋 로드 (10%)
    UIManager::LoadAssets();
    m_loadingProgress = 15;
    Sleep(50);

    // 2. 이펙트 에셋 로드 (10%)
    EffectManager::LoadAssets();
    m_loadingProgress = 20;
    Sleep(50);

    // 3. 적 에셋 전역 초기화 (20%)
    Gangster(0, 0).Init();
    Grunt(0, 0).Init();
    m_loadingProgress = 30;
    Sleep(50);
    
    Pomp(0, 0).Init();
    ShieldCop(0, 0).Init();
    m_loadingProgress = 40;
    Sleep(50);

    // 4. 플레이어 에셋 로드 (20%)
    m_player.Init();
    m_loadingProgress = 60;
    Sleep(50);

    // 5. 모든 스테이지 에셋 로드 및 전처리 (30%)
    StageManager::LoadAllStages(&m_loadingProgress);
    
    // 6. 초기 스테이지 설정 (10%)
    LoadStage(1);
    m_player.SetMaxHistory(m_maxRewindTime);
    m_loadingProgress = 100;
    Sleep(500); // 100% 도달 후 사용자가 확인할 수 있도록 충분히 유지

    // 로딩 완료
    m_isLoaded = true;
}

void Game::LoadStage(int stage) {
    m_currentStage = stage;
    m_isStageCleared = false; 
    m_bGameStarted = false;   
    
    // 이미 로드/전처리된 데이터에서 현재 스테이지 설정 (즉시 완료)
    StageManager::LoadAssets(m_currentStage);
    
    if (m_currentStage == 1) {
        m_maxRewindTime = 20;
    } else {
        m_maxRewindTime = 5;
    }
    
    if (StageManager::GetMap().IsNull() || StageManager::GetColMap().IsNull()) {
        return; 
    }

    StageManager::Init();
    SpawnEnemies();
    
    // Stage-specific camera and map scale settings
    StageManager::StageData& data = StageManager::GetStageData(m_currentStage);
    mapScale = data.mapScale;
    Camera::SetLookAheadX(data.camLookAheadX);
    Camera::SetFixedY(data.camFixedY);
    Camera::SetLerpSpeedX(data.camLerpSpeedX);
    Camera::SetLerpSpeedY(data.camLerpSpeedY);

    m_stageTimer = StageManager::GetStageLimitTime();
    m_player.SetMaxHistory((int)m_stageTimer); 
    m_player.SetPos(StageManager::GetPlayerStartX(), StageManager::GetPlayerStartY() - m_player.GetColH() - 2.0f);
    Camera::Init();
    Camera::Update(m_player.GetX(), m_player.GetY(), m_player.GetColW(), m_player.GetColH(), Input::GetMouseX(), Input::GetMouseY(), m_renderMapScale, StageManager::GetMapWidth(), StageManager::GetMapHeight(), m_isFullMapView);
}


void Game::SpawnEnemies() {
    for (auto& e : m_enemies) if (e) delete e;
    m_enemies.clear();
    
    if (m_currentStage == 1) {
        Enemy* n = new Pomp(800.0f, 200.0f);
        if (n) { m_enemies.push_back(n); }
    } else {
        Enemy* n = new Pomp(500.0f, 200.0f);
        if (n) { m_enemies.push_back(n); }
        Enemy* n2 = new Pomp(900.0f, 300.0f);
        if (n2) { m_enemies.push_back(n2); }
    }
}

void Game::Update() {
    // 로딩 중에는 업데이트 중단
    if (!m_isLoaded) return;

    DWORD ct = GetTickCount();
    if (ct - m_prevTime < 16) return;
    float dT = (ct - m_prevTime) / 1000.0f;
    Input::Update(); UpdateScreenScale();

    // --- 트랜지션(화면 전환) 로직 ---
    if (m_transitionState != TransitionState::NONE) {
        float transitionSpeed = 3.5f; // 약 0.28초
        if (m_transitionState == TransitionState::ENTERING) {
            m_transitionProgress += dT * transitionSpeed;
            if (m_transitionProgress >= 1.0f) {
                m_transitionProgress = 1.0f;
                m_transitionState = TransitionState::WAITING;
                m_transitionWaitTime = ct;
                
                if (m_transitionToNextStage) {
                    m_gameMode = GameMode::PLAYING;
                    m_player.ClearSnapshots();
                    if (m_currentStage < 2) LoadStage(m_currentStage + 1);
                    else LoadStage(1);
                    m_transitionToNextStage = false;
                }
            }
        } else if (m_transitionState == TransitionState::WAITING) {
            if (ct - m_transitionWaitTime > 150) { // 150ms 대기
                m_transitionState = TransitionState::LEAVING;
                m_transitionProgress = 0.0f;
            }
        } else if (m_transitionState == TransitionState::LEAVING) {
            m_transitionProgress += dT * transitionSpeed;
            if (m_transitionProgress >= 1.0f) {
                m_transitionState = TransitionState::NONE;
                m_transitionProgress = 0.0f;
            }
        }
        m_prevTime = ct;
        return; // 트랜지션 중에는 게임 업데이트 블록
    }

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
    if (m_isStageCleared && m_gameMode == GameMode::PLAYING) {
        if (StageManager::IsInClearZone(m_player.GetX(), m_player.GetY(), m_player.GetColW(), m_player.GetColH())) {
            m_gameMode = GameMode::YES_SCENE;
            m_yesSceneStartTime = GetTickCount();
            m_player.SetState(PlayerState::IDLE);
            EffectManager::Init(); // Replay 진입 전 먼지, 잔상 등 이펙트 제거
            m_player.ClearAfterImages(); // 플레이어 잔상(구르기 등) 제거
            m_prevTime = GetTickCount();
            return;
        }
    }

    if (m_gameMode == GameMode::YES_SCENE) {
        if (ct - m_yesSceneStartTime > 2000) { // 2s display + 0.5s fade out
            m_gameMode = GameMode::REPLAYING;
            m_replayFrame = 0;
            
            // Replay 시작 전 적과 문 상태 초기화
            StageManager::Reset();
            for (auto& e : m_enemies) if (e) e->Reset();

            // Set initial camera to frame 0
            const auto& fullHistory = m_player.GetSnapshots();
            if (!fullHistory.empty()) {
                m_player.SetPos(fullHistory[0].x, fullHistory[0].y);
                m_player.SetState(fullHistory[0].state);
                Camera::Update(m_player.GetX(), m_player.GetY(), m_player.GetColW(), m_player.GetColH(), Input::GetMouseX(), Input::GetMouseY(), m_renderMapScale, StageManager::GetMapWidth(), StageManager::GetMapHeight(), m_isFullMapView, true);
            }
        }
        m_prevTime = ct;
        return; // BLOCK Physics/Input
    }

    if (m_gameMode == GameMode::REPLAYING) {
        if (Input::GetKeyDown(VK_LBUTTON)) {
            m_transitionState = TransitionState::ENTERING;
            m_transitionProgress = 0.0f;
            m_transitionToNextStage = true;
            m_prevTime = ct;
            return;
        }

        if (Input::GetKeyDown(VK_SPACE)) {
            m_isReplayPaused = !m_isReplayPaused;
        }

        if (!m_isReplayPaused) {
            const auto& fullHistory = m_player.GetSnapshots();
            if (m_replayFrame < (int)fullHistory.size()) {
                // 리플레이 이벤트 처리 (속도에 맞춰 스킵되는 프레임의 이벤트도 모두 실행)
                for (int i = 0; i < m_replaySpeed; i++) {
                    int targetFrame = m_replayFrame + i;
                    if (targetFrame < (int)fullHistory.size()) {
                        for (const auto& ev : fullHistory[targetFrame].events) {
                            if (ev.type == Player::ReplayEvent::ENEMY_DIE) {
                                if (ev.targetIdx >= 0 && ev.targetIdx < (int)m_enemies.size()) {
                                    m_enemies[ev.targetIdx]->OnTakeDamage(999.0f);
                                }
                            } else if (ev.type == Player::ReplayEvent::DOOR_OPEN) {
                                // 문 열림 이벤트는 StageManager에서 처리 (인덱스 기반)
                                StageManager::GetStageData(m_currentStage).doors[ev.targetIdx].Open(true, ct);
                            }
                        }
                    }
                }

                const auto& d = fullHistory[m_replayFrame];
                m_player.SetPos(d.x, d.y);
                m_player.SetState(d.state);
                
                // 적 업데이트 추가 (리플레이 중에도 움직이게 함)
                for (auto& e : m_enemies) {
                    if (e) {
                        for (int i = 0; i < m_replaySpeed; i++) {
                            e->Update(1.0f);
                        }
                    }
                }

                bool forceSnap = (m_replayFrame == 0);
                m_replayFrame += m_replaySpeed;
                Camera::Update(m_player.GetX(), m_player.GetY(), m_player.GetColW(), m_player.GetColH(), Input::GetMouseX(), Input::GetMouseY(), m_renderMapScale, StageManager::GetMapWidth(), StageManager::GetMapHeight(), m_isFullMapView, forceSnap);
            } else {
                m_transitionState = TransitionState::ENTERING;
                m_transitionProgress = 0.0f;
                m_transitionToNextStage = true;
            }
        } else {
            // 정지 중에도 카메라는 계속 플레이어를 비추도록 (흔들림 효과 등 유지)
            Camera::Update(m_player.GetX(), m_player.GetY(), m_player.GetColW(), m_player.GetColH(), Input::GetMouseX(), Input::GetMouseY(), m_renderMapScale, StageManager::GetMapWidth(), StageManager::GetMapHeight(), m_isFullMapView, false);
        }
        m_prevTime = ct;
        return; // BLOCK Physics/Input
    }

    if (m_player.GetState() == PlayerState::DEAD && !m_player.IsRewinding()) {
        static bool isDeadShaken = false;
        if (!isDeadShaken) {
            Camera::AddShake(1.5f); // 강도를 약간 높임
            Camera::AddPush(0.0f, 40.0f); // 위아래로 툭 떨어지는 느낌을 위해 수직 푸시 추가
            isDeadShaken = true;
        }
        if (Input::GetKeyDown(VK_LBUTTON)) {
            isDeadShaken = false;
            m_player.StartRewind(m_rewindSpeed); 
            StageManager::Reset(); 
            m_stageTimer = StageManager::GetStageLimitTime(); 
            Camera::StartRewindEffect(); 
            EffectManager::Init(); 
            for (auto& e : m_enemies) if (e) e->Reset(); 
            // 리와인드가 시작되었으므로 아래의 리와인드 업데이트 로직으로 넘어감
        } else {
            // 죽었을 때는 카메라 업데이트만 수행 (정지된 느낌)
            Camera::Update(m_player.GetX(), m_player.GetY(), m_player.GetColW(), m_player.GetColH(), Input::GetMouseX(), Input::GetMouseY(), m_renderMapScale, StageManager::GetMapWidth(), StageManager::GetMapHeight(), m_isFullMapView);
            m_prevTime = ct;
            return;
        }
    }

    if (Input::GetKeyDown('F')) m_isFullMapView = !m_isFullMapView;
    if (Input::GetKeyDown('E')) { m_showDebugRect = !m_showDebugRect; m_showGrid = !m_showGrid; }
    if (Input::GetKeyDown('I')) m_player.SetGodMode(!m_player.IsGodMode());
    
    // 디버그용 스테이지 이동 기능
    if (Input::GetKeyDown('1')) LoadStage(1);
    if (Input::GetKeyDown('2')) LoadStage(2);

    static bool prR = false; bool cuR = GetAsyncKeyState('R') & 0x8000;
    if (cuR && !prR) { 
        m_player.StartRewind(m_rewindSpeed); 
        StageManager::Reset(); 
        m_stageTimer = StageManager::GetStageLimitTime(); // 타이머 리셋
        Camera::StartRewindEffect(); // 카메라 리와인드 효과 시작
        EffectManager::Init(); // 리와인드 시작 시 기존 이펙트(먼지, 잔상 등) 제거
        for (auto& e : m_enemies) if (e) e->Reset(); 
    }
    prR = cuR;

    // 타이머 업데이트 (리와인드 중이 아니고 슬로우 모션이 아닐 때만 감소)
    if (!m_player.IsRewinding() && !m_player.GetIsSlowMo() && !m_isTimePaused) {
        float dT = (ct - m_prevTime) / 1000.0f;
        m_stageTimer -= dT;
        if (m_stageTimer <= 0) {
            m_stageTimer = 0;
            // 타이머가 0이 되면 사망 처리 (갓모드인 경우 제외)
            if (!m_player.IsGodMode() && m_player.GetState() != PlayerState::DEAD) {
                m_player.SetState(PlayerState::DEAD);
            }
        }
    }

    if (m_player.IsRewinding()) {
        m_player.Update(Input::GetMouseX(), Input::GetMouseY(), Camera::GetCamX(), Camera::GetCamY(), m_renderMapScale, m_mapOffsetX, m_mapOffsetY, m_isFullMapView);
        m_player.UpdateAnimation();
        
        bool forceSnap = (m_player.GetHistorySize() == 0);
        Camera::Update(m_player.GetX(), m_player.GetY(), m_player.GetColW(), m_player.GetColH(), Input::GetMouseX(), Input::GetMouseY(), m_renderMapScale, StageManager::GetMapWidth(), StageManager::GetMapHeight(), m_isFullMapView, forceSnap);
        
        // 카메라 리와인드 효과가 종료되었는데도 아직 리와인드 중인 경우 강제 중단 및 초기 위치 이동
        if (!Camera::IsRewindEffectActive()) {
            m_player.StopRewind();
            m_player.SetState(PlayerState::IDLE);
            m_player.SetPos(StageManager::GetPlayerStartX(), StageManager::GetPlayerStartY() - m_player.GetColH() - 2.0f);
            m_player.ClearHistory();
            m_player.ClearSnapshots(); // 사망 또는 재시작(R) 완료 시 리플레이 저장 초기화
            // 스냅된 위치로 카메라 즉시 갱신
            Camera::Update(m_player.GetX(), m_player.GetY(), m_player.GetColW(), m_player.GetColH(), Input::GetMouseX(), Input::GetMouseY(), m_renderMapScale, StageManager::GetMapWidth(), StageManager::GetMapHeight(), m_isFullMapView, true);
        }
        
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
            for (int i = 0; i < (int)m_enemies.size(); i++) {
                Enemy* e = m_enemies[i];
                if (e && e->GetIsAlive()) {
                    RECT eR = e->GetRect(); RECT ol;
                    if (IntersectRect(&ol, &aR, &eR)) {
                        m_isTimePaused = true;
                        // 적 죽음 이벤트를 리플레이에 기록
                        m_player.AddReplayEvent(Player::ReplayEvent::ENEMY_DIE, i);

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
        int openedDoorIdx = StageManager::UpdateDoors(m_player.GetX(), m_player.GetY(), m_player.GetColW(), m_player.GetColH(), (GetAsyncKeyState('A') & 0x8000) != 0, (GetAsyncKeyState('D') & 0x8000) != 0, m_player.GetState() == PlayerState::ATTACK, m_player.GetAttackHitX(), m_player.GetAttackHitY(), m_player.GetAttackHitW(), m_player.GetAttackHitH(), ct, ts);
        if (openedDoorIdx != -1) {
            m_player.AddReplayEvent(Player::ReplayEvent::DOOR_OPEN, openedDoorIdx);
        }
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

    // 로딩 중인 경우 로딩 화면 출력
    if (!m_isLoaded) {
        if (!m_imgLoading.IsNull()) {
            m_imgLoading.Draw(hMemDC, 0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
        }

        // 로딩 게이지 바 배경 (우측 하단)
        int barW = 400;
        int barH = 10;
        int margin = 60;
        RECT barBg = { VIRTUAL_WIDTH - barW - margin, VIRTUAL_HEIGHT - barH - margin, VIRTUAL_WIDTH - margin, VIRTUAL_HEIGHT - margin };
        
        HBRUSH hBgBrush = CreateSolidBrush(RGB(40, 40, 40));
        FillRect(hMemDC, &barBg, hBgBrush);
        DeleteObject(hBgBrush);

        // 로딩 게이지 바 진행도 (Cyan) - 실수 연산으로 정확도 향상
        int currentProgress = m_loadingProgress.load();
        int progressWidth = (int)((float)barW * (currentProgress / 100.0f));
        RECT barProgress = { barBg.left, barBg.top, barBg.left + progressWidth, barBg.bottom };
        HBRUSH hPrgBrush = CreateSolidBrush(RGB(0, 255, 255));
        FillRect(hMemDC, &barProgress, hPrgBrush);
        DeleteObject(hPrgBrush);

        // 퍼센트 텍스트 출력
        SetBkMode(hMemDC, TRANSPARENT);
        SetTextColor(hMemDC, RGB(200, 200, 200));
        HFONT hFont = CreateFont(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
            CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH | FF_SWISS, TEXT("Arial"));
        HFONT hOldFont = (HFONT)SelectObject(hMemDC, hFont);

        TCHAR szProgress[32];
        wsprintf(szProgress, TEXT("%d%%"), currentProgress);
        RECT textRect = { barBg.left, barBg.top - 25, barBg.right, barBg.top };
        DrawText(hMemDC, szProgress, -1, &textRect, DT_RIGHT | DT_SINGLELINE);

        SelectObject(hMemDC, hOldFont);
        DeleteObject(hFont);

        SetStretchBltMode(hDC, HALFTONE);
        StretchBlt(hDC, 0, 0, m_winWidth, m_winHeight, hMemDC, 0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, SRCCOPY);
        SelectObject(hMemDC, hOldBmp); DeleteObject(hMemBmp); DeleteDC(hMemDC);
        return;
    }

    float cX = Camera::GetCamX(), cY = Camera::GetCamY(); Camera::ApplyShake(cX, cY);
    
    // 리와인드 효과 중일 때 맵 높이에 맞춰 카메라 Y좌표를 래핑 (필름 효과)
    if (Camera::IsRewindEffectActive()) {
        int mapH = StageManager::GetMapHeight();
        if (mapH > 0) {
            float wrappedY = fmod(cY, (float)mapH);
            if (wrappedY < 0) wrappedY += (float)mapH;
            cY = wrappedY;
        }
    }

    StageManager::Render(hMemDC, m_isFullMapView, m_showDebugRect, mapScale, m_renderMapScale, m_mapOffsetX, m_mapOffsetY, cX, cY, VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
    if (m_showGrid) {
        HPEN hGP = CreatePen(PS_SOLID, 1, RGB(100, 100, 100)); HPEN hOP = (HPEN)SelectObject(hMemDC, hGP);
        for (int x = 0; x <= VIRTUAL_WIDTH; x += 20) { MoveToEx(hMemDC, x, 0, NULL); LineTo(hMemDC, x, VIRTUAL_HEIGHT); }
        for (int y = 0; y <= VIRTUAL_HEIGHT; y += 20) { MoveToEx(hMemDC, 0, y, NULL); LineTo(hMemDC, VIRTUAL_WIDTH, y); }
        SelectObject(hMemDC, hOP); DeleteObject(hGP);
    }
    for (auto& e : m_enemies) if (e) e->Render(hMemDC, cX, cY, mapScale, m_showDebugRect);
    if (m_player.GetIsSlowMo() && m_gameMode != GameMode::REPLAYING) {
        Gdiplus::Graphics g(hMemDC); Gdiplus::Rect fr(0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
        Gdiplus::GraphicsPath p; p.AddRectangle(fr); Gdiplus::PathGradientBrush pgb(&p);
        pgb.SetCenterColor(Gdiplus::Color(0, 0, 0, 0)); pgb.SetCenterPoint(Gdiplus::PointF(VIRTUAL_WIDTH / 2.0f, VIRTUAL_HEIGHT / 2.0f));
        Gdiplus::Color ec[] = { Gdiplus::Color(180, 0, 0, 0) }; int cnt = 1; pgb.SetSurroundColors(ec, &cnt);
        pgb.SetFocusScales(0.2f, 0.2f); g.FillRectangle(&pgb, fr);
    }

    // 타임아웃 경고 효과 (남은 시간 3초 이하일 때 핑크/보라색 오버레이)
    if (m_stageTimer <= 3.0f && m_bGameStarted && !m_player.IsRewinding() && m_gameMode != GameMode::REPLAYING) {
        float alphaRatio = 1.0f - (m_stageTimer / 3.0f); // 3초일 때 0, 0초일 때 1
        if (alphaRatio < 0) alphaRatio = 0;
        if (alphaRatio > 1.0f) alphaRatio = 1.0f;

        // 사망 시에는 완전히 덮음, 그 외에는 최대 150 알파까지 서서히 증가
        int alpha = m_player.GetState() == PlayerState::DEAD ? 200 : (int)(alphaRatio * 150);
        
        Gdiplus::Graphics g(hMemDC);
        // 핑크와 보라 사이의 색상 (예: R:255, G:50, B:200)
        Gdiplus::SolidBrush warningBrush(Gdiplus::Color(alpha, 255, 50, 200));
        g.FillRectangle(&warningBrush, 0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
    }
    float fW = (float)VIRTUAL_WIDTH, fH = (float)VIRTUAL_HEIGHT, fCW = (float)StageManager::GetMapWidth(), fCH = (float)StageManager::GetMapHeight();
    float scX = fW / fCW, scY = fH / fCH, cFS = (scX < scY) ? scX : scY;
    float cFX = (fW - fCW * cFS) / 2.0f, cFY = (fH - fCH * cFS) / 2.0f;
    EffectManager::Render(hMemDC, cX, cY, mapScale, m_isFullMapView, cFS, cFX, cFY);
    m_player.Render(hMemDC, cX, cY, mapScale, playerScale, m_renderMapScale, m_mapOffsetX, m_mapOffsetY, m_isFullMapView, m_showDebugRect);
    
    auto drawTransition = [&](HDC dc) {
        if (m_transitionState == TransitionState::NONE) return;
        HBRUSH hBlack = CreateSolidBrush(RGB(0, 0, 0));
        RECT tr = { 0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT };
        if (m_transitionState == TransitionState::ENTERING || m_transitionState == TransitionState::WAITING) {
            tr.left = (int)(VIRTUAL_WIDTH * (1.0f - m_transitionProgress));
        } else if (m_transitionState == TransitionState::LEAVING) {
            tr.right = (int)(VIRTUAL_WIDTH * (1.0f - m_transitionProgress));
        }
        FillRect(dc, &tr, hBlack);
        DeleteObject(hBlack);
    };

    if (m_gameMode == GameMode::REPLAYING || m_gameMode == GameMode::YES_SCENE) {
        // 더블 버퍼링 유지를 위한 포스트 프로세싱 버퍼 생성
        HDC hPostDC = CreateCompatibleDC(hDC);
        HBITMAP hPostBmp = CreateCompatibleBitmap(hDC, VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
        HBITMAP hOldPostBmp = (HBITMAP)SelectObject(hPostDC, hPostBmp);

        // hMemDC(원본) -> hPostDC(흑백) 복사
        Gdiplus::Graphics g(hPostDC);
        Gdiplus::Bitmap bmp(hMemBmp, NULL);
        Gdiplus::ImageAttributes attr;
        Gdiplus::ColorMatrix mat = { 
            0.3f, 0.3f, 0.3f, 0, 0, 
            0.59f, 0.59f, 0.59f, 0, 0, 
            0.11f, 0.11f, 0.11f, 0, 0, 
            0, 0, 0, 1, 0, 
            0, 0, 0, 0, 1 
        };
        attr.SetColorMatrix(&mat);
        g.DrawImage(&bmp, Gdiplus::Rect(0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT), 0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, Gdiplus::UnitPixel, &attr);
        
        // UI 오버레이 그리기 (가상 해상도 기준)
        if (m_gameMode == GameMode::REPLAYING) {
            if (!m_imgReplayUI[0].IsNull()) {
                int w = m_imgReplayUI[0].GetWidth();
                int h = m_imgReplayUI[0].GetHeight();
                m_imgReplayUI[0].Draw(hPostDC, VIRTUAL_WIDTH - w - 20, VIRTUAL_HEIGHT - h - 20, w, h); // 우측 하단
            }
            
            int topUIIdx = m_isReplayPaused ? 2 : 1;
            if (!m_imgReplayUI[topUIIdx].IsNull()) {
                int w = m_imgReplayUI[topUIIdx].GetWidth();
                int h = m_imgReplayUI[topUIIdx].GetHeight();
                m_imgReplayUI[topUIIdx].Draw(hPostDC, 20, 20, w, h); // 좌측 상단
            }
        } else if (m_gameMode == GameMode::YES_SCENE) {
            DWORD elapsed = GetTickCount() - m_yesSceneStartTime;
            int alpha = 255;
            if (elapsed > 2000) {
                alpha = 255 - (int)((elapsed - 2000) / 500.0f * 255);
                if (alpha < 0) alpha = 0;
            }
            if (alpha > 0) {
                Gdiplus::SolidBrush blackBrush(Gdiplus::Color(alpha, 0, 0, 0));
                g.FillRectangle(&blackBrush, 0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
                
                if (!m_imgReplayUI[3].IsNull()) {
                    int w = m_imgReplayUI[3].GetWidth() / 2;
                    int h = m_imgReplayUI[3].GetHeight() / 2;
                    Gdiplus::ImageAttributes yesAttr;
                    Gdiplus::ColorMatrix yesMat = { 
                        1, 0, 0, 0, 0, 
                        0, 1, 0, 0, 0, 
                        0, 0, 1, 0, 0, 
                        0, 0, 0, alpha / 255.0f, 0, 
                        0, 0, 0, 0, 1 
                    };
                    yesAttr.SetColorMatrix(&yesMat);
                    HBITMAP hBmpYes = m_imgReplayUI[3];
                    Gdiplus::Bitmap bmpYes(hBmpYes, NULL);
                    int x = (VIRTUAL_WIDTH - w) / 2;
                    int y = (VIRTUAL_HEIGHT - h) / 2;
                    g.DrawImage(&bmpYes, Gdiplus::Rect(x, y, w, h), 0, 0, bmpYes.GetWidth(), bmpYes.GetHeight(),
                        Gdiplus::UnitPixel, &yesAttr);
                }
            }
        }

        drawTransition(hPostDC);

        // 최종 화면 출력
        SetStretchBltMode(hDC, HALFTONE);
        StretchBlt(hDC, 0, 0, m_winWidth, m_winHeight, hPostDC, 0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, SRCCOPY);

        SelectObject(hPostDC, hOldPostBmp); DeleteObject(hPostBmp); DeleteDC(hPostDC);
    } else {
        if (!m_player.IsRewinding()) {
            bool isShift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
            UIManager::Render(hMemDC, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, Input::GetMouseX(), Input::GetMouseY(), m_player.GetBatteryLevel(), m_stageTimer, StageManager::GetStageLimitTime(), m_bGameStarted, isShift, m_player.GetState() == PlayerState::DEAD);
        }

        drawTransition(hMemDC);

        SetStretchBltMode(hDC, HALFTONE);
        StretchBlt(hDC, 0, 0, m_winWidth, m_winHeight, hMemDC, 0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, SRCCOPY);
    }

    
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
