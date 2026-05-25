#include <windows.h>
#include <atlimage.h> 
#include <math.h>
#include <vector>
#include "Enemy.h"

// 전역 변수 설정
HINSTANCE g_hInst;
HWND g_hWnd;
LPCTSTR lpszClass = L"My Window Class";
LPCTSTR lpszWindowName = L"Window Programming Lab";

// 플레이어 상태 정의 및 초기화
enum PlayerState { IDLE, WALK, RUN, JUMP, ROLL_PREP };
PlayerState pState = IDLE;

// 윈도우 창 기본 해상도 설정
const int WIN_WIDTH = 1280;
const int WIN_HEIGHT = 720;

// 카메라 스케일 및 플레이어 스케일 계수
float mapScale = 1.0f;
float playerScale = 2.0f;
float camY_Fixed = 60.0f; // 카메라 Y축 고정값

// 플레이어 충돌 판정 및 드로우 보정용 발밑 오프셋
int playerFootOffsetX = 0;
int playerFootOffsetY = 35;

// 이동 관련 물리 변수 (속도, 가속도, 마찰력)
float currentVx = 0.0f;
float moveSpeedWalk = 10.0f;
float moveSpeedRun = 10.0f;
float accelRate = 0.6f;
float frictionRate = 0.3f;

// 점프 및 중력 시스템 계수
const float JUMP_POWER = -10.0f;
const float GRAVITY_NORMAL = 2.0f; // 일반 낙하 중력
const float GRAVITY_HOLD = 1.0f;   // 점프 키 유지 시 저중력 적용 (가변 점프)
const float MAX_FALL_SPEED = 30.0f; // 최대 낙하 속도 제한

// 상태별 애니메이션 프레임 지연 시간 (ms)
DWORD aniDelayIdle = 100;
DWORD aniDelayWalk = 50;
DWORD aniDelayRun = 80;

// 전체 맵 보기 모드 및 이전 입력 상태 플래그
bool g_isFullMapView = false;
bool g_prevFState = false;

// 마우스 위치 기록 및 카메라 초기 좌표
int mouseX = WIN_WIDTH / 2;
float camX = 0.0f;
float camY = camY_Fixed;

// 플레이어 위치 좌표 및 Y축 속도
float pX = 100.0f, pY = 300.0f;
float pVy = 0.0f;
bool isJumping = false;
bool isFacingRight = true; // 좌우 시선 방향 관리

// 현재 애니메이션 프레임 카운터
int currentFrame = 0;
int aniDelay = 0;

// 이미지 리소스 객체 배열 선언
CImage imgMap;
CImage imgColMap;
CImage imgIdle[11];
CImage imgWalk[10];
CImage imgRun[10];
CImage imgIdleToWalk[4];
CImage imgRunToIdle[4];

// 생성된 몬스터들을 관리하는 전역 벡터 구조
std::vector<Enemy*> g_Enemies;

// 함수 선언부
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void LoadAssets();
void UpdatePhysicsAndInput();
void UpdateAnimation();
bool CheckCollision(int x, int y);
void UpdateCamera();
void Render(HDC hDC);

// 프로그램 메인 진입점
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdParam, int nCmdShow) {
    HWND hWnd;
    MSG Message;
    WNDCLASSEX WndClass;
    g_hInst = hInstance;

    // 윈도우 클래스 구조체 정의
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

    // 메인 윈도우 생성
    g_hWnd = CreateWindow(lpszClass, TEXT("Katana Zero Rebirth"), WS_OVERLAPPEDWINDOW, 0, 0, WIN_WIDTH, WIN_HEIGHT, NULL, (HMENU)NULL, hInstance, NULL);
    ShowWindow(g_hWnd, nCmdShow);

    // 메시지 루프 처리
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}

