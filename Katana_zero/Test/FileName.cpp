#include <windows.h>
#include <atlimage.h> // PNG 파일 로드 및 투명도 처리를 위한 CImage
#include <math.h>

// --- [전역 변수 및 상태 정의] ---
HINSTANCE g_hInst;
HWND g_hWnd;
LPCTSTR lpszClass = L"My Window Class";
LPCTSTR lpszWindowName = L"Window Programming Lab";

// 캐릭터 상태 열거형
enum PlayerState { IDLE, WALK, RUN, JUMP, ROLL_PREP };
PlayerState pState = IDLE;

// ==============================================================================
// 🛠️ [환경 설정 및 튜닝 변수 모음] 🛠️
// ==============================================================================
const int WIN_WIDTH = 1280;
const int WIN_HEIGHT = 720;

// 💡 스케일 및 카메라 조정
float mapScale = 1.0f;
float playerScale = 2.0f;
float camY_Fixed = 60.0f;

// 💡 스프라이트 위치 보정 (발바닥 위치 미세 조정)
int playerFootOffsetX = 0;
int playerFootOffsetY = 35;

// 🌟 [관성 및 속도 관련 변수] 🌟
float currentVx = 0.0f;        // 현재 플레이어의 X축 실제 속도 (관성 적용을 위함)
float moveSpeedWalk = 10.0f;    // 걷기 최고 속도
float moveSpeedRun = 10.0f;   // 뛰기 최고 속도
float accelRate = 0.6f;        // 가속력 (숫자가 클수록 키 누르자마자 최고속도에 도달)
float frictionRate = 0.3f;     // 마찰력/관성 (키를 뗐을 때 멈추는 속도. 낮을수록 얼음판처럼 미끄러짐)

// 🌟 [점프 및 중력 관련 변수] 🌟
const float JUMP_POWER = -10.0f;   // 초기 점프 폭발력 (음수여야 위로 뜀)
const float GRAVITY_NORMAL = 2.0f; // 기본 중력 (떨어질 때 혹은 키를 뗐을 때의 무거운 중력)
const float GRAVITY_HOLD = 1.0f;   // 점프 키를 누르고 있을 때의 가벼운 중력 (체공 시간 증가)
const float MAX_FALL_SPEED = 30.0f;// 최대 낙하 속도 제한

// 💡 애니메이션 딜레이
DWORD aniDelayIdle = 100;
DWORD aniDelayWalk = 50;
DWORD aniDelayRun = 80;
// ==============================================================================

// --- [전체화면(F키) 관련 변수] ---
bool g_isFullMapView = false;
bool g_prevFState = false;

// 마우스 및 카메라 변수
int mouseX = WIN_WIDTH / 2;
float camX = 0.0f;
float camY = camY_Fixed;

float pX = 100.0f, pY = 300.0f;
float pVy = 0.0f; // y축 속도
bool isJumping = false;
bool isFacingRight = true;

// 애니메이션 프레임 제어
int currentFrame = 0;
int aniDelay = 0;

// 이미지 객체 (CImage)
CImage imgMap;
CImage imgColMap;
CImage imgIdle[11];
CImage imgWalk[10];
CImage imgRun[10];
CImage imgIdleToWalk[4];
CImage imgRunToIdle[4];

// --- [함수 선언] ---
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void LoadAssets();
void UpdatePhysicsAndInput();
void UpdateAnimation();
bool CheckCollision(int x, int y);
void UpdateCamera();
void Render(HDC hDC);

// 메인 함수
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdParam, int nCmdShow) {
    HWND hWnd;
    MSG Message;
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

    g_hWnd = CreateWindow(lpszClass, TEXT("Katana Zero Rebirth"), WS_OVERLAPPEDWINDOW, 0, 0, WIN_WIDTH, WIN_HEIGHT, NULL, (HMENU)NULL, hInstance, NULL);
    ShowWindow(g_hWnd, nCmdShow);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam) {
    HDC hDC;
    PAINTSTRUCT ps;
    RECT rect;

    switch (iMsg) {
    case WM_CREATE:
        LoadAssets();
        SetTimer(hWnd, 1, 1000 / 60, NULL);
        break;

    case WM_TIMER:
        UpdatePhysicsAndInput();
        UpdateAnimation();
        UpdateCamera();
        InvalidateRect(hWnd, NULL, FALSE);
        break;

    case WM_PAINT: {
        GetClientRect(hWnd, &rect);
        hDC = BeginPaint(hWnd, &ps);

        Render(hDC);

        EndPaint(hWnd, &ps);
        break;
    }
    case WM_DESTROY:
        KillTimer(hWnd, 1);
        PostQuitMessage(0);
        break;
    }
    return DefWindowProc(hWnd, iMsg, wParam, lParam);
}

