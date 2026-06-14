#include "Input.h"

bool Input::m_prevKey[256] = { false };
bool Input::m_currKey[256] = { false };
int Input::m_mouseX = 0;
int Input::m_mouseY = 0;

void Input::Update() {
    for (int i = 0; i < 256; ++i) {
        m_prevKey[i] = m_currKey[i];
        m_currKey[i] = (GetAsyncKeyState(i) & 0x8000) != 0;
    }
}

bool Input::GetKeyDown(int vKey) {
    return m_currKey[vKey] && !m_prevKey[vKey];
}

bool Input::GetKeyUp(int vKey) {
    return !m_currKey[vKey] && m_prevKey[vKey];
}

bool Input::GetKey(int vKey) {
    return m_currKey[vKey];
}

