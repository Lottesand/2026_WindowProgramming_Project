#pragma once
#include "GameMath.h" // 우리가 만든 Vector2와 RectBox를 쓰기 위해 포함!

// 게임 화면에 존재하는 모든 사물(액터)의 최상위 부모 클래스
class GameObject
{
public:
    GameObject();
    virtual ~GameObject();

    // 1. 자식들이 무조건 숙제로 해와야 하는 함수 (순수 가상 함수)
    virtual void Update() = 0;
    virtual void Render() = 0;

    // 2. 외부(매니저 등)에서 이 객체의 정보를 볼 수 있게 해주는 함수
    RectBox GetCollider() const { return m_Collider; }
    Vector2 GetPosition() const { return m_Pos; }

    // 3. 위치 세팅 함수
    void SetPosition(float x, float y) { m_Pos.x = x; m_Pos.y = y; }

protected:
    // ★ 주의: private이 아니라 protected입니다!
    // private으로 하면 자식(Player)도 이 변수를 못 건드리지만,
    // protected로 하면 나를 상속받은 자식들은 내 것처럼 자유롭게 쓸 수 있습니다.

    Vector2 m_Pos;       // 현재 좌표 (보통 캐릭터의 발밑 중앙을 기준으로 잡음)
    RectBox m_Collider;  // 충돌 판정용 AABB 박스
}; 
