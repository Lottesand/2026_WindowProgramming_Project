#include "Enemy.h"
#include "Physics.h"
#include "Player.h"
//커밋, 푸시

extern float camX;
extern float camY;
extern float mapScale;
extern bool g_showDebugRect;
extern Player g_Player;

Enemy::Enemy(float startX, float startY, EnemyType type)
{
    m_x = startX;
    m_y = startY;
    m_vx = 2.0f;
    m_vy = 0.0f;
    m_colW = 40.0f;
    m_colH = 60.0f;
    m_isAlive = true;
    m_isFacingLeft = false;
    m_Type = type;
    m_State = EnemyState::IDLE;
    m_CurrentFrame = 0;
    m_LastTime = GetTickCount();

    m_friction = 0.8f;
    m_knockbackVx = 0.0f;

    m_detectionRange = 200.0f; // 기본 감지 범위 (10칸 정도)
    m_isPlayerDetected = false;
}

Enemy::~Enemy() {}
void Enemy::Init() {}

void Enemy::CheckPlayerDetection()
{
    if (!m_isAlive)
    {
        m_isPlayerDetected = false;
        return;
    }

    // 플레이어의 중심점
    float playerCenterX = g_Player.GetX() + g_Player.GetColW() / 2.0f;
    float playerCenterY = g_Player.GetY() + g_Player.GetColH() / 2.0f;

    // 적의 중심점
    float enemyCenterX = m_x + m_colW / 2.0f;
    float enemyCenterY = m_y + m_colH / 2.0f;

    // 가로 거리 계산
    float dx = fabs(playerCenterX - enemyCenterX);
    // 세로 거리 계산 (직선 감지이므로 세로 오차 범위 제한)
    float dy = fabs(playerCenterY - enemyCenterY);

    // 가로 범위 내에 있고, 세로 높이가 비슷할 때 (오차 50픽셀 이내) 감지
    m_isPlayerDetected = (dx <= m_detectionRange && dy <= 50.0f);
}

void Enemy::RenderDebug(HDC hdc)
{
    if (!g_showDebugRect) return;

    int screenX = (int)((m_x - camX) * mapScale);
    int screenY = (int)((m_y - camY) * mapScale);

    // 충돌 박스 (빨간색)
    HBRUSH redBrush = CreateSolidBrush(RGB(255, 0, 0));
    RECT rect = { screenX, screenY, screenX + (int)(m_colW * mapScale), screenY + (int)(m_colH * mapScale) };
    FrameRect(hdc, &rect, redBrush);
    DeleteObject(redBrush);

    // 감지 범위 시각화 (빨간색 가로 직선)
    // 감지 여부에 따라 선 굵기나 스타일 변경 가능 (여기서는 감지 시 진한 빨간색)
    COLORREF detectColor = m_isPlayerDetected ? RGB(255, 0, 0) : RGB(150, 0, 0);
    HPEN hPen = CreatePen(PS_SOLID, m_isPlayerDetected ? 3 : 1, detectColor);
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);

    int range = (int)(m_detectionRange * mapScale);
    int centerX = screenX + (int)(m_colW * mapScale / 2);
    int centerY = screenY + (int)(m_colH * mapScale / 2);

    // 가로로 뻗는 빨간색 직선 그리기
    MoveToEx(hdc, centerX - range, centerY, NULL);
    LineTo(hdc, centerX + range, centerY);

    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);
}

void Enemy::Update()
{
    if (!m_isAlive) {
        // 죽었을 때도 넉백은 적용 (시체 날아가기 효과)
        if (fabs(m_knockbackVx) > 0.1f) {
            float nextX = m_x + m_knockbackVx;
            if (!CheckCollision((int)(nextX + m_colW / 2), (int)(m_y + m_colH * 0.9f))) {
                m_x = nextX;
            }
            m_knockbackVx *= m_friction;
        }
        return;
    }

    // 넉백 처리
    if (fabs(m_knockbackVx) > 0.1f) {
        float nextX = m_x + m_knockbackVx;
        if (!CheckCollision((int)(nextX + m_colW / 2), (int)(m_y + m_colH * 0.9f))) {
            m_x = nextX;
        }
        m_knockbackVx *= m_friction;
    }

    m_vy += 2.0f;
    if (m_vy > 30.0f) m_vy = 30.0f;

    int footX = (int)m_x + (int)(m_colW / 2);
    int nextFootY = (int)m_y + (int)m_colH + (int)m_vy;

    if (CheckCollision(footX, nextFootY))
    {
        m_vy = 0.0f;
        while (CheckCollision(footX, (int)m_y + (int)m_colH)) { m_y -= 1.0f; }
    }
    else
    {
        m_y += m_vy;
    }

    int nextX = footX + (int)m_vx;
    if (!CheckCollision(nextX, (int)m_y + (int)(m_colH * 0.9f)))
    {
        m_x += m_vx;
        m_State = EnemyState::WALK;
    }
    else
    {
        m_vx = -m_vx;
        m_isFacingLeft = (m_vx < 0);
    }

    if (m_vy > 0.1f) { m_State = EnemyState::FALL; }

    DWORD currentTime = GetTickCount();
    if (currentTime - m_LastTime >= 100)
    {
        m_CurrentFrame++;
        m_LastTime = currentTime;
    }
}

void Enemy::Render(HDC hdc) {}

void Enemy::OnTakeDamage(float damage)
{
    m_isAlive = false;
    m_State = EnemyState::DEAD;
}

void Enemy::ApplyKnockback(float vx)
{
    m_knockbackVx = vx;
}

EnemyType Enemy::GetType() const { return m_Type; }
bool Enemy::GetIsAlive() const { return m_isAlive; }


Gangster::Gangster(float startX, float startY) : Enemy(startX, startY, EnemyType::GANGSTER)
{
    m_ActionState = GangsterAction::NONE;
}

Gangster::~Gangster() {}

