#pragma once
#include "GameApp.h"
#include <vector>

// 캐릭터의 현재 상태를 나타내는 열거형
enum class State { IDLE, WALK, RUN };

class Player
{
public:
    Player();
    ~Player();

    void Init();
    void Update();
    void Render(HDC hdc);

private:
    float m_x, m_y;            // 플레이어 좌표
    State m_State;             // 현재 상태
    bool m_isFacingLeft;       // 왼쪽을 보고 있는가? (이미지 좌우 반전을 위함)

    // 애니메이션 프레임 제어용 변수
    int m_CurrentFrame;
    int m_FrameDelay;          // 애니메이션 속도 조절용 카운터

    // 각 상태별 이미지 배열 (CImage 객체들)
    CImage m_ImgIdle[11];
    CImage m_ImgWalk[10];
    CImage m_ImgRun[11];
    // (필요에 따라 전환 모션 배열도 추가)

    // Player.h 의 private 영역에 추가

    // 기존 오른쪽 바라보는 이미지 배열 (R)
    CImage m_ImgIdle_R[11];
    CImage m_ImgWalk_R[10];
    CImage m_ImgRun_R[11];

    // ★ 새로 만들 왼쪽 바라보는 이미지 배열 (L)
    CImage m_ImgIdle_L[11];
    CImage m_ImgWalk_L[10];
    CImage m_ImgRun_L[11];
};