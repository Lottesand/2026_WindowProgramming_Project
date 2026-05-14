#include "GameApp.h"
#include "Player.h" // 방금 만든 플레이어 헤더 추가!

class KatanaZero : public GameApp
{
    Player m_Player; // 플레이어 객체 생성

public:
    // 게임 시작 시 1번만 실행 (Init 함수 오버라이딩 필요시 GameApp 구조 수정, 아니면 생성자 등에서 호출)
    // 여기서는 가장 간단하게 생성자에서 Init을 호출한다고 가정하겠습니다.
    KatanaZero() {
        m_Player.Init(); // 이미지 로딩!
    }

    void Update() override
    {
        m_Player.Update(); // A, D 키 입력 확인 및 애니메이션 프레임 변경
    }

    void Render(HDC hdc) override
    {
        // 배경에 맵 이미지가 있다면 여기서 먼저 그려주면 됩니다.
        // 예: mapImage.Draw(hdc, 0, 0);
    

        m_Player.Render(hdc); // 플레이어 그리기!
    }
};

// ... wWinMain은 그대로 ...
// 2. 프로그램의 진짜 시작점 (WinMain)
int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    // 우리가 만든 게임 클래스 소환!
    KatanaZero game;

    // 해상도 1280x720, 창 이름은 "Katana Zero"로 초기화
    if (game.Initialize(hInstance, L"Katana Zero", 1280, 720))
    {
        // 성공했다면 엔진의 심장(루프)을 돌린다!
        return game.Run();
    }

    return 0;
}