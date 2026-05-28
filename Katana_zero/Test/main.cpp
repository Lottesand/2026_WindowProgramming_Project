#include <windows.h>
#include <atlimage.h> 
#include <math.h>
#include <algorithm>
#include <vector>
#include <time.h>
#include "Enemy.h"
#include "Player.h"
#include "Physics.h"

using namespace std;

HINSTANCE g_hInst;
HWND g_hWnd;
LPCTSTR lpszClass = L"My Window Class";
LPCTSTR lpszWindowName = L"Window Programming Lab";

std::vector<Enemy*> g_Enemies;
Player g_Player;

struct EnemySpawnInfo {
    EnemyType type;
    float x;
    float y;
};

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

bool g_isTimePaused = false;
std::vector<NeonTrail> g_NeonTrails;
std::vector<EnemySpawnInfo> g_EnemySpawns = {
    { EnemyType::POMP, 800.0f, 300.0f }
};

const int VIRTUAL_WIDTH = 1280;
const int VIRTUAL_HEIGHT = 720;
int WIN_WIDTH = 1280;
int WIN_HEIGHT = 720;

float mapScale = 1.0f;
float playerScale = 2.0f;
float camY_Fixed = 60.0f;

float g_camLookAheadX = 100.0f;
float g_camLerpSpeedX = 0.08f;
float g_camLerpSpeedY = 0.08f;

float g_renderMapScale = 1.0f;
float g_renderPlayerScale = 2.0f;
float g_mapOffsetX = 0.0f;
float g_mapOffsetY = 0.0f;

float g_slashWidth = 60.0f;
float g_shakeIntensity = 30.0f;
float g_hitShakeForce = 15.0f;
float g_shakeDecay = 0.85f;

float g_curShakeX = 0.0f;
float g_curShakeY = 0.0f;
float g_camPushX = 0.0f;
float g_camPushY = 0.0f;
float g_shakeTrauma = 0.0f;