void LoadAssets() {
    HRESULT hr;

    hr = imgMap.Load(TEXT("assets/map.png"));
    if (FAILED(hr)) MessageBox(g_hWnd, TEXT("map.png 로드 실패!"), TEXT("에러"), MB_OK);

    hr = imgColMap.Load(TEXT("assets/colmap.png"));
    if (FAILED(hr)) MessageBox(g_hWnd, TEXT("colmap.png 로드 실패!"), TEXT("에러"), MB_OK);

    TCHAR path[256];
    for (int i = 0; i < 11; i++) {
        wsprintf(path, TEXT("assets/idle/%d.png"), i);
        imgIdle[i].Load(path);
    }
    for (int i = 0; i < 10; i++) {
        wsprintf(path, TEXT("assets/walk/%d.png"), i);
        imgWalk[i].Load(path);
    }
    for (int i = 0; i < 10; i++) {
        wsprintf(path, TEXT("assets/run/%d.png"), i);
        imgRun[i].Load(path);
    }
}

// --- [물리 및 입력 연산] ---
void UpdatePhysicsAndInput() {
    bool currentFState = (GetAsyncKeyState('F') & 0x8000) != 0;
    if (currentFState && !g_prevFState) {
        g_isFullMapView = !g_isFullMapView;
    }
    g_prevFState = currentFState;

    int charWidth = imgIdle[0].IsNull() ? 40 : imgIdle[0].GetWidth();
    int charHeight = imgIdle[0].IsNull() ? 60 : imgIdle[0].GetHeight();

    int footOffsetX = (charWidth / 2) + playerFootOffsetX;
    int footOffsetY = charHeight + playerFootOffsetY;

    int footX = (int)pX + footOffsetX;
    int footY = (int)pY + footOffsetY;
    int maxStepHeight = 15;

    // -------------------------------------------------------------
    // 🌟 1. 관성이 적용된 좌우 이동 로직 (가속 및 마찰력)
    // -------------------------------------------------------------
    float targetVx = 0.0f; // 키 입력에 따른 목표 속도
    float currentSpeedLimit = moveSpeedWalk; // 현재 상태에 따른 최고 속도

    // A키와 D키 입력 감지 (동시 입력 시 상쇄되어 targetVx는 0이 됨)
    if (GetAsyncKeyState('A') & 0x8000) {
        targetVx = -currentSpeedLimit;
        isFacingRight = false;
    }
    if (GetAsyncKeyState('D') & 0x8000) {
        targetVx = currentSpeedLimit;
        isFacingRight = true;
    }

    // 부드러운 가속 및 마찰력 적용 (Lerp 원리)
    if (targetVx != 0.0f) {
        // 키를 누르고 있을 때: 가속력(accelRate)만큼 목표 속도를 향해 증가
        currentVx += (targetVx - currentVx) * accelRate;
    }
    else {
        // 키를 뗐을 때: 마찰력(frictionRate)만큼 0을 향해 감소 (관성 미끄러짐)
        currentVx += (0.0f - currentVx) * frictionRate;

        // 속도가 거의 0에 가까워지면 완전히 멈춤 처리
        if (fabs(currentVx) < 0.1f) currentVx = 0.0f;
    }

    // -------------------------------------------------------------
    // 🌟 2. 이동 및 지형 충돌 처리 (X축)
    // -------------------------------------------------------------
    if (currentVx != 0.0f) {
        int nextX = footX + (int)currentVx;

        if (!CheckCollision(nextX, footY - 5)) {
            pX += currentVx;
        }
        else {
            // 벽에 막혔을 때 계단 오르기 시도
            bool steppedUp = false;
            for (int step = 1; step <= maxStepHeight; step++) {
                if (!CheckCollision(nextX, footY - 5 - step)) {
                    pX += currentVx;
                    pY -= step;
                    steppedUp = true;
                    break;
                }
            }
            // 계단 오르기에도 실패했다면 벽에 부딪힌 것이므로 속도를 0으로 깎음
            if (!steppedUp) currentVx = 0.0f;
        }
    }

    // -------------------------------------------------------------
    // 🌟 3. 가변 점프 및 중력 로직 (짧게 누르면 낮게, 길게 누르면 높게)
    // -------------------------------------------------------------
    bool isJumpKeyPressed = (GetAsyncKeyState('W') & 0x8000) || (GetAsyncKeyState(VK_SPACE) & 0x8000);

    // 점프 시작
    if (isJumpKeyPressed && !isJumping) {
        pVy = JUMP_POWER; // 초기 폭발적인 상승력 부여
        isJumping = true;
    }

    // 중력 계산 (키를 누르고 상승 중일 때는 중력을 적게 받아 체공시간이 김)
    float currentGravity = GRAVITY_NORMAL;
    if (isJumpKeyPressed && pVy < 0.0f) {
        currentGravity = GRAVITY_HOLD;
    }

    pVy += currentGravity; // 중력 누적

    // 최대 낙하 속도 제한 (너무 빨리 떨어져서 바닥을 뚫는 현상 방지)
    if (pVy > MAX_FALL_SPEED) pVy = MAX_FALL_SPEED;

    int nextY = (int)pY + (int)pVy;
    int nextFootY = nextY + footOffsetY;

    // -------------------------------------------------------------
    // 🌟 4. Y축 지형 충돌 처리
    // -------------------------------------------------------------
    if (pVy > 0 && CheckCollision(footX, nextFootY)) {
        // 바닥에 닿았을 때
        isJumping = false;
        pVy = 0;

        pY = nextY;
        while (CheckCollision(footX, (int)pY + footOffsetY)) {
            pY -= 1.0f; // 파묻히지 않게 위로 끌어올림
        }
    }
    else if (pVy < 0 && CheckCollision(footX, (int)pY + footOffsetY + (int)pVy - charHeight)) {
        // [선택적] 천장에 머리를 부딪혔을 때 로직 (필요시 사용)
        // pVy = 0.0f; 
        // pY = nextY;
    }
    else {
        // 공중에 떠 있는 중
        pY = nextY;

        // 발밑 1픽셀 아래가 비어있다면 점프 상태로 전환 (절벽에서 떨어질 때)
        if (!CheckCollision(footX, (int)pY + footOffsetY + 1)) {
            isJumping = true;
        }
        else {
            isJumping = false;
        }
    }

    // 맵 하단 추락 방지 안전장치
    int mapLimit = imgMap.IsNull() ? WIN_HEIGHT : imgMap.GetHeight();
    if (pY + charHeight > mapLimit - 20) {
        pY = mapLimit - charHeight - 20;
        isJumping = false;
        pVy = 0;
    }

    // -------------------------------------------------------------
    // 🌟 5. 애니메이션 상태 결정
    // -------------------------------------------------------------
    PlayerState newState = IDLE;

    if (GetAsyncKeyState('S') & 0x8000) {
        newState = ROLL_PREP;
    }
    else if (isJumping) {
        newState = JUMP;
    }
    else if (fabs(currentVx) > 0.5f) { // 속도가 0이 아니고 실제로 걷고 있을 때만 WALK
        newState = WALK;
    }

    // 상태가 변경되었을 때만 프레임 0으로 초기화
    if (pState != newState) {
        currentFrame = 0;
        pState = newState;
    }
}

