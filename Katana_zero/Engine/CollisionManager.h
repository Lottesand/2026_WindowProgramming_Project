#pragma once
#include <vector>
#include "GameObject.h"

using namespace std;

// 모든 사물의 충돌을 관리하는 심판관 (싱글톤)
class CollisionManager
{
public:
    // 1. 싱글톤 패턴: 어디서든 이 함수만 부르면 똑같은 매니저를 사용할 수 있습니다.
    static CollisionManager& GetInstance()
    {
        static CollisionManager instance;
        return instance;
    }

    // 2. 게임 화면에 나타난 오브젝트를 명부에 등록/삭제하는 함수
    void AddObject(GameObject* obj);
    void RemoveObject(GameObject* obj);

    // 3. 매 프레임마다 명부를 보며 충돌을 검사하는 심장부
    void Update();

private:
    CollisionManager() {}  // 밖에서 함부로 new로 못 만들게 막음
    ~CollisionManager() {}

    // 복사 방지 (싱글톤의 철칙)
    CollisionManager(const CollisionManager&) = delete;
    CollisionManager& operator=(const CollisionManager&) = delete;

private:
    // 충돌 검사를 받을 오브젝트들의 명단 (포인터 배열)
    vector<GameObject*> m_Objects;
}; 