void Gangster::Init()
{
    wchar_t path[256];

    for (int i = 0; i < 8; ++i)
    {
        swprintf_s(path, L"assets/spr_gangsteridle/%d.png", i); m_ImgIdle_R[i].Load(path);
        swprintf_s(path, L"assets/spr_gangsteridle/%d.png", i); m_ImgIdle_L[i].Load(path);
    }
    for (int i = 0; i < 8; ++i)
    {
        swprintf_s(path, L"assets/spr_gangsterwalk/%d.png", i); m_ImgWalk_R[i].Load(path);
        swprintf_s(path, L"assets/spr_gangsterwalk/%d.png", i); m_ImgWalk_L[i].Load(path);
    }
    for (int i = 0; i < 7; ++i)
    {
        swprintf_s(path, L"assets/spr_gangster_aim/%d.png", i); m_ImgAim_R[i].Load(path);
        swprintf_s(path, L"assets/spr_gangster_aim/%d.png", i); m_ImgAim_L[i].Load(path);
    }
    for (int i = 0; i < 6; ++i)
    {
        swprintf_s(path, L"assets/spr_fire_1/%d.png", i); m_ImgFire_R[i].Load(path);
        swprintf_s(path, L"assets/spr_fire_1/%d.png", i); m_ImgFire_L[i].Load(path);
    }
    for (int i = 0; i < 6; ++i)
    {
        swprintf_s(path, L"assets/spr_gangsterturn/%d.png", i); m_ImgTurn_R[i].Load(path);
        swprintf_s(path, L"assets/spr_gangsterturn/%d.png", i); m_ImgTurn_L[i].Load(path);
    }
    for (int i = 0; i < 12; ++i)
    {
        swprintf_s(path, L"assets/spr_gangsterfall/%d.png", i); m_ImgFall_R[i].Load(path);
        swprintf_s(path, L"assets/spr_gangsterfall/%d.png", i); m_ImgFall_L[i].Load(path);
    }
    for (int i = 0; i < 2; ++i)
    {
        swprintf_s(path, L"assets/spr_gangsterhurtfly/%d.png", i); m_ImgHurtFly_R[i].Load(path);
        swprintf_s(path, L"assets/spr_gangsterhurtfly/%d.png", i); m_ImgHurtFly_L[i].Load(path);
    }
    for (int i = 0; i < 14; ++i)
    {
        swprintf_s(path, L"assets/spr_gangsterhurtground/%d.png", i); m_ImgHurtGround_R[i].Load(path);
        swprintf_s(path, L"assets/spr_gangsterhurtground/%d.png", i); m_ImgHurtGround_L[i].Load(path);
    }
    for (int i = 0; i < 10; ++i)
    {
        swprintf_s(path, L"assets/spr_gangsterrun/%d.png", i); m_ImgRun_R[i].Load(path);
        swprintf_s(path, L"assets/spr_gangsterrun/%d.png", i); m_ImgRun_L[i].Load(path);
    }
}

void Gangster::Update()
{
    CheckPlayerDetection(); // 플레이어 감지 체크 추가

    if (m_ActionState == GangsterAction::HURT_FLY || m_ActionState == GangsterAction::HURT_GROUND || m_State == EnemyState::DEAD)
    {
        if (m_ActionState == GangsterAction::HURT_FLY)
        {
            m_vy += 1.5f;
            m_y += m_vy;
            m_x += m_vx;

            if (CheckCollision((int)m_x + (int)(m_colW / 2), (int)m_y + (int)m_colH))
            {
                m_ActionState = GangsterAction::HURT_GROUND;
                m_State = EnemyState::DEAD;
                m_vx = 0;
                m_vy = 0;
                m_CurrentFrame = 0;
            }
        }

        DWORD currentTime = GetTickCount();
        if (currentTime - m_LastTime >= 100)
        {
            // 쓰러지는 모션(HURT_GROUND)일 때
            if (m_ActionState == GangsterAction::HURT_GROUND || m_State == EnemyState::DEAD)
            {
                // 마지막 프레임 전까지만 증가시킴 (14프레임이면 인덱스 13까지)
                if (m_CurrentFrame < 13)
                {
                    m_CurrentFrame++;
                }
                // m_CurrentFrame이 13이 되면 여기서 멈추므로 루프되지 않음
            }
            else
            {
                // 일반 상태(WALK, IDLE 등)는 계속 루프
                m_CurrentFrame++;
            }
            m_LastTime = currentTime;
        }
        return;
    }

    m_vy += 2.0f;
    if (m_vy > 30.0f) m_vy = 30.0f;

    int footX = (int)m_x + (int)(m_colW / 2);
    int nextFootY = (int)m_y + (int)m_colH + (int)m_vy;

    if (CheckCollision(footX, nextFootY))
    {
        m_vy = 0.0f;
        while (CheckCollision(footX, (int)m_y + (int)m_colH)) { m_y -= 1.0f; }
    }
    else
    {
        m_y += m_vy;
    }

    static DWORD patternTimer = GetTickCount();
    static bool isWaiting = false;
    static float walkDistance = 0.0f;
    const float MAX_WALK_DISTANCE = 150.0f;

    DWORD currentPatternTime = GetTickCount();

    if (isWaiting)
    {
        m_vx = 0.0f;
        m_State = EnemyState::IDLE;

        if (currentPatternTime - patternTimer >= 2000)
        {
            isWaiting = false;
            patternTimer = currentPatternTime;
            walkDistance = 0.0f;
            m_isFacingLeft = !m_isFacingLeft;
            m_vx = m_isFacingLeft ? -2.0f : 2.0f;
        }
    }
    else
    {
        m_State = EnemyState::WALK;
        int nextX = footX + (int)m_vx;

        if (!CheckCollision(nextX, (int)m_y + (int)(m_colH * 0.9f)))
        {
            m_x += m_vx;
            walkDistance += fabs(m_vx);
            m_isFacingLeft = (m_vx < 0.0f);

            if (walkDistance >= MAX_WALK_DISTANCE)
            {
                isWaiting = true;
                patternTimer = currentPatternTime;
            }
        }
        else
        {
            isWaiting = true;
            patternTimer = currentPatternTime;
        }
    }

    if (m_vy > 0.1f) { m_State = EnemyState::FALL; }

    DWORD currentTime = GetTickCount();
    if (currentTime - m_LastTime >= 100)
    {
        m_CurrentFrame++;
        m_LastTime = currentTime;
    }
}

