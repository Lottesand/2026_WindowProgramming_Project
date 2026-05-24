#include <windows.h>
#include <atlimage.h> 
#include <math.h>
#include <algorithm>

using namespace std;

// --- [전역 변수 및 상태 정의] ---
HINSTANCE g_hInst;
HWND g_hWnd;
LPCTSTR lpszClass = L"My Window Class";
LPCTSTR lpszWindowName = L"Window Programming Lab";

// 🌟 [추가됨] 걷기 전환 모션 2가지 상태 추가
enum PlayerState {
    IDLE, IDLE_TO_WALK, WALK, WALK_TO_IDLE, RUN,
    JUMP_UP, FALL,
    PREVDOWN, DOWN, POSTDOWN,
    ROLL, ATTACK,
    WALL_GRAB, WALL_SLIDE, WALL_FLIP
};
PlayerState pState = IDLE;

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

float currentVx = 0.0f;
float moveSpeedWalk = 12.0f;
float moveSpeedRun = 10.0f;
float moveSpeedRoll = 15.0f;
float accelRate = 0.6f;
float frictionRate = 0.3f;

const float JUMP_POWER = -10.5f;
const float GRAVITY_NORMAL = 1.0f;
const float GRAVITY_HOLD = 0.45f;
const float MAX_FALL_SPEED = 30.0f;

float dashRadius = 150.0f;
float dashSpeed = 15.0f;
DWORD attackCooldown = 350;

DWORD wallHangTime = 150;
float wallSlideSpeed = 2.5f;
float wallSlideFastSpeed = 12.0f;
float wallJumpPowerY = -11.0f;
float wallJumpPowerX = 14.0f;

// 🌟 [추가됨] 걷기 전환 디테일 튜닝 변수
float speedIdleToWalk = 1.0f;     // IDLE -> WALK 전환 중일 때의 속도 (제자리 느낌)
DWORD aniDelayIdleToWalk = 60;    // IDLE -> WALK 애니메이션 속도 (4프레임)
DWORD aniDelayWalkToIdle = 60;    // WALK -> IDLE 애니메이션 속도 (5프레임)

DWORD aniDelayIdle = 150;
DWORD aniDelayWalk = 100;
DWORD aniDelayRun = 80;
DWORD aniDelayJumpFall = 100;
DWORD aniDelayCrouch = 80;
DWORD aniDelayRoll = 50;
DWORD aniDelayAttack = 40;
DWORD aniDelaySlash = 40;
DWORD aniDelayWallGrab = 80;
DWORD aniDelayWallSlide = 100;
DWORD aniDelayWallFlip = 40;

bool g_prevLButton = false;
DWORD lastAttackTime = 0;
bool canAirYDash = true;

DWORD wallGrabTime = 0;
int wallDir = 0;

float attackTargetX = 0.0f;
float attackTargetY = 0.0f;
float attackDirX = 0.0f;
float attackDirY = 0.0f;
float attackAngle = 0.0f;

float g_renderMapScale = 1.0f;
float g_renderPlayerScale = 2.0f;
float g_mapOffsetX = 0.0f;
float g_mapOffsetY = 0.0f;

bool canRoll = true;
bool canJump = true;

bool g_showDebugRect = false;
bool g_prevEState = false;

bool g_isFullMapView = false;
bool g_prevFState = false;

int mouseX = VIRTUAL_WIDTH / 2;
int mouseY = VIRTUAL_HEIGHT / 2;
float camX = 0.0f;
float camY = camY_Fixed;

float pX = 100.0f, pY = 300.0f;
float pVy = 0.0f;
bool isJumping = false;
bool isFacingRight = true;

int currentFrame = 0;

CImage imgMap, imgColMap, imgIdle[11], imgWalk[10], imgRun[10], imgJumpUp[4], imgFall[4];
CImage imgPrevDown[2], imgDown[1], imgPostDown[2], imgRoll[6], imgCursor, imgAttack[7], imgSlashFX[5];
CImage imgWallGrab[2], imgWallSlide[1], imgWallFlip[11];
// 🌟 [추가됨] 걷기 전환 애니메이션 에셋
CImage imgIdleToWalk[4];
CImage imgWalkToIdle[5];

CImage imgHudBase, imgHudBattery, imgHudTimer, imgHudInven;

