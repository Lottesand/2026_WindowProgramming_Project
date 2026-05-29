#include <windows.h>
#include <atlimage.h> 
#include <math.h>
#include <algorithm>
#include <vector>
#include <time.h>
#include <objidl.h>
#include <gdiplus.h>
#pragma comment(lib, "Gdiplus.lib")
#include "Enemy.h"
#include "Player.h"
#include "Physics.h"

using namespace std;

// ==============================================================================
// 🛠️ [구조체 정의]
// ==============================================================================

struct EnemySpawnInfo {
    EnemyType type;
    float x;
    float y;
};

// [네온 궤적 이펙트 구조체]
struct NeonTrail {
    float x, y;
    float startX, startY;
    float endX, endY;
    float dirX, dirY;
    float angle;
    float length;
    float maxLength;
    int life;
    int maxLife;
};

// [타격 시각 효과 구조체]
struct HitVFX {
    float x, y;
    float angle;
    int currentFrame;
    int maxFrame;
    DWORD lastTime;
    bool isSlash; // true: slashfx, false: hitimpact
};

// [대기 중인 타격 판정 (VFX 종료 후 적용)]
struct PendingHit {
    Enemy* target;
    float kbForce;
    int remainingFrames;
};

// ==============================================================================
// 🛠️ [튜닝 변수 모음] 
// ==============================================================================

// [기본 스케일 및 위치 설정]
float mapScale = 1.02f;            // 월드 기본 배율 (1.01배 확대, 약 10px 정도 타이트하게)
float playerScale = 2.0f;          // 플레이어 스프라이트 출력 배율
float camY_Fixed = 60.0f;          // 카메라의 고정된 Y축 높이값 (지면 기준)

// [카메라 제어 및 마우스 트래킹]
float g_camLookAheadX = 150.0f;    // 마우스 위치에 따라 카메라가 플레이어보다 앞서가는 최대 거리
float g_camLerpSpeedX = 0.08f;     // 카메라 가로 이동의 부드러움 정도 (높을수록 빠름)
float g_camLerpSpeedY = 0.08f;     // 카메라 세로 이동의 부드러움 정도

// [플레이어 렌더링 및 맵 보정]
float g_renderMapScale = 1.0f;     // 전체 맵 보기(F키) 모드에서 계산된 렌더링 배율
float g_renderPlayerScale = 2.0f;  // 전체 맵 보기 모드에서 적용되는 플레이어 크기 배율
float g_mapOffsetX = 0.0f;         // 전체 맵 보기 시 화면 중앙 정렬을 위한 X축 오프셋
float g_mapOffsetY = 0.0f;         // 전체 맵 보기 시 화면 중앙 정렬을 위한 Y축 오프셋

// [전투 연출 및 VFX 설정]
float g_slashWidth = 10.0f;        // 네온 슬래시(공격 이펙트)의 궤적 두께
float g_shakeIntensity = 10.0f;    // 화면 흔들림(Shake)의 강도 (10px 이내)
float g_hitShakeForce = 15.0f;     // 적 타격 시 공격 방향으로 카메라가 밀려나는 힘의 크기
float g_shakeDecay = 0.85f;        // 흔들림 감쇄율 (매 프레임마다 강도가 줄어드는 비율)
    
// [실시간 카메라 상태 관리 변수]
float g_curShakeX = 0.0f;          // 현재 프레임에 적용된 실제 흔들림 X 오프셋
float g_curShakeY = 0.0f;          // 현재 프레임에 적용된 실제 흔들림 Y 오프셋
float g_camPushX = 0.0f;           // 타격 시 발생하는 일시적인 카메라 밀림 X값
float g_camPushY = 0.0f;           // 타격 시 발생하는 일시적인 카메라 밀림 Y값
float g_shakeTrauma = 0.3f;        // 흔들림의 누적 강도 (0.0 ~ 1.0, 시간에 따라 소멸)