void Gangster::Render(HDC hdc)
{
    int screenX = (int)((m_x - camX) * mapScale);
    int screenY = (int)((m_y - camY) * mapScale);

    CImage* targetImg = nullptr;
    float motionScaleX = 1.0f;
    float motionScaleY = 1.0f;
    float enemyScale = 1.8f; 
    if (m_isFacingLeft)
    {
        if (m_ActionState == GangsterAction::HURT_FLY) { targetImg = &m_ImgHurtFly_L[m_CurrentFrame % 2]; }
        else if (m_ActionState == GangsterAction::HURT_GROUND || m_State == EnemyState::DEAD) { targetImg = &m_ImgHurtGround_L[m_CurrentFrame % 14]; }
        else if (m_State == EnemyState::FALL) { targetImg = &m_ImgFall_L[m_CurrentFrame % 12]; }
        else
        {
            switch (m_ActionState)
            {
            case GangsterAction::NONE:
                if (m_State == EnemyState::IDLE)
                {
                    targetImg = &m_ImgIdle_L[m_CurrentFrame % 8];
                    motionScaleX = 1.1f;
                    motionScaleY = 1.1f;
                }
                else if (m_State == EnemyState::WALK) { targetImg = &m_ImgWalk_L[m_CurrentFrame % 8]; }
                break;
            case GangsterAction::AIM: targetImg = &m_ImgAim_L[m_CurrentFrame % 7]; break;
            case GangsterAction::FIRE: targetImg = &m_ImgFire_L[m_CurrentFrame % 6]; break;
            case GangsterAction::TURN: targetImg = &m_ImgTurn_L[m_CurrentFrame % 6]; break;
            case GangsterAction::RUN: targetImg = &m_ImgRun_L[m_CurrentFrame % 10]; break;
            }
        }
    }
    else
    {
        if (m_ActionState == GangsterAction::HURT_FLY) { targetImg = &m_ImgHurtFly_R[m_CurrentFrame % 2]; }
        else if (m_ActionState == GangsterAction::HURT_GROUND || m_State == EnemyState::DEAD) { targetImg = &m_ImgHurtGround_R[m_CurrentFrame % 14]; }
        else if (m_State == EnemyState::FALL) { targetImg = &m_ImgFall_R[m_CurrentFrame % 12]; }
        else
        {
            switch (m_ActionState)
            {
            case GangsterAction::NONE:
                if (m_State == EnemyState::IDLE)
                {
                    targetImg = &m_ImgIdle_R[m_CurrentFrame % 8];
                    motionScaleX = 1.1f;
                    motionScaleY = 1.1f;
                }
                else if (m_State == EnemyState::WALK) { targetImg = &m_ImgWalk_R[m_CurrentFrame % 8]; }
                break;
            case GangsterAction::AIM: targetImg = &m_ImgAim_R[m_CurrentFrame % 7]; break;
            case GangsterAction::FIRE: targetImg = &m_ImgFire_R[m_CurrentFrame % 6]; break;
            case GangsterAction::TURN: targetImg = &m_ImgTurn_R[m_CurrentFrame % 6]; break;
            case GangsterAction::RUN: targetImg = &m_ImgRun_R[m_CurrentFrame % 10]; break;
            }
        }
    }

    if (targetImg && !targetImg->IsNull())
    {
        int finalW = (int)(targetImg->GetWidth() * enemyScale * motionScaleX * mapScale);
        int finalH = (int)(targetImg->GetHeight() * enemyScale * motionScaleY * mapScale);
        int finalY = screenY + (int)(m_colH * mapScale) - finalH;
        int drawX = screenX + (int)(m_colW * mapScale / 2) - (finalW / 2);

        if (m_isFacingLeft)
        {
            int oldMode = SetGraphicsMode(hdc, GM_ADVANCED);
            XFORM xFormOld;
            GetWorldTransform(hdc, &xFormOld);
            XFORM xFormLeft = { -1.0f, 0.0f, 0.0f, 1.0f, (float)(2 * drawX + finalW), 0.0f };
            SetWorldTransform(hdc, &xFormLeft);
            targetImg->Draw(hdc, drawX, finalY, finalW, finalH);
            SetWorldTransform(hdc, &xFormOld);
            SetGraphicsMode(hdc, oldMode);
        }
        else
        {
            targetImg->Draw(hdc, drawX, finalY, finalW, finalH);
        }
    }

    RenderDebug(hdc);
}

void Gangster::OnTakeDamage(float damage)
{
    m_isAlive = false;
    m_ActionState = GangsterAction::HURT_FLY;
    m_vy = -15.0f;
    m_vx = m_isFacingLeft ? 8.0f : -8.0f;
    m_CurrentFrame = 0;
}


Grunt::Grunt(float startX, float startY) : Enemy(startX, startY, EnemyType::GRUNT)
{
    m_ActionState = GruntAction::NONE;
}

Grunt::~Grunt() {}