// --- [VFX 및 시간 설정] ---
int g_neonTrailLife = 6;           // 네온 궤적 이펙트 지속 시간 (프레임 단위)

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

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void LoadAssets();
void UpdateScreenScale();
void UpdateCamera();
void Render(HDC hDC);
void SpawnEnemies();

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdParam, int nCmdShow) {
    HWND hWnd;
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
                
                bool currentFState = (GetAsyncKeyState('F') & 0x8000) != 0;
                if (currentFState && !g_prevFState) g_isFullMapView = !g_isFullMapView;
                g_prevFState = currentFState;

                bool currentEState = (GetAsyncKeyState('E') & 0x8000) != 0;
                if (currentEState && !g_prevEState) {
                    g_showDebugRect = !g_showDebugRect;
                    g_showGrid = !g_showGrid;
                }
                g_prevEState = currentEState;

                static bool prevRState = false;
                bool currentRState = (GetAsyncKeyState('R') & 0x8000) != 0;
                if (currentRState && !prevRState) {
                    SpawnEnemies();
                }
                prevRState = currentRState;

                if (!g_isTimePaused) {
                    g_Player.Update(mouseX, mouseY, camX, camY, g_renderMapScale, g_mapOffsetX, g_mapOffsetY, g_isFullMapView);
                }
                
                static int lastAttackFrame = -1;
                if (!g_isTimePaused && g_Player.GetState() == PlayerState::ATTACK) {
                    int currentFrame = g_Player.GetCurrentFrame();
                    if (currentFrame >= 1 && currentFrame <= 3 && currentFrame != lastAttackFrame) {
                        float hitW = g_Player.GetAttackHitW();
                        float hitH = g_Player.GetAttackHitH();
                        float hitOffset = g_Player.GetAttackHitOffset();
                        float adX = g_Player.GetAttackDirX();
                        float adY = g_Player.GetAttackDirY();

                        float centerX = g_Player.GetX() + g_Player.GetColW() / 2.0f;
                        float centerY = g_Player.GetY() + g_Player.GetColH() / 2.0f;
                        float hitX = centerX + adX * hitOffset - hitW / 2.0f;
                        float hitY = centerY + adY * hitOffset - hitH / 2.0f;

                        RECT aRect = { (int)hitX, (int)hitY, (int)(hitX + hitW), (int)(hitY + hitH) };

                        for (auto& enemyPtr : g_Enemies) {
                            if (enemyPtr && enemyPtr->GetIsAlive()) {
                                RECT eRect = enemyPtr->GetRect();
                                RECT overlap;
                                if (IntersectRect(&overlap, &aRect, &eRect)) {
                                    g_isTimePaused = true;
                                    float enemyX = enemyPtr->GetX() + enemyPtr->GetColW() / 2.0f;
                                    float enemyY = enemyPtr->GetY() + enemyPtr->GetColH() / 2.0f;
                                    float dx = enemyX - centerX;
                                    float dy = enemyY - centerY;
                                    float dist = sqrt(dx * dx + dy * dy);
                                    if (dist < 1.0f) dist = 1.0f;
                                    float ux = dx / dist;
                                    float uy = dy / dist;

                                    NeonTrail trail;
                                    trail.x = enemyX;
                                    trail.y = enemyY;
                                    trail.startX = enemyX - ux * 1000.0f;
                                    trail.startY = enemyY - uy * 1000.0f;
                                    trail.endX = enemyX + ux * 3000.0f;
                                    trail.endY = enemyY + uy * 3000.0f;
                                    trail.dirX = ux;
                                    trail.dirY = uy;
                                    trail.angle = atan2(uy, ux);
                                    trail.length = 0.0f;
                                    trail.maxLength = 4000.0f;
                                    trail.life = g_neonTrailLife;
                                    trail.maxLife = g_neonTrailLife;
                                    g_NeonTrails.push_back(trail);

                                    g_camPushX = ux * g_hitShakeForce * 2.0f;
                                    g_camPushY = uy * g_hitShakeForce * 2.0f;
                                    g_shakeTrauma = 1.0f; 

                                    enemyPtr->OnTakeDamage(1.0f);
                                    float kbForce = (adX >= 0) ? 45.0f : -45.0f;
                                    enemyPtr->ApplyKnockback(kbForce);
                                }
                            }
                        }
                        lastAttackFrame = currentFrame;
                    }
                }
                else if (!g_isTimePaused) {
                    lastAttackFrame = -1;
                }

                if (!g_isTimePaused) {
                    for (auto& enemyObj : g_Enemies) {
                        if (enemyObj) enemyObj->Update();
                    }
                }

                bool hasActiveVFX = false;
                for (auto itTrail = g_NeonTrails.begin(); itTrail != g_NeonTrails.end(); ) {
                    itTrail->life--;
                    if (itTrail->life <= 0) {
                        itTrail = g_NeonTrails.erase(itTrail);
                    }
                    else {
                        itTrail->length += itTrail->maxLength / (itTrail->maxLife / 2.0f); 
                        hasActiveVFX = true;
                        itTrail++;
                    }
                }
                
                if (g_isTimePaused && !hasActiveVFX) {
                    g_isTimePaused = false;
                }

                if (!g_isTimePaused) {
                    g_Player.UpdateAnimation();
                }
                UpdateCamera();
                InvalidateRect(g_hWnd, NULL, FALSE);
                prevTime = currentTime;
            }
        }
    }
    return (int)msg.wParam;
}