// [타격 타이밍 및 수명 설정]
int g_neonTrailLife = 6;           // 네온 궤적 및 역경직 지속 시간 (프레임)
int g_playerAttackCooldown = 150;  // 플레이어 공격 재사용 대기시간 (ms)
int g_playerAttackDuration = 2;    // 플레이어 공격 애니메이션 지속 프레임 (후딜 조절용)
int g_playerAfterImageInterval = 15; // 잔상 생성 간격 (ms)
float g_timeSlowScale = 0.3f;        // 슬로우 모션 시 시간 흐름 배율 (0.1 ~ 1.0)
int g_slowMoDurationLimit = 5000;    // 슬로우 모션 지속 시간 (ms)
float g_slowMoJumpForceScale = 1.2f;     // 슬로우 모션 시 점프 궤적 보정 (1.0이면 일반과 동일, 1.2면 약간 더 높게)
float g_slowMoMoveForceScale = 1.0f;     // 슬로우 모션 시 이동 궤적 보정 (1.0이면 일반과 동일한 거리)
int g_maxJumpHoldTime = 200;         // 점프 키 유지가 인정되는 최대 시간 (ms)


// ==============================================================================
// 🛠️ [전역 변수 및 상태 정의]
// ==============================================================================

HINSTANCE g_hInst;
HWND g_hWnd;
LPCTSTR lpszClass = L"My Window Class";
LPCTSTR lpszWindowName = L"Window Programming Lab";

std::vector<Enemy*> g_Enemies;
Player g_Player;

bool g_isTimePaused = false;
std::vector<NeonTrail> g_NeonTrails;
std::vector<HitVFX> g_HitVFXs;
std::vector<PendingHit> g_PendingHits;

std::vector<EnemySpawnInfo> g_EnemySpawns = {
    { EnemyType::POMP, 800.0f, 300.0f }
};

const int VIRTUAL_WIDTH = 1280;
const int VIRTUAL_HEIGHT = 720;
int WIN_WIDTH = 1280;
int WIN_HEIGHT = 720;

bool g_showDebugRect = false;
bool g_showGrid = false;
bool g_prevEState = false;
bool g_isFullMapView = false;
bool g_prevFState = false;

int mouseX = VIRTUAL_WIDTH / 2;
int mouseY = VIRTUAL_HEIGHT / 2;
float camX = 0.0f;
float camY = 60.0f;

CImage imgMap, imgColMap;
CImage imgHudBase, imgHudBattery, imgHudTimer, imgHudInven;
CImage imgCursor;
CImage imgVfxSlash[5], imgVfxHit[6];

// --- [함수 선언] ---
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void LoadAssets();
void UpdateScreenScale();
void UpdateCamera();
void Render(HDC hDC);
void SpawnEnemies();