// --- [함수 선언] ---
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void LoadAssets();
void UpdateScreenScale();
void UpdatePhysicsAndInput();
void UpdateAnimation();
int GetCollisionType(int x, int y);
bool CheckMapCollision(float x, float y, float w, float h);
bool CheckSpecificCollision(float x, float y, float w, float h, int targetType);
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
                UpdatePhysicsAndInput();
                UpdateAnimation();
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
        PostQuitMessage(0);
        break;
    }
    return DefWindowProc(hWnd, iMsg, wParam, lParam);
}

void LoadAssets() {
    imgMap.Load(TEXT("assets/map.png"));
    imgColMap.Load(TEXT("assets/colmap.png"));

    TCHAR path[256];
    for (int i = 0; i < 11; i++) { wsprintf(path, TEXT("assets/idle/%d.png"), i); imgIdle[i].Load(path); }
    for (int i = 0; i < 10; i++) { wsprintf(path, TEXT("assets/walk/%d.png"), i); imgWalk[i].Load(path); }
    for (int i = 0; i < 10; i++) { wsprintf(path, TEXT("assets/run/%d.png"), i); imgRun[i].Load(path); }
    for (int i = 0; i < 4; i++) { wsprintf(path, TEXT("assets/jump/%d.png"), i); imgJumpUp[i].Load(path); }
    for (int i = 0; i < 4; i++) { wsprintf(path, TEXT("assets/fall/%d.png"), i); imgFall[i].Load(path); }
    for (int i = 0; i < 2; i++) { wsprintf(path, TEXT("assets/prevdown/%d.png"), i); imgPrevDown[i].Load(path); }
    for (int i = 0; i < 1; i++) { wsprintf(path, TEXT("assets/down/%d.png"), i); imgDown[i].Load(path); }
    for (int i = 0; i < 2; i++) { wsprintf(path, TEXT("assets/postdown/%d.png"), i); imgPostDown[i].Load(path); }
    for (int i = 0; i < 6; i++) { wsprintf(path, TEXT("assets/roll/%d.png"), i); imgRoll[i].Load(path); }
    for (int i = 0; i < 2; i++) { wsprintf(path, TEXT("assets/wallgrab/%d.png"), i); imgWallGrab[i].Load(path); }
    for (int i = 0; i < 1; i++) { wsprintf(path, TEXT("assets/wallslide/%d.png"), i); imgWallSlide[i].Load(path); }
    for (int i = 0; i < 11; i++) { wsprintf(path, TEXT("assets/wallflip/%d.png"), i); imgWallFlip[i].Load(path); }

    // 🌟 걷기 전환 모션 에셋 (경로 주의!)
    for (int i = 0; i < 4; i++) { wsprintf(path, TEXT("assets/idletowalk/%d.png"), i); imgIdleToWalk[i].Load(path); }
    for (int i = 0; i < 5; i++) { wsprintf(path, TEXT("assets/walktoidle/%d.png"), i); imgWalkToIdle[i].Load(path); }

    imgCursor.Load(TEXT("assets/cursor.png"));
    for (int i = 0; i < 7; i++) { wsprintf(path, TEXT("assets/attack/%d.png"), i); imgAttack[i].Load(path); }
    for (int i = 0; i < 5; i++) { wsprintf(path, TEXT("assets/slash/%d.png"), i); imgSlashFX[i].Load(path); }

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

int GetCollisionType(int targetX, int targetY) {
    if (imgColMap.IsNull()) return 1;
    if (targetX < 0 || targetY < 0 || targetX >= imgColMap.GetWidth() || targetY >= imgColMap.GetHeight()) return 1;
    COLORREF pixelColor = imgColMap.GetPixel(targetX, targetY);
    int r = GetRValue(pixelColor); int g = GetGValue(pixelColor); int b = GetBValue(pixelColor);
    if (r == 0 && g == 255 && b == 0) return 1;
    if (r == 255 && g == 0 && b == 0) return 2;
    if (r == 0 && g == 0 && b == 255) return 3;
    return 0;
}

bool CheckMapCollision(float x, float y, float w, float h) {
    auto isSolid = [](int t) { return t == 1 || t == 3; };
    if (isSolid(GetCollisionType((int)x, (int)y))) return true;
    if (isSolid(GetCollisionType((int)(x + w / 2), (int)y))) return true;
    if (isSolid(GetCollisionType((int)(x + w), (int)y))) return true;
    if (isSolid(GetCollisionType((int)x, (int)(y + h / 2)))) return true;
    if (isSolid(GetCollisionType((int)(x + w), (int)(y + h / 2)))) return true;
    if (isSolid(GetCollisionType((int)x, (int)(y + h)))) return true;
    if (isSolid(GetCollisionType((int)(x + w / 2), (int)(y + h)))) return true;
    if (isSolid(GetCollisionType((int)(x + w), (int)(y + h)))) return true;
    return false;
}

bool CheckSpecificCollision(float x, float y, float w, float h, int targetType) {
    if (GetCollisionType((int)x, (int)y) == targetType) return true;
    if (GetCollisionType((int)(x + w / 2), (int)y) == targetType) return true;
    if (GetCollisionType((int)(x + w), (int)y) == targetType) return true;
    if (GetCollisionType((int)x, (int)(y + h / 2)) == targetType) return true;
    if (GetCollisionType((int)(x + w), (int)(y + h / 2)) == targetType) return true;
    if (GetCollisionType((int)x, (int)(y + h)) == targetType) return true;
    if (GetCollisionType((int)(x + w / 2), (int)(y + h)) == targetType) return true;
    if (GetCollisionType((int)(x + w), (int)(y + h)) == targetType) return true;
    return false;
}

void UpdatePhysicsAndInput() {
    DWORD currentTime = GetTickCount();

    bool currentFState = (GetAsyncKeyState('F') & 0x8000) != 0;
    if (currentFState && !g_prevFState) g_isFullMapView = !g_isFullMapView;
    g_prevFState = currentFState;

    bool currentEState = (GetAsyncKeyState('E') & 0x8000) != 0;
    if (currentEState && !g_prevEState) g_showDebugRect = !g_showDebugRect;
    g_prevEState = currentEState;

    bool isW = GetAsyncKeyState('W') & 0x8000;
    bool isA = GetAsyncKeyState('A') & 0x8000;
    bool isS = GetAsyncKeyState('S') & 0x8000;
    bool isD = GetAsyncKeyState('D') & 0x8000;
    bool isSpace = GetAsyncKeyState(VK_SPACE) & 0x8000;

    int maxStepHeight = 15;

    int touchWallDir = 0;
    if (CheckSpecificCollision(pX - 3.0f, pY, colW, colH, 3)) touchWallDir = -1;
    else if (CheckSpecificCollision(pX + 3.0f, pY, colW, colH, 3)) touchWallDir = 1;

    bool inAir = isJumping || (pVy != 0.0f);
    bool isJumpKeyPressed = isW || isSpace;
    if (!isJumpKeyPressed) canJump = true;

    // -------------------------------------------------------------
    // 0. 점프 & 플립 판정
    // -------------------------------------------------------------
    if (isJumpKeyPressed && canJump) {
        if (pState == WALL_GRAB || pState == WALL_SLIDE) {
            pState = WALL_FLIP;
            currentFrame = 0;
            pVy = wallJumpPowerY;
            currentVx = (wallDir == 1) ? -wallJumpPowerX : wallJumpPowerX;
            isFacingRight = (wallDir == -1);
            isJumping = true;
            canJump = false;

            pX += (wallDir == 1) ? -2.0f : 2.0f;
            touchWallDir = 0;
        }
        else if (!inAir && touchWallDir != 0 && ((touchWallDir == -1 && isA) || (touchWallDir == 1 && isD))) {
            pState = WALL_SLIDE;
            currentFrame = 0;
            wallDir = touchWallDir;
            isFacingRight = (wallDir == 1);
            pVy = JUMP_POWER;
            isJumping = true;
            canJump = false;
            canAirYDash = true;
        }
        else if (!inAir && pState != ROLL && pState != ATTACK && pState != WALL_FLIP) {
            pVy = JUMP_POWER;
            isJumping = true;
            canJump = false;
        }
    }

    // -------------------------------------------------------------
    // 1. 마우스 조준 대시 공격
    // -------------------------------------------------------------
    bool currentLButton = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    float worldMouseX = (mouseX - g_mapOffsetX) / g_renderMapScale;
    float worldMouseY = (mouseY - g_mapOffsetY) / g_renderMapScale;
    if (!g_isFullMapView) { worldMouseX += camX; worldMouseY += camY; }

    if (currentLButton && !g_prevLButton && pState != ATTACK && pState != ROLL && pState != PREVDOWN && pState != DOWN) {
        if (currentTime - lastAttackTime >= attackCooldown) {
            pState = ATTACK;
            currentFrame = 0;
            lastAttackTime = currentTime;

            float dx = worldMouseX - (pX + colW / 2.0f);
            float dy = worldMouseY - (pY + colH / 2.0f);
            float dist = sqrt(dx * dx + dy * dy);

            attackAngle = atan2(dy, dx);
            isFacingRight = (dx >= 0);

            if (dist > 0) {
                attackDirX = dx / dist;
                attackDirY = dy / dist;
            }
            else {
                attackDirX = 1.0f; attackDirY = 0.0f;
            }

            if (dy < 0) {
                if (canAirYDash) {
                    canAirYDash = false;
                }
                else {
                    attackDirY = 0.0f;
                    attackDirX = (dx >= 0) ? 1.0f : -1.0f;
                }
            }

            float dashDist = min(dist, dashRadius);
            attackTargetX = pX + attackDirX * dashDist;
            attackTargetY = pY + attackDirY * dashDist;
        }
    }
    g_prevLButton = currentLButton;

    // -------------------------------------------------------------
    // 2. 공중 벽타기(Grab/Slide) 진입 및 탈출
    // -------------------------------------------------------------
    if (inAir && touchWallDir != 0 && pState != ATTACK && pState != ROLL) {
        if (pState != WALL_GRAB && pState != WALL_SLIDE) {
            bool isPressingWall = ((touchWallDir == -1 && isA) || (touchWallDir == 1 && isD));
            bool isFlippingToNewWall = (pState == WALL_FLIP && touchWallDir != wallDir);

            if (isFlippingToNewWall || (pState != WALL_FLIP && isPressingWall)) {
                pState = WALL_GRAB;
                currentFrame = 0;
                wallGrabTime = currentTime;
                wallDir = touchWallDir;
                isFacingRight = (wallDir == 1);
                canAirYDash = true;
            }
        }
        else {
            if ((wallDir == 1 && isA) || (wallDir == -1 && isD)) {
                pState = FALL;
                currentVx = (wallDir == 1) ? -moveSpeedWalk : moveSpeedWalk;
            }
            else if (touchWallDir != wallDir) {
                pState = FALL;
            }
        }
    }
    else if (pState == WALL_GRAB || pState == WALL_SLIDE) {
        if (!inAir) pState = IDLE;
        else pState = FALL;
    }

    // -------------------------------------------------------------
    // 🌟 3. 상태별 X, Y축 이동 및 전환 애니메이션용 목표 속도 제어
    // -------------------------------------------------------------
    float targetVx = 0.0f;
    float currentSpeedLimit = moveSpeedWalk;
    bool isWalkAfterRoll = (isS && (isA || isD) && !canRoll);

    if (pState == ATTACK) {
        float distToTarget = sqrt(pow(attackTargetX - pX, 2) + pow(attackTargetY - pY, 2));
        if (distToTarget > dashSpeed) {
            float nextX = pX + attackDirX * dashSpeed;
            float nextY = pY + attackDirY * dashSpeed;
            if (!CheckMapCollision(nextX, pY, colW, colH)) pX += attackDirX * dashSpeed;
            if (!CheckMapCollision(pX, nextY, colW, colH)) pY += attackDirY * dashSpeed;
        }
        else { pX = attackTargetX; pY = attackTargetY; }
        pVy = 0.0f; currentVx = 0.0f;
    }
    else if (pState == ROLL) {
        currentVx = isFacingRight ? moveSpeedRoll : -moveSpeedRoll;
    }
    else if (pState == WALL_GRAB || pState == WALL_SLIDE) {
        currentVx = 0.0f;
    }
    else if (pState == WALL_FLIP) {
        // 유저 입력 무시하고 날아가기
    }
    else if (pState == IDLE_TO_WALK) {
        // 💡 [핵심] 모션 중에는 매우 느린 속도로 제자리걸음 하는 느낌을 줌
        if (isA) { targetVx = -speedIdleToWalk; isFacingRight = false; }
        if (isD) { targetVx = speedIdleToWalk; isFacingRight = true; }
    }
    else if (pState == WALK_TO_IDLE) {
        // 💡 [핵심] 마찰력(frictionRate)에 의해 자연스럽게 스무스하게 정지
        targetVx = 0.0f;
    }
    else if ((pState == PREVDOWN || pState == DOWN || pState == POSTDOWN) && !isWalkAfterRoll) {}
    else {
        if (isA) { targetVx = -currentSpeedLimit; isFacingRight = false; }
        if (isD) { targetVx = currentSpeedLimit; isFacingRight = true; }
    }

    if (targetVx != 0.0f && pState != ROLL && pState != ATTACK && pState != WALL_GRAB && pState != WALL_SLIDE && pState != WALL_FLIP) {
        currentVx += (targetVx - currentVx) * accelRate;
    }
    else if (pState != ROLL && pState != ATTACK && pState != WALL_GRAB && pState != WALL_SLIDE && pState != WALL_FLIP) {
        currentVx += (0.0f - currentVx) * frictionRate;
        if (fabs(currentVx) < 0.1f) currentVx = 0.0f;
    }

    if (currentVx != 0.0f && pState != ATTACK && pState != WALL_GRAB && pState != WALL_SLIDE) {
        float nextX = pX + currentVx;
        if (!CheckMapCollision(nextX, pY, colW, colH - 5)) {
            pX = nextX;
        }
        else {
            bool steppedUp = false;
            for (int step = 1; step <= maxStepHeight; step++) {
                if (!CheckMapCollision(nextX, pY - step, colW, colH - 5)) {
                    pX = nextX; pY -= step; steppedUp = true; break;
                }
            }
            if (!steppedUp) {
                float sign = (currentVx > 0) ? 1.0f : -1.0f;
                int failsafe = 0;
                while (!CheckMapCollision(pX + sign, pY, colW, colH - 5) && failsafe++ < (int)fabs(currentVx) + 2) {
                    pX += sign;
                }
                if (pState != WALL_FLIP) currentVx = 0.0f;
            }
        }
    }

    if (isS && !isJumping && pState != ROLL && pState != ATTACK && pState != WALL_GRAB && pState != WALL_SLIDE && pState != WALL_FLIP) {
        int fTypeL = GetCollisionType((int)pX, (int)(pY + colH + 1));
        int fTypeC = GetCollisionType((int)(pX + colW / 2), (int)(pY + colH + 1));
        int fTypeR = GetCollisionType((int)(pX + colW), (int)(pY + colH + 1));
        if (fTypeL == 2 || fTypeC == 2 || fTypeR == 2) {
            pY += 4.0f; isJumping = true; pVy = 1.0f;
        }
    }

    // Y축 이동 
    if (pState != ATTACK) {
        if (pState == WALL_GRAB) {
            pVy = 0.0f;
            if (currentTime - wallGrabTime >= wallHangTime) {
                pState = WALL_SLIDE; currentFrame = 0;
            }
        }
        else {
            float currentGravity = (isJumpKeyPressed && pVy < 0.0f) ? GRAVITY_HOLD : GRAVITY_NORMAL;
            pVy += currentGravity;

            float maxFall = MAX_FALL_SPEED;
            if (pState == WALL_SLIDE && pVy >= 0.0f) {
                maxFall = isS ? wallSlideFastSpeed : wallSlideSpeed;
                pVy = maxFall;
            }
            else if (pVy > maxFall) {
                pVy = maxFall;
            }
        }

        float nextY = pY + pVy;

        if (pVy > 0) { // 하강 
            bool hitFloor = false;
            float finalFloorY = nextY;

            if (CheckMapCollision(pX, nextY, colW, colH)) hitFloor = true;
            else {
                for (float checkY = pY; checkY <= nextY; checkY += 1.0f) {
                    int typeL = GetCollisionType((int)pX, (int)(checkY + colH));
                    int typeC = GetCollisionType((int)(pX + colW / 2), (int)(checkY + colH));
                    int typeR = GetCollisionType((int)(pX + colW), (int)(checkY + colH));

                    if (typeL == 2 || typeC == 2 || typeR == 2) {
                        if (pY + colH <= checkY + colH + 2) { hitFloor = true; finalFloorY = checkY; break; }
                    }
                }
            }

            if (hitFloor) {
                isJumping = false; pVy = 0; pY = finalFloorY; canAirYDash = true;
                int failsafe = 0;
                while ((CheckMapCollision(pX, pY, colW, colH) ||
                    GetCollisionType((int)pX, (int)(pY + colH)) == 2 ||
                    GetCollisionType((int)(pX + colW / 2), (int)(pY + colH)) == 2 ||
                    GetCollisionType((int)(pX + colW), (int)(pY + colH)) == 2) && failsafe++ < 100) {
                    pY -= 1.0f;
                }
                pY += 1.0f;
            }
            else {
                pY = nextY;
                int nL = GetCollisionType((int)pX, (int)(pY + colH + 1));
                int nC = GetCollisionType((int)(pX + colW / 2), (int)(pY + colH + 1));
                int nR = GetCollisionType((int)(pX + colW), (int)(pY + colH + 1));
                if (!CheckMapCollision(pX, pY + 1.0f, colW, colH) && nL != 2 && nC != 2 && nR != 2) isJumping = true;
                else { isJumping = false; canAirYDash = true; }
            }
        }
        else if (pVy < 0) { // 상승
            if (CheckMapCollision(pX, nextY, colW, colH)) {
                pVy = 0; pY = nextY;
                int failsafe = 0;
                while (CheckMapCollision(pX, pY, colW, colH) && failsafe++ < 100) pY += 1.0f;
            }
            else pY = nextY;
        }

        int mapLimit = imgMap.IsNull() ? VIRTUAL_HEIGHT : imgMap.GetHeight();
        if (pY + colH > mapLimit - 20) { pY = mapLimit - colH - 20; isJumping = false; pVy = 0; }
    }

    // -------------------------------------------------------------
    // 🌟 4. 애니메이션 상태 머신 (걷기 전환 포함)
    // -------------------------------------------------------------
    PlayerState newState = pState;

    if (pState == ATTACK && currentFrame >= 7) {
        newState = isJumping ? FALL : IDLE;
    }
    else if (pState == WALL_FLIP && currentFrame >= 10) {
        newState = isJumping ? (pVy < 0.0f ? JUMP_UP : FALL) : IDLE;
    }
    else if (pState == ROLL && currentFrame >= 6) {
        if (isA || isD) newState = WALK;
        else newState = isS ? DOWN : IDLE;
    }
    else if (pState == PREVDOWN && currentFrame >= 2) newState = DOWN;
    else if (pState == POSTDOWN && currentFrame >= 2) newState = IDLE;

    if (!isS) {
        canRoll = true;
        if (newState == PREVDOWN || newState == DOWN) newState = POSTDOWN;
    }
    else {
        if (canRoll && (isA || isD) && !isJumping && newState != ROLL && newState != ATTACK && newState != WALL_GRAB && newState != WALL_SLIDE && newState != WALL_FLIP) {
            newState = ROLL; canRoll = false; isFacingRight = isD;
        }
        else if (!canRoll && (isA || isD) && !isJumping && newState != ROLL && newState != ATTACK && newState != WALL_GRAB && newState != WALL_SLIDE && newState != WALL_FLIP) {
            newState = WALK;
        }
        else if (!(isA || isD) && !isJumping && newState != ROLL && newState != ATTACK && newState != PREVDOWN && newState != DOWN && newState != POSTDOWN && newState != WALL_GRAB && newState != WALL_SLIDE && newState != WALL_FLIP) {
            newState = PREVDOWN;
        }
        else if (!(isA || isD) && newState == POSTDOWN) {
            newState = PREVDOWN;
        }
    }

    // 🌟 [핵심] 부드러운 걷기 전환(Transitions) 로직 적용
    if (newState != ROLL && newState != ATTACK && newState != PREVDOWN && newState != DOWN && newState != POSTDOWN && newState != WALL_GRAB && newState != WALL_SLIDE && newState != WALL_FLIP) {
        if (isJumping) {
            newState = (pVy < 0.0f) ? JUMP_UP : FALL;
        }
        else {
            if (isA || isD) {
                if (pState == IDLE_TO_WALK) {
                    if (currentFrame >= 3) newState = WALK;
                    else newState = IDLE_TO_WALK; // 4프레임 끝날때까지 유지
                }
                else if (pState == WALK || pState == RUN) {
                    newState = WALK;
                }
                else {
                    newState = IDLE_TO_WALK; // 정지 상태나 낙하 후 걷기 시작할 때 무조건 전환 모션 발동
                }
            }
            else {
                if (pState == WALK_TO_IDLE) {
                    if (currentFrame >= 4) newState = IDLE;
                    else newState = WALK_TO_IDLE; // 5프레임 끝날때까지 유지
                }
                else if (pState == WALK || pState == RUN || pState == IDLE_TO_WALK || pState == FALL) {
                    newState = WALK_TO_IDLE; // 걷다가 혹은 착지하면서 키 떼면 스무스하게 정지
                }
                else {
                    newState = IDLE;
                }
            }
        }
    }
    else if (!isJumping && (newState == WALL_FLIP || newState == WALL_SLIDE || newState == WALL_GRAB)) {
        // 특수 액션 중 땅에 안착해버리면 즉시 걷기/정지 판정으로 부드럽게 이행
        if (isA || isD) newState = IDLE_TO_WALK;
        else newState = WALK_TO_IDLE;
    }

    if (pState != newState) {
        currentFrame = 0;
        pState = newState;
    }
}

void UpdateAnimation() {
    static DWORD lastTime = GetTickCount();
    DWORD currentTime = GetTickCount();
    DWORD targetDelayMs = aniDelayIdle;

    switch (pState) {
    case IDLE: targetDelayMs = aniDelayIdle; break;
    case IDLE_TO_WALK: targetDelayMs = aniDelayIdleToWalk; break;
    case WALK: targetDelayMs = aniDelayWalk; break;
    case WALK_TO_IDLE: targetDelayMs = aniDelayWalkToIdle; break;
    case RUN:  targetDelayMs = aniDelayRun;  break;
    case JUMP_UP:
    case FALL: targetDelayMs = aniDelayJumpFall; break;
    case PREVDOWN:
    case DOWN:
    case POSTDOWN: targetDelayMs = aniDelayCrouch; break;
    case ROLL: targetDelayMs = aniDelayRoll; break;
    case ATTACK: targetDelayMs = aniDelayAttack; break;
    case WALL_GRAB: targetDelayMs = aniDelayWallGrab; break;
    case WALL_SLIDE: targetDelayMs = aniDelayWallSlide; break;
    case WALL_FLIP: targetDelayMs = aniDelayWallFlip; break;
    }

    if (currentTime - lastTime >= targetDelayMs) {
        currentFrame++;
        lastTime = currentTime;

        if (pState == IDLE && currentFrame >= 11) currentFrame = 0;
        if (pState == IDLE_TO_WALK && currentFrame >= 4) currentFrame = 3;
        if (pState == WALK && currentFrame >= 10) currentFrame = 0;
        if (pState == WALK_TO_IDLE && currentFrame >= 5) currentFrame = 4;
        if (pState == RUN && currentFrame >= 10) currentFrame = 0;
        if (pState == JUMP_UP && currentFrame >= 4) currentFrame = 3;
        if (pState == FALL && currentFrame >= 4) currentFrame = 3;
        if (pState == DOWN && currentFrame >= 1) currentFrame = 0;
        if (pState == ATTACK && currentFrame >= 7) currentFrame = 7;

        if (pState == WALL_GRAB && currentFrame >= 2) currentFrame = 1;
        if (pState == WALL_SLIDE && currentFrame >= 1) currentFrame = 0;
        if (pState == WALL_FLIP && currentFrame >= 11) currentFrame = 10;
    }
}

void UpdateCamera() {
    if (g_isFullMapView) return;
    float mouseOffsetX = (float)(mouseX - (VIRTUAL_WIDTH / 2)) / (VIRTUAL_WIDTH / 2);
    float maxLookAhead = 350.0f;
    float targetCamX = (pX + colW / 2.0f) - (VIRTUAL_WIDTH / g_renderMapScale / 2.0f) + (mouseOffsetX * maxLookAhead);

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
        if (g_isFullMapView) {
            imgMap.Draw(hMemDC, (int)g_mapOffsetX, (int)g_mapOffsetY, (int)(mapW * g_renderMapScale), (int)(mapH * g_renderMapScale), 0, 0, mapW, mapH);
        }
        else {
            imgMap.Draw(hMemDC, 0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, (int)camX, (int)camY, (int)(VIRTUAL_WIDTH / g_renderMapScale), (int)(VIRTUAL_HEIGHT / g_renderMapScale));
        }
    }

    CImage* currentImg = NULL;
    switch (pState) {
    case IDLE:         currentImg = &imgIdle[currentFrame]; break;
    case IDLE_TO_WALK: currentImg = &imgIdleToWalk[min(currentFrame, 3)]; break;
    case WALK:         currentImg = &imgWalk[currentFrame]; break;
    case WALK_TO_IDLE: currentImg = &imgWalkToIdle[min(currentFrame, 4)]; break;
    case RUN:          currentImg = &imgRun[currentFrame]; break;
    case JUMP_UP:      currentImg = &imgJumpUp[min(currentFrame, 3)]; break;
    case FALL:         currentImg = &imgFall[min(currentFrame, 3)]; break;
    case PREVDOWN:     currentImg = &imgPrevDown[min(currentFrame, 1)]; break;
    case DOWN:         currentImg = &imgDown[0]; break;
    case POSTDOWN:     currentImg = &imgPostDown[min(currentFrame, 1)]; break;
    case ROLL:         currentImg = &imgRoll[min(currentFrame, 5)]; break;
    case ATTACK:       currentImg = &imgAttack[min(currentFrame, 6)]; break;
    case WALL_GRAB:    currentImg = &imgWallGrab[min(currentFrame, 1)]; break;
    case WALL_SLIDE:   currentImg = &imgWallSlide[0]; break;
    case WALL_FLIP:    currentImg = &imgWallFlip[min(currentFrame, 10)]; break;
    }

    float vPX = 0.0f, vPY = 0.0f;
    float pFitScale = mapScale;

    if (g_isFullMapView) {
        float fitScale = min((float)VIRTUAL_WIDTH / mapW, (float)VIRTUAL_HEIGHT / mapH);
        float fitX = (VIRTUAL_WIDTH - mapW * fitScale) / 2.0f;
        float fitY = (VIRTUAL_HEIGHT - mapH * fitScale) / 2.0f;
        vPX = pX * fitScale + fitX;
        vPY = pY * fitScale + fitY;
        pFitScale = fitScale;
    }
    else {
        vPX = (pX - camX) * mapScale;
        vPY = (pY - camY) * mapScale;
    }

    float sPW = 0.0f, sPH = 0.0f, drawX = 0.0f, drawY = 0.0f;

    if (currentImg && !currentImg->IsNull()) {
        sPW = currentImg->GetWidth() * playerScale * pFitScale;
        sPH = currentImg->GetHeight() * playerScale * pFitScale;

        drawX = vPX + (colW * pFitScale) / 2.0f - (sPW / 2.0f);
        drawY = vPY + (colH * pFitScale) - sPH;

        if (isFacingRight) {
            currentImg->Draw(hMemDC, (int)drawX, (int)drawY, (int)sPW, (int)sPH);
        }
        else {
            XFORM xForm = { -1.0f, 0.0f, 0.0f, 1.0f, drawX + sPW, drawY };
            SetWorldTransform(hMemDC, &xForm);
            currentImg->Draw(hMemDC, 0, 0, (int)sPW, (int)sPH);
            XFORM xFormIdentity = { 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f };
            SetWorldTransform(hMemDC, &xFormIdentity);
        }
    }

    if (pState == ATTACK && currentFrame < 5) {
        CImage* slashImg = &imgSlashFX[currentFrame];
        if (!slashImg->IsNull()) {
            int sW = (int)(slashImg->GetWidth() * playerScale * pFitScale);
            int sH = (int)(slashImg->GetHeight() * playerScale * pFitScale);

            XFORM xForm;
            xForm.eM11 = cos(attackAngle);
            xForm.eM12 = sin(attackAngle);
            xForm.eM21 = -sin(attackAngle);
            xForm.eM22 = cos(attackAngle);
            xForm.eDx = vPX + (colW * pFitScale) / 2.0f;
            xForm.eDy = vPY + (colH * pFitScale) / 2.0f;
            SetWorldTransform(hMemDC, &xForm);

            slashImg->Draw(hMemDC, -sW / 2, -sH / 2, sW, sH);

            XFORM xFormIdentity = { 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f };
            SetWorldTransform(hMemDC, &xFormIdentity);
        }
    }

    if (g_showDebugRect) {
        HBRUSH greenBrush = CreateSolidBrush(RGB(0, 255, 0));
        RECT pRect = { (int)vPX, (int)vPY, (int)(vPX + colW * pFitScale), (int)(vPY + colH * pFitScale) };
        FrameRect(hMemDC, &pRect, greenBrush);
        DeleteObject(greenBrush);

        if (pState == ATTACK) {
            float hitW = 80.0f * pFitScale;
            float hitH = 60.0f * pFitScale;
            float hitX = vPX + (colW * pFitScale) / 2.0f + attackDirX * 40.0f * pFitScale - hitW / 2.0f;
            float hitY = vPY + (colH * pFitScale) / 2.0f + attackDirY * 40.0f * pFitScale - hitH / 2.0f;
            HBRUSH redBrush = CreateSolidBrush(RGB(255, 0, 0));
            RECT aRect = { (int)hitX, (int)hitY, (int)(hitX + hitW), (int)(hitY + hitH) };
            FrameRect(hMemDC, &aRect, redBrush);
            DeleteObject(redBrush);
        }
    }

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