void SpawnEnemies() {
    for (auto& enemyPtr : g_Enemies) {
        if (enemyPtr) delete enemyPtr;
    }
    g_Enemies.clear();
    for (const auto& info : g_EnemySpawns) {
        Enemy* newEnemy = nullptr;
        switch (info.type) {
        case EnemyType::GANGSTER:  newEnemy = new Gangster(info.x, info.y); break;
        case EnemyType::GRUNT:     newEnemy = new Grunt(info.x, info.y); break;
        case EnemyType::POMP:      newEnemy = new Pomp(info.x, info.y); break;
        case EnemyType::SHIELDCOP: newEnemy = new ShieldCop(info.x, info.y); break;
        }
        if (newEnemy) {
            newEnemy->Init();
            g_Enemies.push_back(newEnemy);
        }
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam) {
    switch (iMsg) {
    case WM_CREATE:
        ShowCursor(FALSE);
        LoadAssets();
        g_Player.Init();
        SpawnEnemies();
        break;
    case WM_SIZE:
        WIN_WIDTH = LOWORD(lParam);
        WIN_HEIGHT = HIWORD(lParam);
        break;
    case WM_ERASEBKGND:
        return 1;
    case WM_MOUSEMOVE:
        if (WIN_WIDTH != 0 && WIN_HEIGHT != 0) {
            mouseX = (int)(LOWORD(lParam) * ((float)VIRTUAL_WIDTH / WIN_WIDTH));
            mouseY = (int)(HIWORD(lParam) * ((float)VIRTUAL_HEIGHT / WIN_HEIGHT));
        }
        break;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hDC = BeginPaint(hWnd, &ps);
        Render(hDC);
        EndPaint(hWnd, &ps);
        break;
    }
    case WM_DESTROY:
        for (auto& enemyPtr : g_Enemies) {
            if (enemyPtr) delete enemyPtr;
        }
        g_Enemies.clear();
        PostQuitMessage(0);
        break;
    }
    return DefWindowProc(hWnd, iMsg, wParam, lParam);
}

void LoadAssets() {
    imgMap.Load(TEXT("assets/map.png"));
    imgColMap.Load(TEXT("assets/colmap.png"));
    imgCursor.Load(TEXT("assets/cursor.png"));
    imgHudBase.Load(TEXT("assets/hud/base.png"));
    imgHudBattery.Load(TEXT("assets/hud/battery.png"));
    imgHudTimer.Load(TEXT("assets/hud/timer.png"));
    imgHudInven.Load(TEXT("assets/hud/inven.png"));
}

void UpdateScreenScale() {
    g_renderMapScale = mapScale;
    g_renderPlayerScale = playerScale;
    g_mapOffsetX = 0.0f;
    g_mapOffsetY = 0.0f;
    int mapW = imgMap.IsNull() ? VIRTUAL_WIDTH : imgMap.GetWidth();
    int mapH = imgMap.IsNull() ? VIRTUAL_HEIGHT : imgMap.GetHeight();
    if (g_isFullMapView && !imgMap.IsNull()) {
        float scaleX = (float)VIRTUAL_WIDTH / mapW;
        float scaleY = (float)VIRTUAL_HEIGHT / mapH;
        g_renderMapScale = (scaleX < scaleY) ? scaleX : scaleY;
        g_renderPlayerScale = playerScale * (g_renderMapScale / mapScale);
        g_mapOffsetX = (VIRTUAL_WIDTH - (mapW * g_renderMapScale)) / 2.0f;
        g_mapOffsetY = (VIRTUAL_HEIGHT - (mapH * g_renderMapScale)) / 2.0f;
    }
}

