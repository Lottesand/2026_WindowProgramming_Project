#pragma once
#include <windows.h>

class GameApp
{
public:
    GameApp();
    virtual ~GameApp();

    // 1. 엔진 초기화 및 메인 루프 실행
    bool Initialize(HINSTANCE hInstance, const wchar_t* title, int width, int height);
    int Run();

protected:
    // 2. 파생 클래스(Game 프로젝트)에서 반드시 구현해야 할 함수들
    virtual void Update() = 0;
    virtual void Render() = 0;

private:
    // 3. 윈도우 프로시저 (반드시 static이어야 함)
    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

protected:
    // 자식 클래스에서 접근할 수 있도록 protected로 설정
    HWND m_hWnd;
    HINSTANCE m_hInstance;
    int m_Width;
    int m_Height;
};