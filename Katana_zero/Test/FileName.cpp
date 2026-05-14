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


// --- [환경 설정 및 비율 조정] ---
const int WIN_WIDTH = 1280;
const int WIN_HEIGHT = 720;

// 💡 사진의 느낌을 살리기 위한 비율 (2.5 ~ 3.0 추천)
float mapScale = 1.0f;
float playerScale = 2.0f;

// 💡 Y축 카메라 고정값 (사진처럼 바닥이 아래쪽에 보이게 설정)
// 이 값을 키우면 화면이 아래로 내려가고, 줄이면 위로 올라갑니다.
float camY_Fixed = 60.0f;

// 마우스 및 카메라 변수
int mouseX = WIN_WIDTH / 2;
float camX = 0.0f;
float camY = camY_Fixed; // Y는 시작부터 고정

float pX = 100.0f, pY = 300.0f;
float pVy = 0.0f; // y축 속도
//float camX = 0.0f, camY = 0.0f; // 카메라 월드 좌표 (좌상단 기준)
const float GRAVITY = 0.5f;
const float JUMP_POWER = -10.0f;
const float SPEED = 10.0f;
bool isJumping = false;
bool isFacingRight = true; // 좌우 반전 처리를 위함

// 애니메이션 프레임 제어
int currentFrame = 0;
int aniDelay = 0;

