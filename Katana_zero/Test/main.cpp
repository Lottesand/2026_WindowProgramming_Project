#include <windows.h>
#include <atlimage.h> 
#include <math.h>
#include <algorithm>
#include <vector>
#include "Enemy.h"
#include "Player.h"
#include "Physics.h"

using namespace std;

// --- [전역 변수 및 상태 정의] ---
HINSTANCE g_hInst;
HWND g_hWnd;
LPCTSTR lpszClass = L"My Window Class";
LPCTSTR lpszWindowName = L"Window Programming Lab";

// 생성된 몬스터들을 관리하는 전역 벡터 구조
std::vector<Enemy*> g_Enemies;

// 플레이어 객체 생성
Player g_Player;

// --- [몬스터 배치 설정] ---
struct EnemySpawnInfo {
    EnemyType type;
    float x;
    float y;
};

std::vector<EnemySpawnInfo> g_EnemySpawns = {
    { EnemyType::GANGSTER, 200.0f, 300.0f },
    { EnemyType::GRUNT, 400.0f, 300.0f },
    { EnemyType::POMP, 600.0f, 300.0f },
    { EnemyType::SHIELDCOP, 800.0f, 300.0f }
};

// ==============================================================================
// 🌟 [해상도 및 환경 설정] 🌟
// ==============================================================================
const int VIRTUAL_WIDTH = 1280;
const int VIRTUAL_HEIGHT = 720;

int WIN_WIDTH = 1280;
int WIN_HEIGHT = 720;

// ==============================================================================
// 🛠️ [튜닝 변수 모음] 
// ==============================================================================
float mapScale = 1.0f;
float playerScale = 2.0f;
float camY_Fixed = 60.0f;

float colW = 20.0f;
float colH = 45.0f;

float g_renderMapScale = 1.0f;
float g_renderPlayerScale = 2.0f;
float g_mapOffsetX = 0.0f;
float g_mapOffsetY = 0.0f;

bool g_showDebugRect = false;
bool g_showGrid = false;
bool g_prevEState = false;

bool g_isFullMapView = false;
bool g_prevFState = false;

int mouseX = VIRTUAL_WIDTH / 2;
int mouseY = VIRTUAL_HEIGHT / 2;
float camX = 0.0f;
float camY = camY_Fixed;

CImage imgMap, imgColMap;

CImage imgHudBase, imgHudBattery, imgHudTimer, imgHudInven;
CImage imgCursor;

