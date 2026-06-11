#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#pragma comment(lib, "Gdiplus.lib")
#include "Game.h"
#include "Input.h"
#include "../SceneAndMap/StageManager.h"
#include "../UI/UIManager.h"
#include "../Effects/EffectManager.h"

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
Game* g_game = nullptr;

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdParam, int nCmdShow) {
    WNDCLASSEX WndClass;
    LPCTSTR lpszClass = L"My Window Class";
    
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

    RECT wr = { 0, 0, 1280, 720 };
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

    HWND hWnd = CreateWindow(lpszClass, TEXT("Katana Zero Rebirth"), WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, wr.right - wr.left, wr.bottom - wr.top,
        NULL, (HMENU)NULL, hInstance, NULL);

    ShowWindow(hWnd, nCmdShow);
    
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    g_game = new Game();
    g_game->Init(hWnd, hInstance);
    
    MSG msg;
    while (true) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) break;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            if (g_game) g_game->Update();
            InvalidateRect(hWnd, NULL, FALSE);
        }
    }

    if (g_game) {
        delete g_game;
        g_game = nullptr;
    }

    StageManager::ReleaseAssets();
    UIManager::ReleaseAssets();
    EffectManager::ReleaseAssets();

    Gdiplus::GdiplusShutdown(gdiplusToken);
    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam) {
    switch (iMsg) {
    case WM_CREATE: ShowCursor(FALSE); break;
    case WM_SIZE: if (g_game) g_game->SetWinSize(LOWORD(lParam), HIWORD(lParam)); break;
    case WM_ERASEBKGND: return 1;
    case WM_MOUSEMOVE:
        if (g_game && g_game->GetWinWidth() != 0 && g_game->GetWinHeight() != 0) {
            int mx = (int)(LOWORD(lParam) * (1280.0f / g_game->GetWinWidth()));
            int my = (int)(HIWORD(lParam) * (720.0f / g_game->GetWinHeight()));
            Input::SetMousePos(mx, my);
        }
        break;
    case WM_PAINT: { 
        PAINTSTRUCT ps; 
        HDC hDC = BeginPaint(hWnd, &ps); 
        if (g_game) g_game->Render(hDC); 
        EndPaint(hWnd, &ps); 
        break; 
    }
    case WM_DESTROY: PostQuitMessage(0); break;
    }
    return DefWindowProc(hWnd, iMsg, wParam, lParam);
}