void UpdateCamera() {
    if (g_isFullMapView) return;
    float mouseOffsetX = (float)(mouseX - (VIRTUAL_WIDTH / 2)) / (VIRTUAL_WIDTH / 2);
    float playerCenterX = g_Player.GetX() + g_Player.GetColW() / 2.0f;
    float viewHalfWidth = (VIRTUAL_WIDTH / g_renderMapScale) / 2.0f;
    float targetCamX = playerCenterX - viewHalfWidth + (mouseOffsetX * g_camLookAheadX);
    camX += (targetCamX - camX) * g_camLerpSpeedX;
    camY += (camY_Fixed - camY) * g_camLerpSpeedY;
    g_camPushX *= g_shakeDecay;
    g_camPushY *= g_shakeDecay;
    float traumaSquare = g_shakeTrauma * g_shakeTrauma;
    if (traumaSquare > 0.001f) {
        g_curShakeX = ((float)(rand() % 100) / 50.0f - 1.0f) * g_shakeIntensity * traumaSquare;
        g_curShakeY = ((float)(rand() % 100) / 50.0f - 1.0f) * g_shakeIntensity * traumaSquare;
    } else {
        g_curShakeX = 0; g_curShakeY = 0;
    }
    g_shakeTrauma *= g_shakeDecay;
    if (g_shakeTrauma < 0.01f) g_shakeTrauma = 0;
    if (camX < 0) camX = 0;
    if (camY < 0) camY = 0;
    if (!imgMap.IsNull()) {
        float viewWidthInMap = VIRTUAL_WIDTH / g_renderMapScale;
        float viewHeightInMap = VIRTUAL_HEIGHT / g_renderMapScale;
        float maxCamX = (float)imgMap.GetWidth() - viewWidthInMap;
        if (camX > maxCamX) camX = maxCamX;
        float maxCamY = (float)imgMap.GetHeight() - viewHeightInMap;
        if (camY > maxCamY) camY = maxCamY;
        if (camX < 0) camX = 0;
        if (camY < 0) camY = 0;
    }
}