// --- [함수 선언] ---
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void LoadAssets();
void UpdateScreenScale();
void UpdateCamera();
void Render(HDC hDC);

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
                
                // 입력 처리 및 물리 업데이트
                bool currentFState = (GetAsyncKeyState('F') & 0x8000) != 0;
                if (currentFState && !g_prevFState) g_isFullMapView = !g_isFullMapView;
                g_prevFState = currentFState;

                bool currentEState = (GetAsyncKeyState('E') & 0x8000) != 0;
                if (currentEState && !g_prevEState) {
                    g_showDebugRect = !g_showDebugRect;
                    g_showGrid = !g_showGrid;
                }
                g_prevEState = currentEState;

                g_Player.Update(mouseX, mouseY, camX, camY, g_renderMapScale, g_mapOffsetX, g_mapOffsetY, g_isFullMapView);
                
                // 몬스터 프레임워크 로직 업데이트 처리
                for (auto& enemy : g_Enemies) {
                    enemy->Update();
                }

                g_Player.UpdateAnimation();
                UpdateCamera();

                InvalidateRect(g_hWnd, NULL, FALSE);
                prevTime = currentTime;
            }
        }
    }
    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam) {
    HDC hDC;
    PAINTSTRUCT ps;

    switch (iMsg) {
    case WM_CREATE:
        ShowCursor(FALSE);
        LoadAssets();
        g_Player.Init();

        // g_EnemySpawns에 설정된 정보를 바탕으로 몬스터 생성
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
        hDC = BeginPaint(hWnd, &ps);
        Render(hDC);
        EndPaint(hWnd, &ps);
        break;
    }
    case WM_DESTROY:
        // 메모리 누수 방지를 위한 동적 할당 몬스터 객체 일괄 제거
        for (auto& enemy : g_Enemies) {
            delete enemy;
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
    float maxLookAhead = 350.0f;
    float targetCamX = (g_Player.GetX() + colW / 2.0f) - (VIRTUAL_WIDTH / g_renderMapScale / 2.0f) + (mouseOffsetX * maxLookAhead);

    camX += (targetCamX - camX) * 0.08f;
    float targetCamY = camY_Fixed;
    camY += (targetCamY - camY) * 0.08f;

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
    float g_scaleX = (float)WIN_WIDTH / VIRTUAL_WIDTH;
    float g_scaleY = (float)WIN_HEIGHT / VIRTUAL_HEIGHT;

    HBITMAP hMemBmp = CreateCompatibleBitmap(hDC, VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
    HBITMAP hOldBmp = (HBITMAP)SelectObject(hMemDC, hMemBmp);

    SetGraphicsMode(hMemDC, GM_ADVANCED);
    PatBlt(hMemDC, 0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, BLACKNESS);

    int mapW = imgMap.IsNull() ? VIRTUAL_WIDTH : imgMap.GetWidth();
    int mapH = imgMap.IsNull() ? VIRTUAL_HEIGHT : imgMap.GetHeight();

    if (!imgMap.IsNull()) {
        CImage* targetMap = g_showDebugRect ? &imgColMap : &imgMap;
        if (g_isFullMapView) {
            targetMap->Draw(hMemDC, (int)g_mapOffsetX, (int)g_mapOffsetY, (int)(mapW * g_renderMapScale), (int)(mapH * g_renderMapScale), 0, 0, mapW, mapH);
        }
        else {
            targetMap->Draw(hMemDC, 0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, (int)camX, (int)camY, (int)(VIRTUAL_WIDTH / g_renderMapScale), (int)(VIRTUAL_HEIGHT / g_renderMapScale));
        }
    }

    // 20px 그리드 표시 (E키 토글)
    if (g_showGrid) {
        HPEN hGridPen = CreatePen(PS_SOLID, 1, RGB(100, 100, 100));
        HPEN hOldPen = (HPEN)SelectObject(hMemDC, hGridPen);

        for (int x = 0; x <= VIRTUAL_WIDTH; x += 20) {
            MoveToEx(hMemDC, x, 0, NULL);
            LineTo(hMemDC, x, VIRTUAL_HEIGHT);
        }
        for (int y = 0; y <= VIRTUAL_HEIGHT; y += 20) {
            MoveToEx(hMemDC, 0, y, NULL);
            LineTo(hMemDC, VIRTUAL_WIDTH, y);
        }

        SelectObject(hMemDC, hOldPen);
        DeleteObject(hGridPen);
    }

    // 몬스터 렌더링
    for (auto& enemy : g_Enemies) {
        enemy->Render(hMemDC);
    }

    // 플레이어 렌더링
    g_Player.Render(hMemDC, camX, camY, mapScale, playerScale, g_renderMapScale, g_mapOffsetX, g_mapOffsetY, g_isFullMapView, g_showDebugRect);

    if (!imgHudBase.IsNull()) {
        int hW = imgHudBase.GetWidth();
        int hH = imgHudBase.GetHeight();
        imgHudBase.Draw(hMemDC, 0, 0, hW * 2, hH * 2);
    }
    if (!imgHudBattery.IsNull()) {
        int bW = imgHudBattery.GetWidth();
        int bH = imgHudBattery.GetHeight();
        imgHudBattery.Draw(hMemDC, 10, 5, bW * 2, bH * 2);
    }
    if (!imgHudTimer.IsNull()) {
        int tW = imgHudTimer.GetWidth();
        int tH = imgHudTimer.GetHeight();
        imgHudTimer.Draw(hMemDC, (VIRTUAL_WIDTH / 2 - tW - 10), 0, tW * 2, tH * 2);
    }
    if (!imgHudInven.IsNull()) {
        int iW = imgHudInven.GetWidth();
        int iH = imgHudInven.GetHeight();
        imgHudInven.Draw(hMemDC, VIRTUAL_WIDTH - iW - 80, 0, iW * 2, iH * 2);
    }

    if (!imgCursor.IsNull()) {
        int cW = imgCursor.GetWidth();
        int cH = imgCursor.GetHeight();
        imgCursor.Draw(hMemDC, mouseX - (cW / 2), mouseY - (cH / 2), cW * 2, cH * 2);
    }

    SetStretchBltMode(hDC, HALFTONE);
    SetBrushOrgEx(hDC, 0, 0, NULL);
    StretchBlt(hDC, 0, 0, WIN_WIDTH, WIN_HEIGHT, hMemDC, 0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, SRCCOPY);

    SelectObject(hMemDC, hOldBmp);
    DeleteObject(hMemBmp);
    DeleteDC(hMemDC);
}