void Grunt::Init()
{
    wchar_t path[256];

    for (int i = 0; i < 8; ++i)
    {
        swprintf_s(path, L"assets/spr_grunt_idle/%d.png", i); m_ImgIdle_R[i].Load(path);
        swprintf_s(path, L"assets/spr_grunt_idle/%d.png", i); m_ImgIdle_L[i].Load(path);
    }
    for (int i = 0; i < 10; ++i)
    {
        swprintf_s(path, L"assets/spr_grunt_walk/%d.png", i); m_ImgWalk_R[i].Load(path);
        swprintf_s(path, L"assets/spr_grunt_walk/%d.png", i); m_ImgWalk_L[i].Load(path);
    }
    for (int i = 0; i < 8; ++i)
    {
        swprintf_s(path, L"assets/spr_grunt_attack/%d.png", i); m_ImgAttack_R[i].Load(path);
        swprintf_s(path, L"assets/spr_grunt_attack/%d.png", i); m_ImgAttack_L[i].Load(path);
    }
    for (int i = 0; i < 5; ++i)
    {
        swprintf_s(path, L"assets/spr_grunt_slash/%d.png", i); m_ImgSlash_R[i].Load(path);
        swprintf_s(path, L"assets/spr_grunt_slash/%d.png", i); m_ImgSlash_L[i].Load(path);
    }
    for (int i = 0; i < 8; ++i)
    {
        swprintf_s(path, L"assets/spr_grunt_turn/%d.png", i); m_ImgTurn_R[i].Load(path);
        swprintf_s(path, L"assets/spr_grunt_turn/%d.png", i); m_ImgTurn_L[i].Load(path);
    }
    for (int i = 0; i < 13; ++i)
    {
        swprintf_s(path, L"assets/spr_grunt_fall/%d.png", i); m_ImgFall_R[i].Load(path);
        swprintf_s(path, L"assets/spr_grunt_fall/%d.png", i); m_ImgFall_L[i].Load(path);
    }
    for (int i = 0; i < 2; ++i)
    {
        swprintf_s(path, L"assets/spr_grunt_hurtfly/%d.png", i); m_ImgHurtFly_R[i].Load(path);
        swprintf_s(path, L"assets/spr_grunt_hurtfly/%d.png", i); m_ImgHurtFly_L[i].Load(path);
    }
    for (int i = 0; i < 16; ++i)
    {
        swprintf_s(path, L"assets/spr_grunt_hurtground/%d.png", i); m_ImgHurtGround_R[i].Load(path);
        swprintf_s(path, L"assets/spr_grunt_hurtground/%d.png", i); m_ImgHurtGround_L[i].Load(path);
    }
    for (int i = 0; i < 10; ++i)
    {
        swprintf_s(path, L"assets/spr_grunt_run/%d.png", i); m_ImgRun_R[i].Load(path);
        swprintf_s(path, L"assets/spr_grunt_run/%d.png", i); m_ImgRun_L[i].Load(path);
    }
}

void Grunt::Update()
{
    CheckPlayerDetection();
    if (m_ActionState == GruntAction::HURT_FLY || m_ActionState == GruntAction::HURT_GROUND || m_State == EnemyState::DEAD)
    {
        if (m_ActionState == GruntAction::HURT_FLY)
        {
            m_vy += 1.5f;
            m_y += m_vy;
            m_x += m_vx;

            if (CheckCollision((int)m_x + (int)(m_colW / 2), (int)m_y + (int)m_colH))
            {
                m_ActionState = GruntAction::HURT_GROUND;
                m_State = EnemyState::DEAD;
                m_vx = 0;
                m_vy = 0;
                m_CurrentFrame = 0;
            }
        }

        DWORD currentTime = GetTickCount();
        if (currentTime - m_LastTime >= 100)
        {
            if (m_ActionState == GruntAction::HURT_GROUND || m_State == EnemyState::DEAD)
            {
                if (m_CurrentFrame < 15) { m_CurrentFrame++; }
            }
            else { m_CurrentFrame++; }
            m_LastTime = currentTime;
        }
        return;
    }

    m_vy += 2.0f;
    if (m_vy > 30.0f) m_vy = 30.0f;

    int footX = (int)m_x + (int)(m_colW / 2);
    int nextFootY = (int)m_y + (int)m_colH + (int)m_vy;

    if (CheckCollision(footX, nextFootY))
    {
        m_vy = 0.0f;
        while (CheckCollision(footX, (int)m_y + (int)m_colH)) { m_y -= 1.0f; }
    }
    else { m_y += m_vy; }

    static DWORD patternTimer = GetTickCount();
    static bool isWaiting = false;
    static float walkDistance = 0.0f;
    const float MAX_WALK_DISTANCE = 120.0f;

    DWORD currentPatternTime = GetTickCount();

    if (isWaiting)
    {
        m_vx = 0.0f;
        m_State = EnemyState::IDLE;
        if (currentPatternTime - patternTimer >= 1500)
        {
            isWaiting = false;
            patternTimer = currentPatternTime;
            walkDistance = 0.0f;
            m_isFacingLeft = !m_isFacingLeft;
            m_vx = m_isFacingLeft ? -2.5f : 2.5f;
        }
    }
    else
    {
        m_State = EnemyState::WALK;
        int nextX = footX + (int)m_vx;
        if (!CheckCollision(nextX, (int)m_y + (int)(m_colH * 0.9f)))
        {
            m_x += m_vx;
            walkDistance += fabs(m_vx);
            m_isFacingLeft = (m_vx < 0.0f);
            if (walkDistance >= MAX_WALK_DISTANCE)
            {
                isWaiting = true;
                patternTimer = currentPatternTime;
            }
        }
        else
        {
            isWaiting = true;
            patternTimer = currentPatternTime;
        }
    }

    if (m_vy > 0.1f) { m_State = EnemyState::FALL; }

    DWORD currentTime = GetTickCount();
    if (currentTime - m_LastTime >= 100)
    {
        m_CurrentFrame++;
        m_LastTime = currentTime;
    }
}