void Render(HDC hDC) {
    HDC hMemDC = CreateCompatibleDC(hDC);
    HBITMAP hMemBmp = CreateCompatibleBitmap(hDC, VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
    HBITMAP hOldBmp = (HBITMAP)SelectObject(hMemDC, hMemBmp);
    SetGraphicsMode(hMemDC, GM_ADVANCED);
    PatBlt(hMemDC, 0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, BLACKNESS);
    float originalCamX = camX;
    float originalCamY = camY;
    camX += (g_curShakeX + g_camPushX);
    camY += (g_curShakeY + g_camPushY);
    int curMapW = imgMap.IsNull() ? VIRTUAL_WIDTH : imgMap.GetWidth();
    int curMapH = imgMap.IsNull() ? VIRTUAL_HEIGHT : imgMap.GetHeight();
    if (!imgMap.IsNull()) {
        CImage* targetMap = g_showDebugRect ? &imgColMap : &imgMap;
        if (g_isFullMapView) {
            targetMap->Draw(hMemDC, (int)g_mapOffsetX, (int)g_mapOffsetY, (int)(curMapW * g_renderMapScale), (int)(curMapH * g_renderMapScale), 0, 0, curMapW, curMapH);
        } else {
            targetMap->Draw(hMemDC, 0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, (int)camX, (int)camY, (int)(VIRTUAL_WIDTH / g_renderMapScale), (int)(VIRTUAL_HEIGHT / g_renderMapScale));
        }
    }
    if (g_showGrid) {
        HPEN hGridPen = CreatePen(PS_SOLID, 1, RGB(100, 100, 100));
        HPEN hOldPen = (HPEN)SelectObject(hMemDC, hGridPen);
        for (int x = 0; x <= VIRTUAL_WIDTH; x += 20) {
            MoveToEx(hMemDC, x, 0, NULL); LineTo(hMemDC, x, VIRTUAL_HEIGHT);
        }
        for (int y = 0; y <= VIRTUAL_HEIGHT; y += 20) {
            MoveToEx(hMemDC, 0, y, NULL); LineTo(hMemDC, VIRTUAL_WIDTH, y);
        }
        SelectObject(hMemDC, hOldPen); DeleteObject(hGridPen);
    }
    for (auto& enemyObj : g_Enemies) { if (enemyObj) enemyObj->Render(hMemDC); }
    float fW = (float)VIRTUAL_WIDTH, fH = (float)VIRTUAL_HEIGHT;
    float fCW = (float)curMapW, fCH = (float)curMapH;
    float scaleX = fW / fCW, scaleY = fH / fCH;
    float currentFitScale = (scaleX < scaleY) ? scaleX : scaleY;
    float currentFitX = (fW - fCW * currentFitScale) / 2.0f;
    float currentFitY = (fH - fCH * currentFitScale) / 2.0f;
    for (const auto& trail : g_NeonTrails) {
        float pFitScale = g_isFullMapView ? g_renderMapScale : mapScale;
        bool isPink = (GetTickCount() / 50) % 2 == 0;
        COLORREF drawColor = isPink ? RGB(255, 0, 255) : RGB(0, 255, 255);
        int segments = 20; 
        float segmentLen = trail.length / segments;
        for (int i = 0; i < segments; ++i) {
            float startDist = i * segmentLen, endDist = (i + 1) * segmentLen;
            float edgeAlpha = 1.0f, progress = (float)i / segments;
            if (progress < 0.15f) edgeAlpha = progress / 0.15f;
            else if (progress > 0.85f) edgeAlpha = (1.0f - progress) / 0.15f;
            float sX = trail.startX + trail.dirX * startDist, sY = trail.startY + trail.dirY * startDist;       
            float eX = trail.startX + trail.dirX * endDist, eY = trail.startY + trail.dirY * endDist;
            float vSX, vSY, vEX, vEY;
            if (g_isFullMapView) {
                vSX = sX * currentFitScale + currentFitX; vSY = sY * currentFitScale + currentFitY;
                vEX = eX * currentFitScale + currentFitX; vEY = eY * currentFitScale + currentFitY;
            } else {
                vSX = (sX - camX) * mapScale; vSY = (sY - camY) * mapScale;
                vEX = (eX - camX) * mapScale; vEY = (eY - camY) * mapScale;
            }
            float baseWidth = g_slashWidth * pFitScale * (trail.life / (float)trail.maxLife) * 0.25f;
            if (baseWidth < 1.0f) baseWidth = 1.0f;
            COLORREF finalColor = RGB((int)(GetRValue(drawColor) * edgeAlpha), (int)(GetGValue(drawColor) * edgeAlpha), (int)(GetBValue(drawColor) * edgeAlpha));
            HPEN hPen = CreatePen(PS_SOLID, (int)baseWidth, finalColor);
            HPEN hOldPen = (HPEN)SelectObject(hMemDC, hPen);
            MoveToEx(hMemDC, (int)vSX, (int)vSY, NULL); LineTo(hMemDC, (int)vEX, (int)vEY);
            SelectObject(hMemDC, hOldPen); DeleteObject(hPen);
        }
    }
    g_Player.Render(hMemDC, camX, camY, mapScale, playerScale, g_renderMapScale, g_mapOffsetX, g_mapOffsetY, g_isFullMapView, g_showDebugRect);
    if (!imgHudBase.IsNull()) imgHudBase.Draw(hMemDC, 0, 0, imgHudBase.GetWidth() * 2, imgHudBase.GetHeight() * 2);
    if (!imgHudBattery.IsNull()) imgHudBattery.Draw(hMemDC, 10, 5, imgHudBattery.GetWidth() * 2, imgHudBattery.GetHeight() * 2);
    if (!imgHudTimer.IsNull()) imgHudTimer.Draw(hMemDC, (VIRTUAL_WIDTH / 2 - imgHudTimer.GetWidth() - 10), 0, imgHudTimer.GetWidth() * 2, imgHudTimer.GetHeight() * 2);
    if (!imgHudInven.IsNull()) imgHudInven.Draw(hMemDC, VIRTUAL_WIDTH - imgHudInven.GetWidth() - 80, 0, imgHudInven.GetWidth() * 2, imgHudInven.GetHeight() * 2);
    if (!imgCursor.IsNull()) imgCursor.Draw(hMemDC, mouseX - imgCursor.GetWidth(), mouseY - imgCursor.GetHeight(), imgCursor.GetWidth() * 2, imgCursor.GetHeight() * 2);
    camX = originalCamX; camY = originalCamY;
    SetStretchBltMode(hDC, HALFTONE); SetBrushOrgEx(hDC, 0, 0, NULL);
    StretchBlt(hDC, 0, 0, WIN_WIDTH, WIN_HEIGHT, hMemDC, 0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, SRCCOPY);
    SelectObject(hMemDC, hOldBmp); DeleteObject(hMemBmp); DeleteDC(hMemDC);
}
