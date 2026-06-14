#pragma once
#include <windows.h>

class Input {
public:
    static void Update();
    static bool GetKeyDown(int vKey);
    static bool GetKeyUp(int vKey);
    static bool GetKey(int vKey);
    
    static int GetMouseX() { return m_mouseX; }
    static int GetMouseY() { return m_mouseY; }
    static void SetMousePos(int x, int y) { m_mouseX = x; m_mouseY = y; }

private:
    static bool m_prevKey[256];
    static bool m_currKey[256];
    static int m_mouseX;
    static int m_mouseY;
};