void Grunt::Render(HDC hdc) 
{
    int screenX = (int)((m_x - camX) * mapScale);
    int screenY = (int)((m_y - camY) * mapScale);

    CImage* targetImg = nullptr;
    float motionScaleX = 1.0f;
    float motionScaleY = 1.0f;
    float enemyScale = 1.8f; 
    if (m_isFacingLeft)
    {
        if (m_ActionState == GruntAction::HURT_FLY) { targetImg = &m_ImgHurtFly_L[m_CurrentFrame % 2]; }
        else if (m_ActionState == GruntAction::HURT_GROUND || m_State == EnemyState::DEAD) { targetImg = &m_ImgHurtGround_L[m_CurrentFrame % 16]; }
        else if (m_State == EnemyState::FALL) { targetImg = &m_ImgFall_L[m_CurrentFrame % 13]; }
        else
        {
            switch (m_ActionState)
            {
            case GruntAction::NONE:
                if (m_State == EnemyState::IDLE) { targetImg = &m_ImgIdle_L[m_CurrentFrame % 8]; }
                else if (m_State == EnemyState::WALK) { targetImg = &m_ImgWalk_L[m_CurrentFrame % 10]; }
                break;
            case GruntAction::ATTACK: targetImg = &m_ImgAttack_L[m_CurrentFrame % 8]; break;
            case GruntAction::SLASH: targetImg = &m_ImgSlash_L[m_CurrentFrame % 5]; break;
            case GruntAction::TURN: targetImg = &m_ImgTurn_L[m_CurrentFrame % 8]; break;
            case GruntAction::RUN: targetImg = &m_ImgRun_L[m_CurrentFrame % 10]; break;
            }
        }
    }
    else
    {
        if (m_ActionState == GruntAction::HURT_FLY) { targetImg = &m_ImgHurtFly_R[m_CurrentFrame % 2]; }
        else if (m_ActionState == GruntAction::HURT_GROUND || m_State == EnemyState::DEAD) { targetImg = &m_ImgHurtGround_R[m_CurrentFrame % 16]; }
        else if (m_State == EnemyState::FALL) { targetImg = &m_ImgFall_R[m_CurrentFrame % 13]; }
        else
        {
            switch (m_ActionState)
            {
            case GruntAction::NONE:
                if (m_State == EnemyState::IDLE) { targetImg = &m_ImgIdle_R[m_CurrentFrame % 8]; }
                else if (m_State == EnemyState::WALK) { targetImg = &m_ImgWalk_R[m_CurrentFrame % 10]; }
                break;
            case GruntAction::ATTACK: targetImg = &m_ImgAttack_R[m_CurrentFrame % 8]; break;
            case GruntAction::SLASH: targetImg = &m_ImgSlash_R[m_CurrentFrame % 5]; break;
            case GruntAction::TURN: targetImg = &m_ImgTurn_R[m_CurrentFrame % 8]; break;
            case GruntAction::RUN: targetImg = &m_ImgRun_R[m_CurrentFrame % 10]; break;
            }
        }
    }

    if (targetImg && !targetImg->IsNull())
    {
        int finalW = (int)(targetImg->GetWidth() * enemyScale * motionScaleX * mapScale);
        int finalH = (int)(targetImg->GetHeight() * enemyScale * motionScaleY * mapScale);
        int finalY = screenY + (int)(m_colH * mapScale) - finalH;
        int drawX = screenX + (int)(m_colW * mapScale / 2) - (finalW / 2);

        if (m_isFacingLeft)
        {
            int oldMode = SetGraphicsMode(hdc, GM_ADVANCED);
            XFORM xFormOld;
            GetWorldTransform(hdc, &xFormOld);
            XFORM xFormLeft = { -1.0f, 0.0f, 0.0f, 1.0f, (float)(2 * drawX + finalW), 0.0f };
            SetWorldTransform(hdc, &xFormLeft);
            targetImg->Draw(hdc, drawX, finalY, finalW, finalH);
            SetWorldTransform(hdc, &xFormOld);
            SetGraphicsMode(hdc, oldMode);
        }
        else
        {
            targetImg->Draw(hdc, drawX, finalY, finalW, finalH);
        }
    }

    RenderDebug(hdc);
}

void Grunt::OnTakeDamage(float damage)
{
    m_isAlive = false;
    m_ActionState = GruntAction::HURT_FLY;
    m_vy = -15.0f;
    m_vx = m_isFacingLeft ? 8.0f : -8.0f;
    m_CurrentFrame = 0;
}


Pomp::Pomp(float startX, float startY) : Enemy(startX, startY, EnemyType::POMP)
{
    m_ActionState = PompAction::NONE;
}

Pomp::~Pomp() {}

void Pomp::Init()
{
    wchar_t path[256];
    for (int i = 0; i < 8; ++i)
    {
        swprintf_s(path, L"assets/spr_pomp_idle/%d.png", i); m_ImgIdle_R[i].Load(path);
        swprintf_s(path, L"assets/spr_pomp_idle/%d.png", i); m_ImgIdle_L[i].Load(path);
    }
    for (int i = 0; i < 10; ++i)
    {
        swprintf_s(path, L"assets/spr_pomp_walk/%d.png", i); m_ImgWalk_R[i].Load(path);
        swprintf_s(path, L"assets/spr_pomp_walk/%d.png", i); m_ImgWalk_L[i].Load(path);
    }
    for (int i = 0; i < 6; ++i)
    {
        swprintf_s(path, L"assets/spr_pomp_attack/%d.png", i); m_ImgAttack_R[i].Load(path);
        swprintf_s(path, L"assets/spr_pomp_attack/%d.png", i); m_ImgAttack_L[i].Load(path);
    }
    for (int i = 0; i < 10; ++i)
    {
        swprintf_s(path, L"assets/spr_pomp_box_idle/%d.png", i); m_ImgBoxIdle_R[i].Load(path);
        swprintf_s(path, L"assets/spr_pomp_box_idle/%d.png", i); m_ImgBoxIdle_L[i].Load(path);
    }
    for (int i = 0; i < 14; ++i)
    {
        swprintf_s(path, L"assets/spr_pomp_box_hit/%d.png", i); m_ImgBoxHit_R[i].Load(path);
        swprintf_s(path, L"assets/spr_pomp_box_hit/%d.png", i); m_ImgBoxHit_L[i].Load(path);
    }
    for (int i = 0; i < 6; ++i)
    {
        swprintf_s(path, L"assets/spr_pomp_turn/%d.png", i); m_ImgTurn_R[i].Load(path);
        swprintf_s(path, L"assets/spr_pomp_turn/%d.png", i); m_ImgTurn_L[i].Load(path);
    }
    for (int i = 0; i < 13; ++i)
    {
        swprintf_s(path, L"assets/spr_pomp_fall/%d.png", i); m_ImgFall_R[i].Load(path);
        swprintf_s(path, L"assets/spr_pomp_fall/%d.png", i); m_ImgFall_L[i].Load(path);
    }
    for (int i = 0; i < 2; ++i)
    {
        swprintf_s(path, L"assets/spr_pomp_hurtfly/%d.png", i); m_ImgHurtFly_R[i].Load(path);
        swprintf_s(path, L"assets/spr_pomp_hurtfly/%d.png", i); m_ImgHurtFly_L[i].Load(path);
    }
    for (int i = 0; i < 15; ++i)
    {
        swprintf_s(path, L"assets/spr_pomp_hurtground/%d.png", i); m_ImgHurtGround_R[i].Load(path);
        swprintf_s(path, L"assets/spr_pomp_hurtground/%d.png", i); m_ImgHurtGround_L[i].Load(path);
    }
    for (int i = 0; i < 10; ++i)
    {
        swprintf_s(path, L"assets/spr_pomp_run/%d.png", i); m_ImgRun_R[i].Load(path);
        swprintf_s(path, L"assets/spr_pomp_run/%d.png", i); m_ImgRun_L[i].Load(path);
    }
}

