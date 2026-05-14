#include "GameObject.h"

GameObject::GameObject()
{
    // 태어날 때 좌표와 충돌 박스를 0으로 초기화
    m_Pos = { 0.0f, 0.0f };
    m_Collider = { 0.0f, 0.0f, 0.0f, 0.0f };
}

GameObject::~GameObject()
{
    // 나중에 동적 할당한 이미지나 리소스가 있다면 여기서 지워줍니다.
}