// ==============================================================================
// 🛠️ [WinMain 및 메인 루프]
// ==============================================================================

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdParam, int nCmdShow) {
    WNDCLASSEX WndClass;
    g_hInst = hInstance;
    WndClass.cbSize = sizeof(WndClass);
    WndClass.style = CS_HREDRAW | CS_VREDRAW;
    WndClass.lpfnWndProc = (WNDPROC)WndProc;
    WndClass.cbClsExtra = 0;
    WndClass.cbWndExtra = 0;
    WndClass.hInstance = hInstance;
    WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    WndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    WndClass.lpszMenuName = NULL;
    WndClass.lpszClassName = lpszClass;
    WndClass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    RegisterClassEx(&WndClass);

    RECT wr = { 0, 0, WIN_WIDTH, WIN_HEIGHT };
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

    g_hWnd = CreateWindow(lpszClass, TEXT("Katana Zero Rebirth"), WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, wr.right - wr.left, wr.bottom - wr.top,
        NULL, (HMENU)NULL, hInstance, NULL);

    ShowWindow(g_hWnd, nCmdShow);
    srand((unsigned int)time(NULL));

    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    MSG msg;
    DWORD prevTime = GetTickCount();

    while (true) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) break;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            DWORD currentTime = GetTickCount();
            if (currentTime - prevTime >= 16) {
                UpdateScreenScale();

                bool curF = (GetAsyncKeyState('F') & 0x8000) != 0;
                if (curF && !g_prevFState) g_isFullMapView = !g_isFullMapView;
                g_prevFState = curF;

                bool curE = (GetAsyncKeyState('E') & 0x8000) != 0;
                if (curE && !g_prevEState) { g_showDebugRect = !g_showDebugRect; g_showGrid = !g_showGrid; }
                g_prevEState = curE;

                static bool prevR = false;
                bool curR = (GetAsyncKeyState('R') & 0x8000) != 0;
                if (curR && !prevR) SpawnEnemies();
                prevR = curR;

                if (!g_isTimePaused) {
                    g_Player.Update(mouseX, mouseY, camX, camY, g_renderMapScale, g_mapOffsetX, g_mapOffsetY, g_isFullMapView);
                }

                float currentTimeScale = g_Player.GetIsSlowMo() ? g_timeSlowScale : 1.0f;

                static int lastAttackFrame = -1;
                if (!g_isTimePaused && g_Player.GetState() == PlayerState::ATTACK) {
                    int curFrame = g_Player.GetCurrentFrame();
                    if (curFrame >= 1 && curFrame <= 3 && curFrame != lastAttackFrame) {
                        float hitW = g_Player.GetAttackHitW();
                        float hitH = g_Player.GetAttackHitH();
                        float hitOff = g_Player.GetAttackHitOffset();
                        float adX = g_Player.GetAttackDirX();
                        float adY = g_Player.GetAttackDirY();
                        float centerX = g_Player.GetX() + g_Player.GetColW() / 2.0f;
                        float centerY = g_Player.GetY() + g_Player.GetColH() / 2.0f;
                        float hitX = centerX + adX * hitOff - hitW / 2.0f;
                        float hitY = centerY + adY * hitOff - hitH / 2.0f;
                        RECT aRect = { (int)hitX, (int)hitY, (int)(hitX + hitW), (int)(hitY + hitH) };

                        for (auto& enemyPtr : g_Enemies) {
                            if (enemyPtr && enemyPtr->GetIsAlive()) {
                                RECT eRect = enemyPtr->GetRect();
                                RECT overlap;
                                if (IntersectRect(&overlap, &aRect, &eRect)) {
                                    g_isTimePaused = true;
                                    float ex = enemyPtr->GetX() + enemyPtr->GetColW() / 2.0f;
                                    float ey = enemyPtr->GetY() + enemyPtr->GetColH() / 2.0f;
                                    float dx = ex - centerX;
                                    float dy = ey - centerY;
                                    float dist = sqrt(dx * dx + dy * dy);
                                    if (dist < 1.0f) dist = 1.0f;
                                    float ux = dx / dist;
                                    float uy = dy / dist;

                                    // 1. Neon Trail 생성 (데이터 완전 초기화)
                                    NeonTrail trail;
                                    trail.x = ex;
                                    trail.y = ey;
                                    trail.startX = ex - ux * 1000.0f;
                                    trail.startY = ey - uy * 1000.0f;
                                    trail.endX = ex + ux * 3000.0f;
                                    trail.endY = ey + uy * 3000.0f;
                                    trail.dirX = ux;
                                    trail.dirY = uy;
                                    trail.angle = atan2(uy, ux);
                                    trail.length = 0.0f;
                                    trail.maxLength = 4000.0f;
                                    trail.life = g_neonTrailLife;
                                    trail.maxLife = g_neonTrailLife;
                                    g_NeonTrails.push_back(trail);

                                    // 2. HitVFX (Slash & Impact) 생성
                                    HitVFX slash;
                                    slash.x = ex; slash.y = ey; slash.angle = trail.angle;
                                    slash.currentFrame = 0; slash.maxFrame = 5;
                                    slash.lastTime = currentTime; slash.isSlash = true;
                                    g_HitVFXs.push_back(slash);

                                    HitVFX impact;
                                    impact.x = ex; impact.y = ey; impact.angle = trail.angle;
                                    impact.currentFrame = 0; impact.maxFrame = 6;
                                    impact.lastTime = currentTime; impact.isSlash = false;
                                    g_HitVFXs.push_back(impact);

                                    // 3. 카메라 흔들림 트리거
                                    g_camPushX = ux * g_hitShakeForce * 2.0f;
                                    g_camPushY = uy * g_hitShakeForce * 2.0f;
                                    g_shakeTrauma = 1.0f;

                                    // 4. 지연된 타격 판정 (VFX 끝난 후)
                                    PendingHit ph;
                                    ph.target = enemyPtr;
                                    ph.kbForce = (adX >= 0) ? 45.0f : -45.0f;
                                    ph.remainingFrames = 6;
                                    g_PendingHits.push_back(ph);
                                }
                            }
                        }
                        lastAttackFrame = curFrame;
                    }
                }
                else if (!g_isTimePaused) lastAttackFrame = -1;
                
                if (!g_isTimePaused) {
                    for (auto& enemyObj : g_Enemies) if (enemyObj) {
                        enemyObj->Update(currentTimeScale); 
                    }
                }

                // --- [VFX 업데이트 로직] ---
                bool hasActiveVFX = false;

                // 네온 궤적 업데이트 (시간 배율 적용)
                for (auto it = g_NeonTrails.begin(); it != g_NeonTrails.end(); ) {
                    it->life -= (int)(1.0f / currentTimeScale); // 슬로우 시 수명 천천히 감소
                    if (it->life <= 0) it = g_NeonTrails.erase(it);
                    else { 
                        it->length += (it->maxLength / (it->maxLife / 2.0f)) * currentTimeScale; 
                        hasActiveVFX = true; 
                        it++; 
                    }
                }

                // 이미지 VFX 업데이트 (시간 배율 적용)
                for (auto it = g_HitVFXs.begin(); it != g_HitVFXs.end(); ) {
                    if (currentTime - it->lastTime >= (DWORD)(40 / currentTimeScale)) { 
                        it->currentFrame++; it->lastTime = currentTime; 
                    }
                    if (it->currentFrame >= it->maxFrame) it = g_HitVFXs.erase(it);
                    else { hasActiveVFX = true; it++; }
                }

                // 대기 중인 타격 적용
                for (auto it = g_PendingHits.begin(); it != g_PendingHits.end(); ) {
                    it->remainingFrames--;
                    if (it->remainingFrames <= 0) {
                        if (it->target && it->target->GetIsAlive()) {
                            it->target->OnTakeDamage(1.0f);
                            it->target->ApplyKnockback(it->kbForce);
                        }
                        it = g_PendingHits.erase(it);
                    }
                    else it++;
                }

                if (g_isTimePaused && !hasActiveVFX) g_isTimePaused = false;

                if (!g_isTimePaused) g_Player.UpdateAnimation();
                UpdateCamera();
                InvalidateRect(g_hWnd, NULL, FALSE);
                prevTime = currentTime;
            }
        }
    }

    Gdiplus::GdiplusShutdown(gdiplusToken);
    return (int)msg.wParam;
}

