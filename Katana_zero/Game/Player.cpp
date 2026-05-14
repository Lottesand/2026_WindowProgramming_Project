#include "Player.h"

Player::Player() : m_x(500.0f), m_y(500.0f), m_State(State::IDLE), m_isFacingLeft(false), m_CurrentFrame(0), m_FrameDelay(0)
{
}

Player::~Player()
{
    // CImage는 소멸될 때 알아서 메모리를 해제합니다!
}

void Player::Init()
{
    // 파일 경로를 담을 넉넉한 문자열 버퍼 (도화지) 준비
    wchar_t filePath[256];

    // 1. Idle 11장 로드 (0 ~ 10)
    for (int i = 0; i <= 10; ++i)
    {
        // "idle/0.png", "idle/1.png" 식으로 글자를 조합해서 filePath에 저장합니다.
        // (만약 파일 이름이 00.png, 01.png 처럼 두 자리라면 L"idle/%02d.png" 로 바꿔주세요!)
        swprintf_s(filePath, L"idle/%d.png", i);
        m_ImgIdle[i].Load(filePath);
    }

    // 2. Walk 10장 로드 (0 ~ 9)
    for (int i = 0; i <= 9; ++i)
    {
        swprintf_s(filePath, L"walk/%d.png", i);
        m_ImgWalk[i].Load(filePath);
    }

    // 3. Run 11장 로드 (0 ~ 10)
    for (int i = 0; i <= 10; ++i)
    {
        swprintf_s(filePath, L"run/%d.png", i);
        m_ImgRun[i].Load(filePath);
    }
}

void Player::Update()
{
    // 1. 키보드 입력 처리 (A, D 키)
    // GetAsyncKeyState는 윈도우 메시지 대기 없이 실시간으로 키보드 눌림을 체크해서 움직임이 아주 부드럽습니다.
    bool isMoving = false;

    if (GetAsyncKeyState('A') & 0x8000)
    {
        m_x -= 3.0f; // 왼쪽 이동 속도
        m_isFacingLeft = true;
        isMoving = true;
    }
    else if (GetAsyncKeyState('D') & 0x8000)
    {
        m_x += 3.0f; // 오른쪽 이동 속도
        m_isFacingLeft = false;
        isMoving = true;
    }

    // 2. 상태(State) 변경 로직
    State oldState = m_State;

    if (isMoving) {
        m_State = State::RUN; // 일단 RUN으로 처리 (Shift 누르면 RUN, 아니면 WALK로 분기 가능)
    }
    else {
        m_State = State::IDLE;
    }

    // 상태가 바뀌었다면 애니메이션 프레임을 0으로 초기화
    if (oldState != m_State) {
        m_CurrentFrame = 0;
        m_FrameDelay = 0;
    }

    // 3. 애니메이션 프레임 증가 (속도 조절)
    m_FrameDelay++;
    if (m_FrameDelay > 5) // 5번 Update 될 때마다 프레임 1장씩 넘김 (숫자 조절로 애니 속도 변경)
    {
        m_CurrentFrame++;
        m_FrameDelay = 0;

        // 현재 상태의 최대 프레임 수를 넘어가면 다시 0번으로 롤백 (루프)
        if (m_State == State::IDLE && m_CurrentFrame >= 11) m_CurrentFrame = 0;
        if (m_State == State::WALK && m_CurrentFrame >= 10) m_CurrentFrame = 0;
        if (m_State == State::RUN && m_CurrentFrame >= 11) m_CurrentFrame = 0;
    }
}

void Player::Render(HDC hdc)
{
    CImage* currentImage = nullptr;

    // 현재 상태와 프레임에 맞는 이미지 포인터 가져오기
    switch (m_State)
    {
    case State::IDLE: currentImage = &m_ImgIdle[m_CurrentFrame]; break;
    case State::WALK: currentImage = &m_ImgWalk[m_CurrentFrame]; break;
    case State::RUN:  currentImage = &m_ImgRun[m_CurrentFrame];  break;
    }

    // 이미지가 정상적으로 로드되었는지 확인 후 그리기
    if (currentImage != nullptr && !currentImage->IsNull())
    {
        // 💡 에러의 원인이었던 좌우 반전 꼼수(-width) 삭제!
        // 일단은 무조건 원본 방향(오른쪽)으로만 안전하게 그립니다.
        currentImage->Draw(hdc, (int)m_x, (int)m_y);
    }
}


