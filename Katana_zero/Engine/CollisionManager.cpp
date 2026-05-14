#include "CollisionManager.h"

void CollisionManager::AddObject(GameObject* obj)
{
    // 명단에 추가
    m_Objects.push_back(obj);
}

void CollisionManager::RemoveObject(GameObject* obj)
{
    // 명단에서 해당 오브젝트를 찾아서 지우기
    for (auto it = m_Objects.begin(); it != m_Objects.end(); ++it)
    {
        if (*it == obj)
        {
            m_Objects.erase(it);
            break;
        }
    }
}

void CollisionManager::Update()
{
    // 오브젝트가 2개 미만이면 부딪힐 일이 없으니 패스
    if (m_Objects.size() < 2) return;

    // 이중 루프를 돌면서 모든 오브젝트끼리 짝을 지어 검사합니다.
    // (A와 B를 검사했으면 B와 A는 검사할 필요 없으므로 j는 i+1부터 시작)
    for (size_t i = 0; i < m_Objects.size(); ++i)
    {
        for (size_t j = i + 1; j < m_Objects.size(); ++j)
        {
            GameObject* objA = m_Objects[i];
            GameObject* objB = m_Objects[j];

            // 우리가 Math.h에서 만든 AABB 충돌 알고리즘 사용!
            if (Collision::IsOverlap(objA->GetCollider(), objB->GetCollider()))
            {
                // 부딪혔다!!
                // (나중에 GameObject에 OnCollision() 함수를 만들어서 여기서 호출해 줄 겁니다)
                // 예: objA->OnCollision(objB);
            }
        }
    }
}