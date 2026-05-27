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

bool g_isTimePaused = false;
std::vector<NeonTrail> g_NeonTrails;
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

// [타격 연출 튜닝 변수]
float g_slashWidth = 100.0f;       // 슬래시 가로폭
float g_shakeIntensity = 30.0f;  // 화면 흔들림 기본 강도
float g_hitShakeForce = 15.0f;    // 타격 시 마우스 방향으로 툭 치는 힘 (세기)
float g_shakeDecay = 0.85f;      // 흔들림 감쇄율

float g_curShakeX = 0.0f;
float g_curShakeY = 0.0f;


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

                if (!g_isTimePaused) {
                    g_Player.Update(mouseX, mouseY, camX, camY, g_renderMapScale, g_mapOffsetX, g_mapOffsetY, g_isFullMapView);
                }
                
                // --- [히트 판정 및 이펙트 생성] ---
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
                                    // 타격 성공! 시간 정지
                                    g_isTimePaused = true;
                                    
                                    float enemyX = enemyPtr->GetX() + enemyPtr->GetColW() / 2.0f;
                                    float enemyY = enemyPtr->GetY() + enemyPtr->GetColH() / 2.0f;

                                    // 방향 계산 (플레이어 -> 적)
                                    float dx = enemyX - centerX;
                                    float dy = enemyY - centerY;
                                    float dist = sqrt(dx * dx + dy * dy);
                                    if (dist < 1.0f) dist = 1.0f;
                                    float ux = dx / dist;
                                    float uy = dy / dist;

                                    // 네온 궤적 생성
                                    NeonTrail trail;
                                    trail.x = enemyX;
                                    trail.y = enemyY;
                                    trail.startX = enemyX - ux * 2000.0f;
                                    trail.startY = enemyY - uy * 2000.0f;
                                    trail.endX = enemyX + ux * 2000.0f;
                                    trail.endY = enemyY + uy * 2000.0f;
                                    trail.dirX = ux;
                                    trail.dirY = uy;
                                    trail.angle = atan2(uy, ux);
                                    trail.length = 0.0f;
                                    trail.maxLength = 4000.0f;
                                    trail.life = 8;
                                    trail.maxLife = 8;
                                    g_NeonTrails.push_back(trail);

                                    // 방향성 화면 흔들림 설정 (공격 방향으로 툭!)
                                    // g_hitShakeForce 변수를 사용하여 찔끔 갔다 오는 효과의 세기 조절
                                    g_curShakeX = ux * g_hitShakeForce;
                                    g_curShakeY = uy * g_hitShakeForce;

                                    // 적의 데미지 처리
                                    enemyPtr->OnTakeDamage(1.0f);
                                    float kbForce = (adX >= 0) ? 25.0f : -25.0f;
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
                    // 몬스터 프레임워크 로직 업데이트 처리
                    for (auto& enemyObj : g_Enemies) {
                        if (enemyObj) {
                            enemyObj->Update();
                        }
                    }
                }

                // 네온 궤적 업데이트
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
                
                // 모든 VFX가 끝나면 시간 정지 해제
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
        for (auto& enemyPtr : g_Enemies) {
            if (enemyPtr) {
                delete enemyPtr;
            }
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
    float targetCamX = (g_Player.GetX() + g_Player.GetColW() / 2.0f) - (VIRTUAL_WIDTH / g_renderMapScale / 2.0f) + (mouseOffsetX * maxLookAhead);

    camX += (targetCamX - camX) * 0.08f;
    float targetCamY = camY_Fixed;
    camY += (targetCamY - camY) * 0.08f;

    // 흔들림 감쇄 (오프셋으로만 관리)
    g_curShakeX *= g_shakeDecay;
    g_curShakeY *= g_shakeDecay;
    if (fabs(g_curShakeX) < 0.05f) g_curShakeX = 0;
    if (fabs(g_curShakeY) < 0.05f) g_curShakeY = 0;

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

    // --- [화면 흔들림 임시 적용] ---
    // 렌더링 중에만 camX/Y를 오프셋만큼 이동시켜 모든 객체가 함께 흔들리게 함
    float originalCamX = camX;
    float originalCamY = camY;
    camX += g_curShakeX;
    camY += g_curShakeY;

    int curMapW = imgMap.IsNull() ? VIRTUAL_WIDTH : imgMap.GetWidth();
    int curMapH = imgMap.IsNull() ? VIRTUAL_HEIGHT : imgMap.GetHeight();

    if (!imgMap.IsNull()) {
        CImage* targetMap = g_showDebugRect ? &imgColMap : &imgMap;
        if (g_isFullMapView) {
            targetMap->Draw(hMemDC, (int)g_mapOffsetX, (int)g_mapOffsetY, (int)(curMapW * g_renderMapScale), (int)(curMapH * g_renderMapScale), 0, 0, curMapW, curMapH);
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
    for (auto& enemyObj : g_Enemies) {
        if (enemyObj) {
            enemyObj->Render(hMemDC);
        }
    }

    // 네온 궤적 렌더링
    float currentFitScale = (std::min)((float)VIRTUAL_WIDTH / (float)curMapW, (float)VIRTUAL_HEIGHT / (float)curMapH);
    float currentFitX = (VIRTUAL_WIDTH - curMapW * currentFitScale) / 2.0f;
    float currentFitY = (VIRTUAL_HEIGHT - curMapH * currentFitScale) / 2.0f;

    for (const auto& trail : g_NeonTrails) {
        float pFitScale = g_isFullMapView ? g_renderMapScale : mapScale;
        
        // --- [색상 사이클링: 프레임 단위로 민트 <-> 핑크 고속 교체] ---
        // 빛 효과 없이 솔리드한 색상만 사용 (약 2~3프레임마다 교체되도록 50ms 주기)
        bool isPink = (GetTickCount() / 50) % 2 == 0; 
        COLORREF drawColor = isPink ? RGB(255, 0, 255) : RGB(0, 255, 255);

        int segments = 20; 
        float segmentLen = trail.length / segments;

        for (int i = 0; i < segments; ++i) {
            float startDist = i * segmentLen;
            float endDist = (i + 1) * segmentLen;
            
            // 양 끝단만 아주 살짝 투명도 처리 (뚝 끊겨 보이지 않게만)
            float edgeAlpha = 1.0f;
            float progress = (float)i / segments;
            if (progress < 0.15f) edgeAlpha = progress / 0.15f;
            else if (progress > 0.85f) edgeAlpha = (1.0f - progress) / 0.15f;

            float sX = trail.startX + trail.dirX * startDist;
            float sY = trail.startY + trail.dirY * startDist;
            float eX = trail.startX + trail.dirX * endDist;
            float eY = trail.startY + trail.dirY * endDist;

            float vSX, vSY, vEX, vEY;
            if (g_isFullMapView) {
                vSX = sX * currentFitScale + currentFitX; vSY = sY * currentFitScale + currentFitY;
                vEX = eX * currentFitScale + currentFitX; vEY = eY * currentFitScale + currentFitY;
            }
            else {
                vSX = (sX - camX) * mapScale; vSY = (sY - camY) * mapScale;
                vEX = (eX - camX) * mapScale; vEY = (eY - camY) * mapScale;
            }

            // 기본 두께 (매우 얇고 날카롭게, 빛 효과 제거)
            float baseWidth = g_slashWidth * pFitScale * (trail.life / (float)trail.maxLife) * 0.25f; 
            if (baseWidth < 1.0f) baseWidth = 1.0f;

            // --- [빛 없는 버전: 단일 솔리드 라인] ---
            // 레이어 겹침 없이 하나의 선명한 선으로만 표현
            COLORREF finalColor = RGB(
                (int)(GetRValue(drawColor) * edgeAlpha),
                (int)(GetGValue(drawColor) * edgeAlpha),
                (int)(GetBValue(drawColor) * edgeAlpha)
            );

            HPEN hPen = CreatePen(PS_SOLID, (int)baseWidth, finalColor);
            HPEN hOldPen = (HPEN)SelectObject(hMemDC, hPen);
            MoveToEx(hMemDC, (int)vSX, (int)vSY, NULL);
            LineTo(hMemDC, (int)vEX, (int)vEY);
            SelectObject(hMemDC, hOldPen);
            DeleteObject(hPen);
        }
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
        imgCursor.Draw(hMemDC, mouseX - cW, mouseY - cH, cW * 2, cH * 2);
    }

    // --- [카메라 원상 복구] ---
    camX = originalCamX;
    camY = originalCamY;

    SetStretchBltMode(hDC, HALFTONE);
    SetBrushOrgEx(hDC, 0, 0, NULL);
    StretchBlt(hDC, 0, 0, WIN_WIDTH, WIN_HEIGHT, hMemDC, 0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, SRCCOPY);

    SelectObject(hMemDC, hOldBmp);
    DeleteObject(hMemBmp);
    DeleteDC(hMemDC);
}