// 이미지 객체 (CImage)
CImage imgMap;       // 시각적 맵
CImage imgColMap;    // 충돌 검사용 맵 (Col Map)
CImage imgIdle[11];  // Idle 11장
CImage imgWalk[10];  // Walk 10장
CImage imgRun[10];   // Run 10장
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

    // 임시 창 크기
    g_hWnd = CreateWindow(lpszClass, TEXT("Katana Zero Rebirth"), WS_OVERLAPPEDWINDOW, 0, 0, 1280, 720, NULL, (HMENU)NULL, hInstance, NULL);
    ShowWindow(g_hWnd, nCmdShow);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam) {
    HDC hDC, hMemDC;
    PAINTSTRUCT ps;
    HBITMAP hOldBmp, hMemBmp;
    RECT rect;

    switch (iMsg) {
    case WM_CREATE:
        LoadAssets(); // 여기서 에셋 로드 끝! 중복 로드 삭제.
        SetTimer(hWnd, 1, 1000 / 60, NULL);
        break;

    case WM_TIMER:
        UpdatePhysicsAndInput(); // 입력 및 물리 연산
        UpdateAnimation();       // 애니메이션 프레임 업데이트
        UpdateCamera();          // 카메라 업데이트
        InvalidateRect(hWnd, NULL, FALSE);
        break;

    case WM_PAINT: {
        GetClientRect(hWnd, &rect);
        hDC = BeginPaint(hWnd, &ps);

        Render(hDC); // 복잡한 그리기는 전부 Render에서 처리!

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

// --- [에셋 로드 함수] ---
// --- [에셋 로드 함수] ---
void LoadAssets() {
    HRESULT hr;

    // 1. 맵 에셋 로드 (경로를 현재 프로젝트 상황에 맞게 수정하세요)
    // 만약 폴더 없이 프로젝트(코드)와 같은 곳에 파일이 있다면 TEXT("map.png") 로 수정!
    hr = imgMap.Load(TEXT("assets/map.png"));
    if (FAILED(hr)) {
        MessageBox(g_hWnd, TEXT("map.png 로드 실패! 파일 경로를 확인해주세요."), TEXT("에러"), MB_OK);
    }

    hr = imgColMap.Load(TEXT("assets/colmap.png"));
    if (FAILED(hr)) {
        MessageBox(g_hWnd, TEXT("colmap.png 로드 실패!"), TEXT("에러"), MB_OK);
    }

    // 2. 캐릭터 에셋 배열 로드
    TCHAR path[256];
    for (int i = 0; i < 11; i++) {
        // 폴더 구조 없이 파일만 있다면 TEXT("idle_%d.png") 등으로 수정
        wsprintf(path, TEXT("assets/idle/%d.png"), i);
        hr = imgIdle[i].Load(path);
        if (FAILED(hr) && i == 0) { // 너무 많이 뜨지 않게 0번 째 프레임만 체크
            MessageBox(g_hWnd, TEXT("Idle 애니메이션 이미지 로드 실패! 경로 확인!"), TEXT("에러"), MB_OK);
        }
    }
    for (int i = 0; i < 10; i++) {
        wsprintf(path, TEXT("assets/walk/%d.png"), i);
        imgWalk[i].Load(path);
    }
    // ... 나머지 이미지 로드 ...
}

// --- [물리 및 입력 연산 - 9장 GetAsyncKeyState 활용] ---
// --- [물리 및 입력 연산] ---
// --- [물리 및 입력 연산 - 추락 방지 안전장치 적용] ---
void UpdatePhysicsAndInput() {
    bool isMoving = false;

    int charWidth = imgIdle[0].IsNull() ? 40 : imgIdle[0].GetWidth();
    int charHeight = imgIdle[0].IsNull() ? 60 : imgIdle[0].GetHeight();

    // 🚨 [핵심 수정 1] 스프라이트 투명 여백 보정 (발바닥 위치 미세 조정)
    // 캐릭터 이미지에서 실제 발끝이 이미지 맨 아래가 아닐 수 있습니다.
    int footOffsetX = charWidth / 2;     // X축 중심
    // 💡 캐릭터가 계속 파묻힌다면 15라는 숫자를 20, 30으로 늘려보고, 
    // 반대로 공중에 뜬다면 10, 5로 줄여보세요!
    int footOffsetY = charHeight + 35;

    int footX = (int)pX + footOffsetX;
    int footY = (int)pY + footOffsetY;

   // 🚨 [핵심 수정] 계단 및 언덕 자동 오르기 로직 추가
    int maxStepHeight = 15; // 자동으로 오를 수 있는 최대 계단 높이 (픽셀 단위, 언덕이 가파르면 숫자를 키우세요)

    // 좌측 이동 (A)
    if (GetAsyncKeyState('A') & 0x8000) {
        int targetX = footX - (int)SPEED;
        
        // 1. 평지 이동 검사
        if (!CheckCollision(targetX, footY - 5)) {
            pX -= SPEED; 
        } 
        // 2. 평지가 막혔다면, 위로 빈 공간이 있는지(계단인지) 확인
        else {
            for (int step = 1; step <= maxStepHeight; step++) {
                // 발바닥을 step만큼 들어올렸을 때 충돌하지 않는다면?
                if (!CheckCollision(targetX, footY - 5 - step)) {
                    pX -= SPEED; // 앞으로 가면서
                    pY -= step;  // 위로 올라갑니다!
                    break;       // 오르기 성공했으니 루프 탈출
                }
            }
        }
        isFacingRight = false;
        isMoving = true;
    }

    // 우측 이동 (D)
    if (GetAsyncKeyState('D') & 0x8000) {
        int targetX = footX + (int)SPEED;
        
        // 1. 평지 이동 검사
        if (!CheckCollision(targetX, footY - 5)) {
            pX += SPEED; 
        } 
        // 2. 평지가 막혔다면, 계단 오르기 시도
        else {
            for (int step = 1; step <= maxStepHeight; step++) {
                if (!CheckCollision(targetX, footY - 5 - step)) {
                    pX += SPEED;
                    pY -= step; 
                    break;
                }
            }
        }
        isFacingRight = true;
        isMoving = true;
    }
    // 점프
    if ((GetAsyncKeyState('W') & 0x8000 || GetAsyncKeyState(VK_SPACE) & 0x8000) && !isJumping) {
        pVy = JUMP_POWER;
        isJumping = true;
    }

    // --- 중력 및 바닥 충돌 적용 ---
    pVy += GRAVITY;
    int nextY = (int)pY + (int)pVy;
    int nextFootY = nextY + footOffsetY; // 다음 프레임에 도달할 발바닥 Y 좌표

    // 🚨 [핵심 수정 2] 바닥 충돌 시 파묻힌 만큼 밀어내기 (Snapping)
    if (pVy > 0 && CheckCollision(footX, nextFootY)) { // 아래로 떨어지는 중 바닥에 닿았다면
        isJumping = false;
        pVy = 0;

        // 캐릭터가 바닥(검은색 픽셀)에 파묻혀 있다면, 
        // 파묻히지 않을 때까지 위로 1픽셀씩 끌어올립니다.
        while (CheckCollision(footX, (int)pY + footOffsetY)) {
            pY -= 1.0f;
        }
    }
    else {
        // 충돌하지 않았다면 정상적으로 낙하
        pY = nextY;
        isJumping = true;
    }

    // [임시 하드 리미트 안전장치]
    int mapLimit = imgMap.IsNull() ? WIN_HEIGHT : imgMap.GetHeight();
    if (pY + charHeight > mapLimit - 20) {
        pY = mapLimit - charHeight - 20;
        isJumping = false;
        pVy = 0;
    }

    // 애니메이션 상태 갱신
    PlayerState newState = IDLE;

    if (GetAsyncKeyState('S') & 0x8000) {
        newState = ROLL_PREP;
    }
    else if (isJumping) {
        newState = JUMP;
    }
    else if (isMoving) {
        newState = WALK;
    }

    if (pState != newState) {
        currentFrame = 0;
        pState = newState;
    }
}

// --- [충돌 맵 기반 검사 수정] ---
bool CheckCollision(int targetX, int targetY) {
    if (imgColMap.IsNull()) return true; // 맵 로드 실패 시 무한 추락 방지

    // 화면 밖을 벗어나면 낭떠러지/벽으로 간주
    if (targetX < 0 || targetY < 0 || targetX >= imgColMap.GetWidth() || targetY >= imgColMap.GetHeight())
        return true;

    COLORREF pixelColor = imgColMap.GetPixel(targetX, targetY);

    // 💡 수정됨: 순도 100% 검은색(0,0,0)이 아니라, "어두운 색"이면 바닥으로 인식하게 완화
    // R, G, B가 모두 100보다 작으면 검은색(벽/바닥) 영역으로 판단합니다.
    if (GetRValue(pixelColor) == 0 && GetGValue(pixelColor) == 255 && GetBValue(pixelColor) == 0) {
        return true;
    }
    return false;
}

// --- [애니메이션 프레임 갱신 (실제 시간 기반)] ---
void UpdateAnimation() {
    // GetTickCount()는 윈도우가 시작된 후 흐른 시간을 밀리초(1/1000초) 단위로 알려줍니다.
    static DWORD lastTime = GetTickCount();
    DWORD currentTime = GetTickCount();

    // 1프레임을 유지할 목표 시간 (밀리초 단위, 1000 = 1초)
    DWORD targetDelayMs = 100;

    // 상태별로 프레임이 넘어가는 '실제 시간(ms)'을 설정합니다.
    switch (pState) {
    case IDLE: targetDelayMs = 150; break; // 0.15초마다 1프레임
    case WALK: targetDelayMs = 80;  break; // 0.08초마다 1프레임
    case RUN:  targetDelayMs = 50;  break; // 0.05초마다 1프레임
    }

    // 💡 현재 시간과 마지막으로 프레임을 넘긴 시간을 비교해서, 
    // 목표한 시간이 지났을 때만 프레임을 증가시킵니다!
    if (currentTime - lastTime >= targetDelayMs) {
        currentFrame++;
        lastTime = currentTime; // 타이머 리셋
    }
}

// --- [카메라 로직: 플레이어를 부드럽게 추적 (Lerp)] ---
// --- [카메라 로직: 마우스 리드 + 플레이어 지연 추적 + Y축 고정] ---
void UpdateCamera() {
    // 1. 마우스가 화면 중앙에서 얼마나 멀리 있는지 (-0.5 ~ 0.5)
    float mouseOffsetX = (float)(mouseX - (WIN_WIDTH / 2)) / (WIN_WIDTH / 2);

    // 마우스가 카메라를 얼마나 멀리 끌고 갈 것인가
    float maxLookAhead = 350.0f;

    // 2. 목표 X: 플레이어를 중앙에 두되, 마우스 방향으로 쏠림 적용
    float targetCamX = pX - (WIN_WIDTH / mapScale / 2.0f) + (mouseOffsetX * maxLookAhead);

    // 3. 부드러운 이동 (Lerp)
    camX += (targetCamX - camX) * 0.08f;

    // 4. Y축 무조건 고정
    camY = camY_Fixed;

    // 🚨 [핵심 수정] 5. 맵 좌/우 경계 제한
    // 5-1. 왼쪽 끝 막기
    if (camX < 0) camX = 0;

    // 5-2. 오른쪽 끝 막기
    if (!imgMap.IsNull()) {
        // 카메라가 최대로 갈 수 있는 X 좌표 = 맵 전체 너비 - (화면 너비 / 맵 확대 배율)
        float maxCamX = imgMap.GetWidth() - (WIN_WIDTH / mapScale);
        if (camX > maxCamX) camX = maxCamX;
    }
}

void Render(HDC hDC) {
    HDC hMemDC = CreateCompatibleDC(hDC);
    HBITMAP hMemBmp = CreateCompatibleBitmap(hDC, WIN_WIDTH, WIN_HEIGHT);
    HBITMAP hOldBmp = (HBITMAP)SelectObject(hMemDC, hMemBmp);

    SetGraphicsMode(hMemDC, GM_ADVANCED);
    PatBlt(hMemDC, 0, 0, WIN_WIDTH, WIN_HEIGHT, BLACKNESS);

    // 1. 맵 그리기 (고정된 camY 반영)
    if (!imgMap.IsNull()) {
        imgMap.Draw(hMemDC,
            0, 0, WIN_WIDTH, WIN_HEIGHT,
            (int)camX, (int)camY,
            (int)(WIN_WIDTH / mapScale), (int)(WIN_HEIGHT / mapScale)
        );
    }

    // 2. 플레이어 애니메이션 선택
    CImage* currentImg = NULL;
    switch (pState) {
        case IDLE: 
        case JUMP: currentImg = &imgIdle[currentFrame % 11]; break;
        case WALK: currentImg = &imgWalk[currentFrame % 10]; break;
        case RUN:  currentImg = &imgRun[currentFrame % 10]; break;
    }

    // 3. 플레이어 그리기 (맵 배율 좌표 + 독립적 캐릭터 배율)
    if (currentImg && !currentImg->IsNull()) {
        int screenPX = (int)((pX - camX) * mapScale);
        int screenPY = (int)((pY - camY) * mapScale);
        
        int pW = (int)(currentImg->GetWidth() * playerScale);
        int pH = (int)(currentImg->GetHeight() * playerScale);

        if (isFacingRight) {
            currentImg->Draw(hMemDC, screenPX, screenPY, pW, pH);
        } else {
            // 좌우 반전 로직
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