// --- [충돌 맵 기반 검사] ---
bool CheckCollision(int targetX, int targetY) {
    if (imgColMap.IsNull()) return true;

    if (targetX < 0 || targetY < 0 || targetX >= imgColMap.GetWidth() || targetY >= imgColMap.GetHeight())
        return true;

    COLORREF pixelColor = imgColMap.GetPixel(targetX, targetY);

    if (GetRValue(pixelColor) == 0 && GetGValue(pixelColor) == 255 && GetBValue(pixelColor) == 0) {
        return true;
    }
    return false;
}

// --- [애니메이션 프레임 갱신] ---
void UpdateAnimation() {
    static DWORD lastTime = GetTickCount();
    DWORD currentTime = GetTickCount();

    DWORD targetDelayMs = aniDelayIdle;

    switch (pState) {
    case IDLE: targetDelayMs = aniDelayIdle; break;
    case WALK: targetDelayMs = aniDelayWalk; break;
    case RUN:  targetDelayMs = aniDelayRun;  break;
    case JUMP: targetDelayMs = aniDelayIdle; break;
    }

    if (currentTime - lastTime >= targetDelayMs) {
        currentFrame++;
        lastTime = currentTime;
    }
}

// --- [카메라 로직] ---
void UpdateCamera() {
    if (g_isFullMapView) return;

    float mouseOffsetX = (float)(mouseX - (WIN_WIDTH / 2)) / (WIN_WIDTH / 2);
    float maxLookAhead = 350.0f;
    float targetCamX = pX - (WIN_WIDTH / mapScale / 2.0f) + (mouseOffsetX * maxLookAhead);

    camX += (targetCamX - camX) * 0.08f;
    camY = camY_Fixed;

    if (camX < 0) camX = 0;

    if (!imgMap.IsNull()) {
        float maxCamX = imgMap.GetWidth() - (WIN_WIDTH / mapScale);
        if (camX > maxCamX) camX = maxCamX;
    }
}

