#include "GameApp.h"

GameApp::GameApp() : m_hWnd(nullptr), m_hInstance(nullptr), m_Width(0), m_Height(0) {}
GameApp::~GameApp() {}

// 1. 윈도우 창 초기화
bool GameApp::Initialize(HINSTANCE hInstance, const wchar_t* title, int width, int height)
{
    m_hInstance = hInstance;
    m_Width = width;
    m_Height = height;

    WNDCLASSEXW wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = GameApp::WindowProc; // static 프로시저 연결!
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = L"KatanaZeroEngine";

    RegisterClassExW(&wcex);

    // 우리가 원하는 해상도(예: 1280x720)가 창의 '테두리'를 제외한 '내부(클라이언트 영역)' 크기가 되도록 보정
    RECT rc = { 0, 0, width, height };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

    // ★ 가장 중요한 부분: 마지막 인자에 'this'를 넘겨줍니다!
    m_hWnd = CreateWindowExW(0, L"KatanaZeroEngine", title, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top,
        nullptr, nullptr, hInstance, this);

    if (!m_hWnd) return false;

    ShowWindow(m_hWnd, SW_SHOW);
    UpdateWindow(m_hWnd);

    return true;
}

// 2. 게임 루프 (심장 박동)
// GameApp.cpp 의 Run() 함수 전체 교체

int GameApp::Run()
{
    MSG msg = {};

    // ★ 더블 버퍼링을 위한 도화지 세팅 준비
    HDC hdc = GetDC(m_hWnd); // 진짜 모니터 화면
    HDC memDC = CreateCompatibleDC(hdc); // 가짜 스케치북 (메모리)

    // 모니터 크기와 똑같은 도화지(비트맵) 만들기
    HBITMAP hBit = CreateCompatibleBitmap(hdc, m_Width, m_Height);
    HBITMAP oldBit = (HBITMAP)SelectObject(memDC, hBit); // 스케치북에 도화지 끼우기

    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            // 1. 게임 로직 업데이트 (좌표 이동)
            Update();

            // 2. 화면 지우기 (스케치북 전체를 하얀색으로 칠함)
            PatBlt(memDC, 0, 0, m_Width, m_Height, WHITENESS);

            // 3. 자식 클래스(KatanaZero)에게 가짜 스케치북(memDC)을 넘겨서 그림을 그리게 함
            Render(memDC);

            // 4. 스케치북에 다 그린 그림을 진짜 모니터(hdc)에 빛의 속도로 복사!! (깜빡임 완벽 제거)
            BitBlt(hdc, 0, 0, m_Width, m_Height, memDC, 0, 0, SRCCOPY);

            Sleep(10);
        }
    }

    // 게임이 끝나면 빌렸던 붓과 스케치북을 모두 반납 (메모리 누수 방지)
    SelectObject(memDC, oldBit);
    DeleteObject(hBit);
    DeleteDC(memDC);
    ReleaseDC(m_hWnd, hdc);

    return (int)msg.wParam;
}

// 3. 정적 윈도우 프로시저
LRESULT CALLBACK GameApp::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    GameApp* pApp = nullptr;

    if (message == WM_NCCREATE)
    {
        // 창이 막 생성될 때, 아까 CreateWindowEx에서 넘긴 'this' 포인터를 낚아채서 윈도우 메모리에 몰래 저장합니다.
        CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        pApp = reinterpret_cast<GameApp*>(pCreate->lpCreateParams);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pApp);
    }
    else
    {
        // 그 이후에는 저장해둔 'this' 포인터를 꺼내서 사용합니다.
        pApp = reinterpret_cast<GameApp*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    }

    if (pApp)
    {
        // 나중에 키보드나 마우스 입력 처리를 pApp->HandleMessage() 형태로 넘겨줄 수 있습니다.
        switch (message)
        {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        }
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}