void Pomp::Update()
{
    CheckPlayerDetection();
    if (m_ActionState == PompAction::HURT_FLY || m_ActionState == PompAction::HURT_GROUND || m_State == EnemyState::DEAD)
    {
        if (m_ActionState == PompAction::HURT_FLY)
        {
            m_vy += 1.5f;
            m_y += m_vy;
            m_x += m_vx;
            if (CheckCollision((int)m_x + (int)(m_colW / 2), (int)m_y + (int)m_colH))
            {
                m_ActionState = PompAction::HURT_GROUND;
                m_State = EnemyState::DEAD;
                m_vx = 0; m_vy = 0; m_CurrentFrame = 0;
            }
        }
        DWORD currentTime = GetTickCount();
        if (currentTime - m_LastTime >= 100)
        {
            if (m_ActionState == PompAction::HURT_GROUND || m_State == EnemyState::DEAD)
            {
                if (m_CurrentFrame < 14) { m_CurrentFrame++; }
            }
            else { m_CurrentFrame++; }
            m_LastTime = currentTime;
        }
        return;
    }

    m_vy += 2.0f;
    if (m_vy > 30.0f) m_vy = 30.0f;

    int footX = (int)m_x + (int)(m_colW / 2);
    int nextFootY = (int)m_y + (int)m_colH + (int)m_vy;

    if (CheckCollision(footX, nextFootY))
    {
        m_vy = 0.0f;
        while (CheckCollision(footX, (int)m_y + (int)m_colH)) { m_y -= 1.0f; }
    }
    else { m_y += m_vy; }

    static DWORD patternTimer = GetTickCount();
    static bool isWaiting = false;
    static float walkDistance = 0.0f;
    const float MAX_WALK_DISTANCE = 100.0f;

    DWORD currentPatternTime = GetTickCount();

    if (isWaiting)
    {
        m_vx = 0.0f;
        m_State = EnemyState::IDLE;
        if (currentPatternTime - patternTimer >= 1800)
        {
            isWaiting = false;
            patternTimer = currentPatternTime;
            walkDistance = 0.0f;
            m_isFacingLeft = !m_isFacingLeft;
            m_vx = m_isFacingLeft ? -2.2f : 2.2f;
        }
    }
    else
    {
        m_State = EnemyState::WALK;
        int nextX = footX + (int)m_vx;
        if (!CheckCollision(nextX, (int)m_y + (int)(m_colH * 0.9f)))
        {
            m_x += m_vx;
            walkDistance += fabs(m_vx);
            m_isFacingLeft = (m_vx < 0.0f);
            if (walkDistance >= MAX_WALK_DISTANCE)
            {
                isWaiting = true;
                patternTimer = currentPatternTime;
            }
        }
        else
        {
            isWaiting = true;
            patternTimer = currentPatternTime;
        }
    }

    if (m_vy > 0.1f) { m_State = EnemyState::FALL; }

    DWORD currentTime = GetTickCount();
    if (currentTime - m_LastTime >= 100)
    {
        m_CurrentFrame++;
        m_LastTime = currentTime;
    }
}

void Pomp::Render(HDC hdc)
{
    int screenX = (int)((m_x - camX) * mapScale);
    int screenY = (int)((m_y - camY) * mapScale);

    CImage* targetImg = nullptr;
    float motionScaleX = 1.0f;
    float motionScaleY = 1.0f;
    float enemyScale = 1.8f; 
    if (m_isFacingLeft)
    {
        if (m_ActionState == PompAction::HURT_FLY) { targetImg = &m_ImgHurtFly_L[m_CurrentFrame % 2]; }
        else if (m_ActionState == PompAction::HURT_GROUND || m_State == EnemyState::DEAD) { targetImg = &m_ImgHurtGround_L[m_CurrentFrame % 15]; }
        else if (m_State == EnemyState::FALL) { targetImg = &m_ImgFall_L[m_CurrentFrame % 13]; }
        else
        {
            switch (m_ActionState)
            {
            case PompAction::NONE:
                if (m_State == EnemyState::IDLE) { targetImg = &m_ImgIdle_L[m_CurrentFrame % 8]; }
                else if (m_State == EnemyState::WALK) { targetImg = &m_ImgWalk_L[m_CurrentFrame % 10]; }
                break;
            case PompAction::ATTACK: targetImg = &m_ImgAttack_L[m_CurrentFrame % 6]; break;
            case PompAction::BOX_IDLE: targetImg = &m_ImgBoxIdle_L[m_CurrentFrame % 10]; break;
            case PompAction::BOX_HIT: targetImg = &m_ImgBoxHit_L[m_CurrentFrame % 14]; break;
            case PompAction::TURN: targetImg = &m_ImgTurn_L[m_CurrentFrame % 6]; break;
            case PompAction::RUN: targetImg = &m_ImgRun_L[m_CurrentFrame % 10]; break;
            }
        }
    }
    else
    {
        if (m_ActionState == PompAction::HURT_FLY) { targetImg = &m_ImgHurtFly_R[m_CurrentFrame % 2]; }
        else if (m_ActionState == PompAction::HURT_GROUND || m_State == EnemyState::DEAD) { targetImg = &m_ImgHurtGround_R[m_CurrentFrame % 15]; }
        else if (m_State == EnemyState::FALL) { targetImg = &m_ImgFall_R[m_CurrentFrame % 13]; }
        else
        {
            switch (m_ActionState)
            {
            case PompAction::NONE:
                if (m_State == EnemyState::IDLE) { targetImg = &m_ImgIdle_R[m_CurrentFrame % 8]; }
                else if (m_State == EnemyState::WALK) { targetImg = &m_ImgWalk_R[m_CurrentFrame % 10]; }
                break;
            case PompAction::ATTACK: targetImg = &m_ImgAttack_R[m_CurrentFrame % 6]; break;
            case PompAction::BOX_IDLE: targetImg = &m_ImgBoxIdle_R[m_CurrentFrame % 10]; break;
            case PompAction::BOX_HIT: targetImg = &m_ImgBoxHit_R[m_CurrentFrame % 14]; break;
            case PompAction::TURN: targetImg = &m_ImgTurn_R[m_CurrentFrame % 6]; break;
            case PompAction::RUN: targetImg = &m_ImgRun_R[m_CurrentFrame % 10]; break;
            }
        }
    }

    if (targetImg && !targetImg->IsNull())
    {
        int finalW = (int)(targetImg->GetWidth() * enemyScale * motionScaleX * mapScale);
        int finalH = (int)(targetImg->GetHeight() * enemyScale * motionScaleY * mapScale);
        int finalY = screenY + (int)(m_colH * mapScale) - finalH;
        int drawX = screenX + (int)(m_colW * mapScale / 2) - (finalW / 2);

        if (m_isFacingLeft)
        {
            int oldMode = SetGraphicsMode(hdc, GM_ADVANCED);
            XFORM xFormOld;
            GetWorldTransform(hdc, &xFormOld);
            XFORM xFormLeft = { -1.0f, 0.0f, 0.0f, 1.0f, (float)(2 * drawX + finalW), 0.0f };
            SetWorldTransform(hdc, &xFormLeft);
            targetImg->Draw(hdc, drawX, finalY, finalW, finalH);
            SetWorldTransform(hdc, &xFormOld);
            SetGraphicsMode(hdc, oldMode);
        }
        else
        {
            targetImg->Draw(hdc, drawX, finalY, finalW, finalH);
        }
    }

    RenderDebug(hdc);
}