// --- [렌더링 함수] ---
void Render(HDC hDC) {
    HDC hMemDC = CreateCompatibleDC(hDC);
    HBITMAP hMemBmp = CreateCompatibleBitmap(hDC, WIN_WIDTH, WIN_HEIGHT);
    HBITMAP hOldBmp = (HBITMAP)SelectObject(hMemDC, hMemBmp);

    SetGraphicsMode(hMemDC, GM_ADVANCED);
    PatBlt(hMemDC, 0, 0, WIN_WIDTH, WIN_HEIGHT, BLACKNESS);

    float renderMapScale = mapScale;
    float renderPlayerScale = playerScale;
    float mapOffsetX = 0.0f;
    float mapOffsetY = 0.0f;

    int mapW = imgMap.IsNull() ? WIN_WIDTH : imgMap.GetWidth();
    int mapH = imgMap.IsNull() ? WIN_HEIGHT : imgMap.GetHeight();

    if (g_isFullMapView && !imgMap.IsNull()) {
        float scaleX = (float)WIN_WIDTH / mapW;
        float scaleY = (float)WIN_HEIGHT / mapH;
        renderMapScale = (scaleX < scaleY) ? scaleX : scaleY;

        renderPlayerScale = playerScale * (renderMapScale / mapScale);

        mapOffsetX = (WIN_WIDTH - (mapW * renderMapScale)) / 2.0f;
        mapOffsetY = (WIN_HEIGHT - (mapH * renderMapScale)) / 2.0f;
    }

    if (!imgMap.IsNull()) {
        if (g_isFullMapView) {
            imgMap.Draw(hMemDC,
                (int)mapOffsetX, (int)mapOffsetY,
                (int)(mapW * renderMapScale), (int)(mapH * renderMapScale),
                0, 0, mapW, mapH);
        }
        else {
            imgMap.Draw(hMemDC,
                0, 0, WIN_WIDTH, WIN_HEIGHT,
                (int)camX, (int)camY,
                (int)(WIN_WIDTH / mapScale), (int)(WIN_HEIGHT / mapScale)
            );
        }
    }

    CImage* currentImg = NULL;
    switch (pState) {
    case IDLE:
    case JUMP: currentImg = &imgIdle[currentFrame % 11]; break;
    case WALK: currentImg = &imgWalk[currentFrame % 10]; break;
    case RUN:  currentImg = &imgRun[currentFrame % 10]; break;
    }

    if (currentImg && !currentImg->IsNull()) {
        int screenPX, screenPY;

        if (g_isFullMapView) {
            screenPX = (int)(pX * renderMapScale) + (int)mapOffsetX;
            screenPY = (int)(pY * renderMapScale) + (int)mapOffsetY;
        }
        else {
            screenPX = (int)((pX - camX) * mapScale);
            screenPY = (int)((pY - camY) * mapScale);
        }

        int pW = (int)(currentImg->GetWidth() * renderPlayerScale);
        int pH = (int)(currentImg->GetHeight() * renderPlayerScale);

        if (isFacingRight) {
            currentImg->Draw(hMemDC, screenPX, screenPY, pW, pH);
        }
        else {
            XFORM xForm = { -1.0f, 0.0f, 0.0f, 1.0f, (float)(screenPX + pW), (float)screenPY };
            SetWorldTransform(hMemDC, &xForm);
            currentImg->Draw(hMemDC, 0, 0, pW, pH);
            XFORM xFormIdentity = { 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f };
            SetWorldTransform(hMemDC, &xFormIdentity);
        }
    }

    BitBlt(hDC, 0, 0, WIN_WIDTH, WIN_HEIGHT, hMemDC, 0, 0, SRCCOPY);
    SelectObject(hMemDC, hOldBmp);
    DeleteObject(hMemBmp);
    DeleteDC(hMemDC);
}