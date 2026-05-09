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
int GameApp::Run()
{
    MSG msg = {};

    // GetMessage가 아닌 PeekMessage를 사용!
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            // 윈도우 메시지가 없을 때(마우스나 키보드가 가만히 있을 때)도
            // 게임은 계속 업데이트되고 화면이 그려져야 합니다.
            Update();
            Render();
        }
    }
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