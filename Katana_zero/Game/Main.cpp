// 만약 여기서 빨간 줄이 뜬다면, 아래 '주의사항'을 확인해 주세요!
#include "GameApp.h" 

// 1. GameApp 엔진을 상속받는 우리만의 게임 클래스 생성
class KatanaZero : public GameApp
{
public:
    // 부모(GameApp)가 시킨 숙제(순수 가상 함수)를 여기서 해결합니다.
    void Update() override
    {
        // 나중에 같이 여기에 '시간 감속', '캐릭터 이동' 로직을 넣을 겁니다.
    }

    void Render() override
    {
        // 나중에 여기에 주인공(제로)과 맵을 그리는 코드를 넣을 겁니다.
    }
};

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