void Pomp::OnTakeDamage(float damage)
{
    m_isAlive = false;
    m_ActionState = PompAction::HURT_FLY;
    m_vy = -15.0f;
    m_vx = m_isFacingLeft ? 8.0f : -8.0f;
    m_CurrentFrame = 0;
}


ShieldCop::ShieldCop(float startX, float startY) : Enemy(startX, startY, EnemyType::SHIELDCOP)
{
    m_ActionState = ShieldCopAction::NONE;
}

ShieldCop::~ShieldCop() {}

void ShieldCop::Init()
{
    wchar_t path[256];
    for (int i = 0; i < 6; ++i)
    {
        swprintf_s(path, L"assets/spr_shieldcop_idle/%d.png", i); m_ImgIdle_R[i].Load(path);
        swprintf_s(path, L"assets/spr_shieldcop_idle/%d.png", i); m_ImgIdle_L[i].Load(path);
    }
    for (int i = 0; i < 10; ++i)
    {
        swprintf_s(path, L"assets/spr_shieldcop_walk/%d.png", i); m_ImgWalk_R[i].Load(path);
        swprintf_s(path, L"assets/spr_shieldcop_walk/%d.png", i); m_ImgWalk_L[i].Load(path);
    }
    for (int i = 0; i < 10; ++i)
    {
        swprintf_s(path, L"assets/spr_shieldcop_run/%d.png", i); m_ImgRun_R[i].Load(path);
        swprintf_s(path, L"assets/spr_shieldcop_run/%d.png", i); m_ImgRun_L[i].Load(path);
    }
    for (int i = 0; i < 8; ++i)
    {
        swprintf_s(path, L"assets/spr_shieldcop_turn/%d.png", i); m_ImgTurn_R[i].Load(path);
        swprintf_s(path, L"assets/spr_shieldcop_turn/%d.png", i); m_ImgTurn_L[i].Load(path);
    }
    for (int i = 0; i < 19; ++i)
    {
        swprintf_s(path, L"assets/spr_shieldcop_aim/%d.png", i); m_ImgAim_R[i].Load(path);
        swprintf_s(path, L"assets/spr_shieldcop_aim/%d.png", i); m_ImgAim_L[i].Load(path);
    }
    for (int i = 0; i < 6; ++i)
    {
        swprintf_s(path, L"assets/spr_shieldcop_bash/%d.png", i); m_ImgBash_R[i].Load(path);
        swprintf_s(path, L"assets/spr_shieldcop_bash/%d.png", i); m_ImgBash_L[i].Load(path);
    }
    for (int i = 0; i < 2; ++i)
    {
        swprintf_s(path, L"assets/spr_shieldcop_knockback/%d.png", i); m_ImgKnockback_R[i].Load(path);
        swprintf_s(path, L"assets/spr_shieldcop_knockback/%d.png", i); m_ImgKnockback_L[i].Load(path);
    }
    for (int i = 0; i < 15; ++i)
    {
        swprintf_s(path, L"assets/spr_shieldcop_tragedy_die_1/%d.png", i); m_ImgTragedyDie_R[i].Load(path);
        swprintf_s(path, L"assets/spr_shieldcop_tragedy_die_1/%d.png", i); m_ImgTragedyDie_L[i].Load(path);
    }
}