// ==============================================================================
// 🛠️ [헬퍼 함수 구현]
// ==============================================================================

void SpawnEnemies() {
    for (auto& enemyPtr : g_Enemies) if (enemyPtr) delete enemyPtr;
    g_Enemies.clear();
    for (const auto& info : g_EnemySpawns) {
        Enemy* newEnemy = nullptr;
        switch (info.type) {
        case EnemyType::GANGSTER:  newEnemy = new Gangster(info.x, info.y); break;
        case EnemyType::GRUNT:     newEnemy = new Grunt(info.x, info.y); break;
        case EnemyType::POMP:      newEnemy = new Pomp(info.x, info.y); break;
        case EnemyType::SHIELDCOP: newEnemy = new ShieldCop(info.x, info.y); break;
        }
        if (newEnemy) { newEnemy->Init(); g_Enemies.push_back(newEnemy); }
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam) {
    switch (iMsg) {
    case WM_CREATE: ShowCursor(FALSE); LoadAssets(); g_Player.Init(); SpawnEnemies(); break;
    case WM_SIZE: WIN_WIDTH = LOWORD(lParam); WIN_HEIGHT = HIWORD(lParam); break;
    case WM_ERASEBKGND: return 1;
    case WM_MOUSEMOVE:
        if (WIN_WIDTH != 0 && WIN_HEIGHT != 0) {
            mouseX = (int)(LOWORD(lParam) * ((float)VIRTUAL_WIDTH / WIN_WIDTH));
            mouseY = (int)(HIWORD(lParam) * ((float)VIRTUAL_HEIGHT / WIN_HEIGHT));
        }
        break;
    case WM_PAINT: { PAINTSTRUCT ps; HDC hDC = BeginPaint(hWnd, &ps); Render(hDC); EndPaint(hWnd, &ps); break; }
    case WM_DESTROY: for (auto& e : g_Enemies) if (e) delete e; g_Enemies.clear(); PostQuitMessage(0); break;
    }
    return DefWindowProc(hWnd, iMsg, wParam, lParam);
}

void LoadAssets() {
    imgMap.Load(TEXT("assets/map.png")); imgColMap.Load(TEXT("assets/colmap.png"));
    imgCursor.Load(TEXT("assets/cursor.png")); imgHudBase.Load(TEXT("assets/hud/base.png"));
    imgHudBattery.Load(TEXT("assets/hud/battery.png")); imgHudTimer.Load(TEXT("assets/hud/timer.png"));
    imgHudInven.Load(TEXT("assets/hud/inven.png"));
    wchar_t path[256];
    for (int i = 0; i < 5; ++i) { swprintf_s(path, L"assets/spr_slashfx/%d.png", i); imgVfxSlash[i].Load(path); }
    for (int i = 0; i < 6; ++i) { swprintf_s(path, L"assets/spr_hit_impact/%d.png", i); imgVfxHit[i].Load(path); }
}

void UpdateScreenScale() {
    g_renderMapScale = mapScale; g_renderPlayerScale = playerScale;
    g_mapOffsetX = 0.0f; g_mapOffsetY = 0.0f;
    int mapW = imgMap.IsNull() ? VIRTUAL_WIDTH : imgMap.GetWidth();
    int mapH = imgMap.IsNull() ? VIRTUAL_HEIGHT : imgMap.GetHeight();
    if (g_isFullMapView && !imgMap.IsNull()) {
        float scaleX = (float)VIRTUAL_WIDTH / mapW; float scaleY = (float)VIRTUAL_HEIGHT / mapH;
        g_renderMapScale = (scaleX < scaleY) ? scaleX : scaleY;
        g_renderPlayerScale = playerScale * (g_renderMapScale / mapScale);
        g_mapOffsetX = (VIRTUAL_WIDTH - (mapW * g_renderMapScale)) / 2.0f;
        g_mapOffsetY = (VIRTUAL_HEIGHT - (mapH * g_renderMapScale)) / 2.0f;
    }
}

void UpdateCamera() {
    if (g_isFullMapView) return;
    float mOffX = (float)(mouseX - (VIRTUAL_WIDTH / 2)) / (VIRTUAL_WIDTH / 2);
    float pMidX = g_Player.GetX() + g_Player.GetColW() / 2.0f;
    float vHW = (VIRTUAL_WIDTH / g_renderMapScale) / 2.0f;
    float tCamX = pMidX - vHW + (mOffX * g_camLookAheadX);
    camX += (tCamX - camX) * g_camLerpSpeedX;
    camY += (camY_Fixed - camY) * g_camLerpSpeedY;
    g_camPushX *= g_shakeDecay; g_camPushY *= g_shakeDecay;
    float trSq = g_shakeTrauma * g_shakeTrauma;
    if (trSq > 0.001f) {
        g_curShakeX = ((float)(rand() % 100) / 50.0f - 1.0f) * g_shakeIntensity * trSq;
        g_curShakeY = ((float)(rand() % 100) / 50.0f - 1.0f) * g_shakeIntensity * trSq;
    } else { g_curShakeX = 0; g_curShakeY = 0; }
    g_shakeTrauma *= g_shakeDecay; if (g_shakeTrauma < 0.01f) g_shakeTrauma = 0;
    if (camX < 0) camX = 0; if (camY < 0) camY = 0;
    if (!imgMap.IsNull()) {
        float vW = VIRTUAL_WIDTH / g_renderMapScale; float vH = VIRTUAL_HEIGHT / g_renderMapScale;
        float maxCX = (float)imgMap.GetWidth() - vW; if (camX > maxCX) camX = maxCX;
        float maxCY = (float)imgMap.GetHeight() - vH; if (camY > maxCY) camY = maxCY;
        if (camX < 0) camX = 0; if (camY < 0) camY = 0;
    }
}

void Render(HDC hDC) {
    HDC hMemDC = CreateCompatibleDC(hDC);
    HBITMAP hMemBmp = CreateCompatibleBitmap(hDC, VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
    HBITMAP hOldBmp = (HBITMAP)SelectObject(hMemDC, hMemBmp);
    SetGraphicsMode(hMemDC, GM_ADVANCED);
    PatBlt(hMemDC, 0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, BLACKNESS);
    float oCX = camX, oCY = camY;
    camX += (g_curShakeX + g_camPushX); camY += (g_curShakeY + g_camPushY);
    int cMW = imgMap.IsNull() ? VIRTUAL_WIDTH : imgMap.GetWidth();
    int cMH = imgMap.IsNull() ? VIRTUAL_HEIGHT : imgMap.GetHeight();
    if (!imgMap.IsNull()) {
        CImage* tMap = g_showDebugRect ? &imgColMap : &imgMap;
        if (g_isFullMapView) tMap->Draw(hMemDC, (int)g_mapOffsetX, (int)g_mapOffsetY, (int)(cMW * g_renderMapScale), (int)(cMH * g_renderMapScale), 0, 0, cMW, cMH);
        else tMap->Draw(hMemDC, 0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, (int)camX, (int)camY, (int)(VIRTUAL_WIDTH / g_renderMapScale), (int)(VIRTUAL_HEIGHT / g_renderMapScale));
    }
    if (g_showGrid) {
        HPEN hGP = CreatePen(PS_SOLID, 1, RGB(100, 100, 100)); HPEN hOP = (HPEN)SelectObject(hMemDC, hGP);
        for (int x = 0; x <= VIRTUAL_WIDTH; x += 20) { MoveToEx(hMemDC, x, 0, NULL); LineTo(hMemDC, x, VIRTUAL_HEIGHT); }
        for (int y = 0; y <= VIRTUAL_HEIGHT; y += 20) { MoveToEx(hMemDC, 0, y, NULL); LineTo(hMemDC, VIRTUAL_WIDTH, y); }
        SelectObject(hMemDC, hOP); DeleteObject(hGP);
    }
    for (auto& e : g_Enemies) if (e) e->Render(hMemDC);
    float fW = (float)VIRTUAL_WIDTH, fH = (float)VIRTUAL_HEIGHT;
    float fCW = (float)cMW, fCH = (float)cMH;
    float scX = fW / fCW, scY = fH / fCH;
    float cFS = (scX < scY) ? scX : scY;
    float cFX = (fW - fCW * cFS) / 2.0f; float cFY = (fH - fCH * cFS) / 2.0f;

    // --- [슬로우 모션 비네팅(Vignette) 효과] ---
    // 플레이어와 VFX보다 먼저 그려서, 플레이어/VFX는 밝게 유지하고 배경/적만 어둡게 처리
    if (g_Player.GetIsSlowMo()) {
        Gdiplus::Graphics graphics(hMemDC);
        Gdiplus::Rect fullRect(0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
        
        Gdiplus::GraphicsPath path;
        path.AddRectangle(fullRect);

        Gdiplus::PathGradientBrush pgb(&path);
        Gdiplus::Color centerColor(0, 0, 0, 0); // 중심은 투명
        pgb.SetCenterColor(centerColor);
        pgb.SetCenterPoint(Gdiplus::PointF(VIRTUAL_WIDTH / 2.0f, VIRTUAL_HEIGHT / 2.0f));

        Gdiplus::Color edgeColors[] = { Gdiplus::Color(180, 0, 0, 0) }; // 가장자리는 불투명한 검정 (알파 180)
        int count = 1;
        pgb.SetSurroundColors(edgeColors, &count);
        
        pgb.SetFocusScales(0.2f, 0.2f); 

        graphics.FillRectangle(&pgb, fullRect);
    }

    // 1. 네온 궤적 (Neon Trail) 렌더링 - 슬로우 모션 아닐 때만 렌더링
    if (!g_Player.GetIsSlowMo()) {
        for (const auto& tr : g_NeonTrails) {
            float pFS = g_isFullMapView ? g_renderMapScale : mapScale;
            bool isP = (GetTickCount() / 50) % 2 == 0; 
            COLORREF dC = isP ? RGB(255, 0, 255) : RGB(0, 255, 255);
            int segs = 20; 
            float sLen = tr.length / segs;
            for (int i = 0; i < segs; ++i) {
                float sD = i * sLen, eD = (i + 1) * sLen;
                float eA = 1.0f, prog = (float)i / segs;
                if (prog < 0.15f) eA = prog / 0.15f; else if (prog > 0.85f) eA = (1.0f - prog) / 0.15f;
                float sX = tr.startX + tr.dirX * sD, sY = tr.startY + tr.dirY * sD;       
                float eX = tr.startX + tr.dirX * eD, eY = tr.startY + tr.dirY * eD;
                float vSX, vSY, vEX, vEY;
                if (g_isFullMapView) { vSX = sX * cFS + cFX; vSY = sY * cFS + cFY; vEX = eX * cFS + cFX; vEY = eY * cFS + cFY; }
                else { vSX = (sX - camX) * mapScale; vSY = (sY - camY) * mapScale; vEX = (eX - camX) * mapScale; vEY = (eY - camY) * mapScale; }
                float bW = g_slashWidth * pFS * (tr.life / (float)tr.maxLife) * 1.5f; if (bW < 1.0f) bW = 1.0f;
                COLORREF fC = RGB((int)(GetRValue(dC) * eA), (int)(GetGValue(dC) * eA), (int)(GetBValue(dC) * eA));
                HPEN hP = CreatePen(PS_SOLID, (int)bW, fC); 
                HPEN hOP = (HPEN)SelectObject(hMemDC, hP);
                MoveToEx(hMemDC, (int)vSX, (int)vSY, NULL); LineTo(hMemDC, (int)vEX, (int)vEY);
                SelectObject(hMemDC, hOP); DeleteObject(hP);
            }
        }
    }

    // 2. 이미지 VFX (Slash & Impact) 렌더링
    for (const auto& v : g_HitVFXs) {
        CImage* vI = v.isSlash ? &imgVfxSlash[v.currentFrame] : &imgVfxHit[v.currentFrame];
        if (vI && !vI->IsNull()) {
            float customMultiplier = v.isSlash ? 1.5f : 1.8f;
            float vfxScale = playerScale * (g_isFullMapView ? g_renderMapScale : mapScale) * customMultiplier;
            int vW = (int)(vI->GetWidth() * vfxScale); 
            int vH = (int)(vI->GetHeight() * vfxScale);
            float dX, dY;
            if (g_isFullMapView) { dX = v.x * cFS + cFX; dY = v.y * cFS + cFY; }
            else { dX = (v.x - camX) * mapScale; dY = (v.y - camY) * mapScale; }
            XFORM xF; xF.eM11 = cos(v.angle); xF.eM12 = sin(v.angle); xF.eM21 = -sin(v.angle); xF.eM22 = cos(v.angle); xF.eDx = dX; xF.eDy = dY;
            SetWorldTransform(hMemDC, &xF); vI->Draw(hMemDC, -vW / 2, -vH / 2, vW, vH);
            XFORM xFI = { 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f }; SetWorldTransform(hMemDC, &xFI);
        }
    }

    g_Player.Render(hMemDC, camX, camY, mapScale, playerScale, g_renderMapScale, g_mapOffsetX, g_mapOffsetY, g_isFullMapView, g_showDebugRect);

    if (!imgHudBase.IsNull()) imgHudBase.Draw(hMemDC, 0, 0, imgHudBase.GetWidth() * 2, imgHudBase.GetHeight() * 2);
    if (!imgHudBattery.IsNull()) imgHudBattery.Draw(hMemDC, 10, 5, imgHudBattery.GetWidth() * 2, imgHudBattery.GetHeight() * 2);
    if (!imgHudTimer.IsNull()) imgHudTimer.Draw(hMemDC, (VIRTUAL_WIDTH / 2 - imgHudTimer.GetWidth() - 10), 0, imgHudTimer.GetWidth() * 2, imgHudTimer.GetHeight() * 2);
    if (!imgHudInven.IsNull()) imgHudInven.Draw(hMemDC, VIRTUAL_WIDTH - imgHudInven.GetWidth() - 80, 0, imgHudInven.GetWidth() * 2, imgHudInven.GetHeight() * 2);
    if (!imgCursor.IsNull()) imgCursor.Draw(hMemDC, mouseX - imgCursor.GetWidth(), mouseY - imgCursor.GetHeight(), imgCursor.GetWidth() * 2, imgCursor.GetHeight() * 2);
    camX = oCX; camY = oCY; SetStretchBltMode(hDC, HALFTONE); SetBrushOrgEx(hDC, 0, 0, NULL);
    StretchBlt(hDC, 0, 0, WIN_WIDTH, WIN_HEIGHT, hMemDC, 0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, SRCCOPY);
    SelectObject(hMemDC, hOldBmp); DeleteObject(hMemBmp); DeleteDC(hMemDC);
}
