#pragma once
#pragma once

// 1. 2D 좌표를 표현할 구조체
struct Vector2
{
    float x;
    float y;
};

// 2. 사각형(충돌 박스)을 표현할 구조체
struct RectBox
{
    float left;
    float top;
    float right;
    float bottom;
};

// 3. 충돌 계산 도구함
class Collision
{
public:
    // AABB (사각형 vs 사각형) 충돌 판정 함수
    static bool IsOverlap(const RectBox& a, const RectBox& b)
    {
        // 아까 말한 '절대 겹칠 수 없는 4가지 상황'을 검사합니다.
        if (a.right < b.left) return false;
        if (a.left > b.right) return false;
        if (a.bottom < b.top) return false; // Win32 API는 y축이 아래로 갈수록 증가하므로 bottom이 더 큰 값입니다.
        if (a.top > b.bottom) return false;

        // 위 4개에 하나도 안 걸렸다면? 100% 충돌!
        return true;
    }
};