// 메인 윈도우 프로시저 (메시지 처리 핸들러)
LRESULT CALLBACK WndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam) {
    HDC hDC;
    PAINTSTRUCT ps;
    RECT rect;

    switch (iMsg) {
    case WM_CREATE:
        LoadAssets();

        // 몬스터들을 화면 안쪽으로 배치하여 출력 여부를 확인합니다.
        g_Enemies.push_back(new Gangster(200.0f, 300.0f));
        g_Enemies.push_back(new Grunt(400.0f, 300.0f));
        g_Enemies.push_back(new Pomp(600.0f, 300.0f));
        g_Enemies.push_back(new ShieldCop(800.0f, 300.0f)); // 화면 중앙 영역으로 배치

        for (auto& enemy : g_Enemies) {
            enemy->Init();
        }

    

        // 초당 60프레임 속도로 이벤트를 발생시키는 메인 타이머 가동
        SetTimer(hWnd, 1, 1000 / 60, NULL);
        break;

    case WM_TIMER:
        // 플레이어 물리 현상 및 키보드 입력 업데이트
        UpdatePhysicsAndInput();

        // 몬스터 프레임워크 로직 업데이트 처리
        for (auto& enemy : g_Enemies) {
            enemy->Update();
        }

        // 애니메이션 프레임 흐름 및 카메라 시야 업데이트
        UpdateAnimation();
        UpdateCamera();

        // 화면 갱신 요청 (더블 버퍼링 렌더링 유도)
        InvalidateRect(hWnd, NULL, FALSE);
        break;

    case WM_PAINT: {
        GetClientRect(hWnd, &rect);
        hDC = BeginPaint(hWnd, &ps);

        // 더블 버퍼링 기법을 포함한 메인 렌더링 호출
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

        // 등록된 타이머 해제 및 프로그램 종료 신호 송신
        KillTimer(hWnd, 1);
        PostQuitMessage(0);
        break;
    }
    return DefWindowProc(hWnd, iMsg, wParam, lParam);
}

// 게임 내 리소스 텍스처 로딩 로직
void LoadAssets() {
    HRESULT hr;

    // 배경용 비주얼 맵 로딩
    hr = imgMap.Load(TEXT("assets/map.png"));
    if (FAILED(hr)) MessageBox(g_hWnd, TEXT("map.png 로드 실패!"), TEXT("에러"), MB_OK);

    // 충돌 감지용 컬러 코딩 맵 로딩
    hr = imgColMap.Load(TEXT("assets/colmap.png"));
    if (FAILED(hr)) MessageBox(g_hWnd, TEXT("colmap.png 로드 실패!"), TEXT("에러"), MB_OK);

    // 플레이어 기본 동작 스프라이트 시트 로딩
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

// 플레이어의 이동 입력, 물리 연산 및 지형 충돌 처리 로직
void UpdatePhysicsAndInput() {
    // F키 토글 입력으로 전체 맵 모드 스위칭 처리
    bool currentFState = (GetAsyncKeyState('F') & 0x8000) != 0;
    if (currentFState && !g_prevFState) {
        g_isFullMapView = !g_isFullMapView;
    }
    g_prevFState = currentFState;

    // 플레이어 가상의 충돌 바디 크기 계산 (이미지가 없을 시 기본 크기 대체)
    int charWidth = imgIdle[0].IsNull() ? 40 : imgIdle[0].GetWidth();
    int charHeight = imgIdle[0].IsNull() ? 60 : imgIdle[0].GetHeight();

    // 지형 충돌 감지를 행할 발밑 타겟 좌표 계산 오프셋
    int footOffsetX = (charWidth / 2) + playerFootOffsetX;
    int footOffsetY = charHeight + playerFootOffsetY;

    int footX = (int)pX + footOffsetX;
    int footY = (int)pY + footOffsetY;
    int maxStepHeight = 15; // 계단 및 경사로 탑승 가능한 최대 높이

    float targetVx = 0.0f;
    float currentSpeedLimit = moveSpeedWalk;

    // A, D 이동 키 입력 처리 및 시선 반전 설정
    if (GetAsyncKeyState('A') & 0x8000) {
        targetVx = -currentSpeedLimit;
        isFacingRight = false;
    }
    if (GetAsyncKeyState('D') & 0x8000) {
        targetVx = currentSpeedLimit;
        isFacingRight = true;
    }

    // 선형 보간 기법을 이용한 부드러운 가속 및 마찰력(감속) 계산
    if (targetVx != 0.0f) {
        currentVx += (targetVx - currentVx) * accelRate;
    }
    else {
        currentVx += (0.0f - currentVx) * frictionRate;
        if (fabs(currentVx) < 0.1f) currentVx = 0.0f;
    }

    // X축 이동 속도가 존재할 때의 수평 지형 충돌 판정
    if (currentVx != 0.0f) {
        int nextX = footX + (int)currentVx;

        // 이동하려는 방향 정면에 벽이 없다면 그대로 좌표 가산
        if (!CheckCollision(nextX, footY - 5)) {
            pX += currentVx;
        }
        else {
            // 벽이 감지된다면 계단 판정 루프를 돌려 올라설 수 있는지 체크
            bool steppedUp = false;
            for (int step = 1; step <= maxStepHeight; step++) {
                if (!CheckCollision(nextX, footY - 5 - step)) {
                    pX += currentVx;
                    pY -= step;
                    steppedUp = true;
                    break;
                }
            }
            if (!steppedUp) currentVx = 0.0f; // 계단으로도 못 넘는 벽이면 속도 정지
        }
    }

    // W키 혹은 스페이스바를 통한 점프 키 입력 체크
    bool isJumpKeyPressed = (GetAsyncKeyState('W') & 0x8000) || (GetAsyncKeyState(VK_SPACE) & 0x8000);

    // 바닥에 상주하는 상태에서만 점프 가속을 부여
    if (isJumpKeyPressed && !isJumping) {
        pVy = JUMP_POWER;
        isJumping = true;
    }

    // 카타나 제로 특유의 가변 점프 기능 적용 (점프 키 유지 여부에 따른 중력 조절)
    float currentGravity = GRAVITY_NORMAL;
    if (isJumpKeyPressed && pVy < 0.0f) {
        currentGravity = GRAVITY_HOLD;
    }

    pVy += currentGravity;

    // 종단 속도(최대 낙하 속도) 초과 방지 락
    if (pVy > MAX_FALL_SPEED) pVy = MAX_FALL_SPEED;

    int nextY = (int)pY + (int)pVy;
    int nextFootY = nextY + footOffsetY;

    // Y축 낙하 중 지면 충돌 검사 실행
    if (pVy > 0 && CheckCollision(footX, nextFootY)) {
        isJumping = false;
        pVy = 0;

        pY = nextY;
        // 충돌 색상 안으로 파묻힌 플레이어 좌표를 표면 위로 보정
        while (CheckCollision(footX, (int)pY + footOffsetY)) {
            pY -= 1.0f;
        }
    }
    // 상승 중 천장 충돌 검사
    else if (pVy < 0 && CheckCollision(footX, (int)pY + footOffsetY + (int)pVy - charHeight)) {
    }
    else {
        pY = nextY;

        // 발밑에 바로 한 픽셀 아래 공백이라면 낙하 점프 상태로 전환
        if (!CheckCollision(footX, (int)pY + footOffsetY + 1)) {
            isJumping = true;
        }
        else {
            isJumping = false;
        }
    }

    // 맵 스크롤의 하단 아웃바운드 예외 가이드
    int mapLimit = imgMap.IsNull() ? WIN_HEIGHT : imgMap.GetHeight();
    if (pY + charHeight > mapLimit - 20) {
        pY = mapLimit - charHeight - 20;
        isJumping = false;
        pVy = 0;
    }

    // 플레이어 조건에 부합하는 상태 머신 변환 로직
    PlayerState newState = IDLE;

    if (GetAsyncKeyState('S') & 0x8000) {
        newState = ROLL_PREP;
    }
    else if (isJumping) {
        newState = JUMP;
    }
    else if (fabs(currentVx) > 0.5f) {
        newState = WALK;
    }

    // 상태가 변경되었을 경우 애니메이션 프레임을 다시 첫 번째로 동기화 초기화
    if (pState != newState) {
        currentFrame = 0;
        pState = newState;
    }
}

// 지정 좌표의 픽셀 색상값 분석을 통한 지형 충돌 감지 함수 (Green: 0, 255, 0 장벽 판정)
bool CheckCollision(int targetX, int targetY) {
    if (imgColMap.IsNull()) return true;

    // 맵의 전체 도메인을 이탈하려 할 때 강제 충돌벽 처리
    if (targetX < 0 || targetY < 0 || targetX >= imgColMap.GetWidth() || targetY >= imgColMap.GetHeight())
        return true;

    // 해당 위치 픽셀 색상 추출
    COLORREF pixelColor = imgColMap.GetPixel(targetX, targetY);

    // RGB(0, 255, 0) 순수 초록색 요소를 만나면 이동 불가 벽으로 처리
    if (GetRValue(pixelColor) == 0 && GetGValue(pixelColor) == 255 && GetBValue(pixelColor) == 0) {
        return true;
    }
    return false;
}

// 시간 흐름에 따른 애니메이션 타이밍 및 프레임 증감 제어
void UpdateAnimation() {
    static DWORD lastTime = GetTickCount();
    DWORD currentTime = GetTickCount();

    DWORD targetDelayMs = aniDelayIdle;

    // 현재 플레이어 행동 모드에 따른 프레임 지연율 스위칭
    switch (pState) {
    case IDLE: targetDelayMs = aniDelayIdle; break;
    case WALK: targetDelayMs = aniDelayWalk; break;
    case RUN:  targetDelayMs = aniDelayRun;  break;
    case JUMP: targetDelayMs = aniDelayIdle; break;
    }

    // 설정 지연 속도를 경과했을 시 다음 스프라이트 컷으로 변경
    if (currentTime - lastTime >= targetDelayMs) {
        currentFrame++;
        lastTime = currentTime;
    }
}

// 플레이어 중심의 스무스 카메라 뷰포트 스크롤 업데이트 연산
void UpdateCamera() {
    if (g_isFullMapView) return; // 전체 맵 렌더 상태일 땐 스크롤 추적 스킵

    // 마우스의 중심 편차를 추적하여 화면 미리보기 확장 연산 (LookAhead)
    float mouseOffsetX = (float)(mouseX - (WIN_WIDTH / 2)) / (WIN_WIDTH / 2);
    float maxLookAhead = 350.0f;
    float targetCamX = pX - (WIN_WIDTH / mapScale / 2.0f) + (mouseOffsetX * maxLookAhead);

    // 카메라의 급격한 끊김 보정을 위한 완충 보간 이동 알고리즘
    camX += (targetCamX - camX) * 0.08f;
    camY = camY_Fixed; // 세로 카메라는 기획 수치로 수평 락

    // 좌측 끝 마감 경계 예외 처리
    if (camX < 0) camX = 0;

    // 우측 끝 배경 해상도 초과 방지 예외 처리
    if (!imgMap.IsNull()) {
        float maxCamX = imgMap.GetWidth() - (WIN_WIDTH / mapScale);
        if (camX > maxCamX) camX = maxCamX;
    }
}

// 후면 비트맵 버퍼 기반 더블 버퍼링 화면 출력 로직
void Render(HDC hDC) {
    // 메모리 DC 디바이스 및 가상 호환 비트맵 캔버스 준비
    HDC hMemDC = CreateCompatibleDC(hDC);
    HBITMAP hMemBmp = CreateCompatibleBitmap(hDC, WIN_WIDTH, WIN_HEIGHT);
    HBITMAP hOldBmp = (HBITMAP)SelectObject(hMemDC, hMemBmp);

    // 좌우 스프라이트 대칭 변환 처리를 위한 고급 그래픽 모드 플래그 가동
    SetGraphicsMode(hMemDC, GM_ADVANCED);
    PatBlt(hMemDC, 0, 0, WIN_WIDTH, WIN_HEIGHT, BLACKNESS); // 캔버스 블랙 클리어

    float renderMapScale = mapScale;
    float renderPlayerScale = playerScale;
    float mapOffsetX = 0.0f;
    float mapOffsetY = 0.0f;

    int mapW = imgMap.IsNull() ? WIN_WIDTH : imgMap.GetWidth();
    int mapH = imgMap.IsNull() ? WIN_HEIGHT : imgMap.GetHeight();

    // 디버깅 전용인 F키 전체 맵 출력 연산 스케일러 빌드 수식
    if (g_isFullMapView && !imgMap.IsNull()) {
        float scaleX = (float)WIN_WIDTH / mapW;
        float scaleY = (float)WIN_HEIGHT / mapH;
        renderMapScale = (scaleX < scaleY) ? scaleX : scaleY;

        renderPlayerScale = playerScale * (renderMapScale / mapScale);

        mapOffsetX = (WIN_WIDTH - (mapW * renderMapScale)) / 2.0f;
        mapOffsetY = (WIN_HEIGHT - (mapH * renderMapScale)) / 2.0f;
    }

    // 배경 맵 그래픽 소스 드로우 처리
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

    // 동적 생성된 몬스터(Gangster, Grunt, Pomp, ShieldCop)들의 화면 드로우 패스 실행
    for (auto& enemy : g_Enemies) {
        enemy->Render(hMemDC);
    }

    // 상태 변경 사이클에 동기화할 플레이어 렌더 타겟 포인터 서칭
    CImage* currentImg = NULL;
    switch (pState) {
    case IDLE:
    case JUMP: currentImg = &imgIdle[currentFrame % 11]; break;
    case WALK: currentImg = &imgWalk[currentFrame % 10]; break;
    case RUN:  currentImg = &imgRun[currentFrame % 10]; break;
    }

    // 최종 산출된 플레이어 이미지를 좌표 보정 후 버퍼 화면에 그리기
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

        // 시선 방향에 따른 스프라이트 출력 분기 (우측: 기본 드로우 / 좌측: 월드 변환 행렬 대칭 반전 드로우)
        if (isFacingRight) {
            currentImg->Draw(hMemDC, screenPX, screenPY, pW, pH);
        }
        else {
            XFORM xForm = { -1.0f, 0.0f, 0.0f, 1.0f, (float)(screenPX + pW), (float)screenPY };
            SetWorldTransform(hMemDC, &xForm);
            currentImg->Draw(hMemDC, 0, 0, pW, pH);
            XFORM xFormIdentity = { 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f }; // 연산 종료 후 행렬 원상태 초기화 복구
            SetWorldTransform(hMemDC, &xFormIdentity);
        }
    }

    // 백버퍼의 최종 이미지를 실제 윈도우 스크린 DC 공간으로 일괄 전송 고속 복사 (BitBlt)
    BitBlt(hDC, 0, 0, WIN_WIDTH, WIN_HEIGHT, hMemDC, 0, 0, SRCCOPY);

    // 사용 처리가 끝난 임시 비트맵 및 메모리 DC 컨텍스트 소멸 자원 환수
    SelectObject(hMemDC, hOldBmp);
    DeleteObject(hMemBmp);
    DeleteDC(hMemDC);
}