void ShieldCop::Update()
{
    CheckPlayerDetection();
    if (m_ActionState == ShieldCopAction::HURT_FLY || m_ActionState == ShieldCopAction::HURT_GROUND || m_State == EnemyState::DEAD)
    {
        if (m_ActionState == ShieldCopAction::HURT_FLY)
        {
            m_vy += 1.5f;
            m_y += m_vy;
            m_x += m_vx;
            if (CheckCollision((int)m_x + (int)(m_colW / 2), (int)m_y + (int)m_colH))
            {
                m_ActionState = ShieldCopAction::HURT_GROUND;
                m_State = EnemyState::DEAD;
                m_vx = 0; m_vy = 0; m_CurrentFrame = 0;
            }
        }
        DWORD currentTime = GetTickCount();
        if (currentTime - m_LastTime >= 100)
        {
            if (m_ActionState == ShieldCopAction::HURT_GROUND || m_State == EnemyState::DEAD)
            {
                if (m_CurrentFrame < 14) { m_CurrentFrame++; }
            }
            else { m_CurrentFrame++; }
            m_LastTime = currentTime;
        }
        return;
    }

    m_vy += 2.0f;
    if (m_vy > 30.0f) m_vy = 30.0f;

    int footX = (int)m_x + (int)(m_colW / 2);
    int nextFootY = (int)m_y + (int)m_colH + (int)m_vy;

    if (CheckCollision(footX, nextFootY))
    {
        m_vy = 0.0f;
        while (CheckCollision(footX, (int)m_y + (int)m_colH)) { m_y -= 1.0f; }
    }
    else { m_y += m_vy; }

    static DWORD patternTimer = GetTickCount();
    static bool isWaiting = false;
    static float walkDistance = 0.0f;
    const float MAX_WALK_DISTANCE = 140.0f;

    DWORD currentPatternTime = GetTickCount();

    if (isWaiting)
    {
        m_vx = 0.0f;
        m_State = EnemyState::IDLE;
        if (currentPatternTime - patternTimer >= 2200)
        {
            isWaiting = false;
            patternTimer = currentPatternTime;
            walkDistance = 0.0f;
            m_isFacingLeft = !m_isFacingLeft;
            m_vx = m_isFacingLeft ? -1.8f : 1.8f;
        }
    }
    else
    {
        m_State = EnemyState::WALK;
        int nextX = footX + (int)m_vx;
        if (!CheckCollision(nextX, (int)m_y + (int)(m_colH * 0.9f)))
        {
            m_x += m_vx;
            walkDistance += fabs(m_vx);
            m_isFacingLeft = (m_vx < 0.0f);
            if (walkDistance >= MAX_WALK_DISTANCE)
            {
                isWaiting = true;
                patternTimer = currentPatternTime;
            }
        }
        else
        {
            isWaiting = true;
            patternTimer = currentPatternTime;
        }
    }

    if (m_vy > 0.1f) { m_State = EnemyState::FALL; }

    DWORD currentTime = GetTickCount();
    if (currentTime - m_LastTime >= 100)
    {
        m_CurrentFrame++;
        m_LastTime = currentTime;
    }
}

void ShieldCop::Render(HDC hdc)
{
    int screenX = (int)((m_x - camX) * mapScale);
    int screenY = (int)((m_y - camY) * mapScale);

    CImage* targetImg = nullptr;
    float motionScaleX = 1.0f;
    float motionScaleY = 1.0f;
    float enemyScale = 1.8f; 
    if (m_isFacingLeft)
    {
        if (m_ActionState == ShieldCopAction::HURT_FLY) { targetImg = &m_ImgKnockback_L[m_CurrentFrame % 2]; }
        else if (m_ActionState == ShieldCopAction::HURT_GROUND || m_State == EnemyState::DEAD) { targetImg = &m_ImgTragedyDie_L[m_CurrentFrame % 15]; }
        else if (m_State == EnemyState::FALL) { targetImg = &m_ImgIdle_L[m_CurrentFrame % 6]; }
        else
        {
            switch (m_ActionState)
            {
            case ShieldCopAction::NONE:
                if (m_State == EnemyState::IDLE) { targetImg = &m_ImgIdle_L[m_CurrentFrame % 6]; }
                else if (m_State == EnemyState::WALK) { targetImg = &m_ImgWalk_L[m_CurrentFrame % 10]; }
                break;
            case ShieldCopAction::AIM: targetImg = &m_ImgAim_L[m_CurrentFrame % 19]; break;
            case ShieldCopAction::BASH: targetImg = &m_ImgBash_L[m_CurrentFrame % 6]; break;
            case ShieldCopAction::TURN: targetImg = &m_ImgTurn_L[m_CurrentFrame % 8]; break;
            case ShieldCopAction::RUN: targetImg = &m_ImgRun_L[m_CurrentFrame % 10]; break;
            }
        }
    }
    else
    {
        if (m_ActionState == ShieldCopAction::HURT_FLY) { targetImg = &m_ImgKnockback_R[m_CurrentFrame % 2]; }
        else if (m_ActionState == ShieldCopAction::HURT_GROUND || m_State == EnemyState::DEAD) { targetImg = &m_ImgTragedyDie_R[m_CurrentFrame % 15]; }
        else if (m_State == EnemyState::FALL) { targetImg = &m_ImgIdle_R[m_CurrentFrame % 6]; }
        else
        {
            switch (m_ActionState)
            {
            case ShieldCopAction::NONE:
                if (m_State == EnemyState::IDLE) { targetImg = &m_ImgIdle_R[m_CurrentFrame % 6]; }
                else if (m_State == EnemyState::WALK) { targetImg = &m_ImgWalk_R[m_CurrentFrame % 10]; }
                break;
            case ShieldCopAction::AIM: targetImg = &m_ImgAim_R[m_CurrentFrame % 19]; break;
            case ShieldCopAction::BASH: targetImg = &m_ImgBash_R[m_CurrentFrame % 6]; break;
            case ShieldCopAction::TURN: targetImg = &m_ImgTurn_R[m_CurrentFrame % 8]; break;
            case ShieldCopAction::RUN: targetImg = &m_ImgRun_R[m_CurrentFrame % 10]; break;
            }
        }
    }

    if (targetImg && !targetImg->IsNull())
    {
        int finalW = (int)(targetImg->GetWidth() * enemyScale * motionScaleX * mapScale);
        int finalH = (int)(targetImg->GetHeight() * enemyScale * motionScaleY * mapScale);
        int finalY = screenY + (int)(m_colH * mapScale) - finalH;
        int drawX = screenX + (int)(m_colW * mapScale / 2) - (finalW / 2);

        if (m_isFacingLeft)
        {
            int oldMode = SetGraphicsMode(hdc, GM_ADVANCED);
            XFORM xFormOld;
            GetWorldTransform(hdc, &xFormOld);
            XFORM xFormLeft = { -1.0f, 0.0f, 0.0f, 1.0f, (float)(2 * drawX + finalW), 0.0f };
            SetWorldTransform(hdc, &xFormLeft);
            targetImg->Draw(hdc, drawX, finalY, finalW, finalH);
            SetWorldTransform(hdc, &xFormOld);
            SetGraphicsMode(hdc, oldMode);
        }
        else
        {
            targetImg->Draw(hdc, drawX, finalY, finalW, finalH);
        }
    }

    RenderDebug(hdc);
}

void ShieldCop::OnTakeDamage(float damage)
{
    m_isAlive = false;
    m_ActionState = ShieldCopAction::HURT_FLY;
    m_vy = -15.0f;
    m_vx = m_isFacingLeft ? 8.0f : -8.0f;
    m_CurrentFrame = 0;
}
