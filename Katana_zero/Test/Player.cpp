#include "Player.h"
#include <objidl.h>
#include <gdiplus.h>

extern int g_playerAttackCooldown;
extern int g_playerAttackDuration;
extern int g_playerAfterImageInterval;
extern float g_timeSlowScale;
extern int g_slowMoDurationLimit;
extern float g_slowMoJumpForceScale;
extern float g_slowMoMoveForceScale;
extern int g_maxJumpHoldTime;

Player::Player() {
    m_x = 100.0f; m_y = 300.0f;
    m_vx = 0.0f; m_vy = 0.0f;
    m_state = PlayerState::IDLE;
    m_isJumping = false;
    m_isFacingRight = true;
    m_currentFrame = 0;
    m_lastTime = GetTickCount();

    m_colW = 40.0f; m_colH = 64.0f;
    m_moveSpeedWalk = 12.0f;
    m_moveSpeedRoll = 15.0f;
    m_accelRate = 0.6f;
    m_frictionRate = 0.3f;

    m_dashRadius = 150.0f;
    m_dashSpeed = 15.0f;
    m_attackCooldown = (DWORD)g_playerAttackCooldown;
    m_lastAttackTime = 0;

    m_wallHangTime = 150;
    m_wallSlideSpeed = 2.5f;
    m_wallSlideFastSpeed = 12.0f;
    m_wallJumpPowerY = -11.0f;
    m_wallJumpPowerX = 14.0f;

    m_speedIdleToWalk = 1.0f;
    m_aniDelayIdleToWalk = 60;
    m_aniDelayWalkToIdle = 60;

    m_aniDelayIdle = 150;
    m_aniDelayWalk = 80;
    m_aniDelayRun = 80;
    m_aniDelayJumpFall = 100;
    m_aniDelayCrouch = 80;
    m_aniDelayRoll = 50;
    m_aniDelayAttack = 40;
    m_aniDelaySlash = 40;
    m_aniDelayWallGrab = 80;
    m_aniDelayWallSlide = 100;
    m_aniDelayWallFlip = 40;

    m_canRoll = true;
    m_canJump = true;
    m_canAirYDash = true;

    m_jumpHoldTimer = 0;
    m_maxJumpHoldTime = 200; // 최대 0.2초 동안만 점프 키 유지 효과 적용

    m_wallGrabTime = 0;
    m_wallDir = 0;

    m_attackTargetX = 0.0f; m_attackTargetY = 0.0f;
    m_attackDirX = 0.0f; m_attackDirY = 0.0f;
    m_dashDirX = 0.0f; m_dashDirY = 0.0f;
    m_attackAngle = 0.0f;

    m_attackHitW = 80.0f;
    m_attackHitH = 60.0f;
    m_attackHitOffset = 40.0f;

    for (int i = 0; i < 2; i++) {
        m_afterImages[i].active = false;
    }
    m_lastAfterImageTime = GetTickCount();

    m_isSlowMo = false;
    m_slowMoStartTime = 0;
}

Player::~Player() {}

void Player::Init() {
    TCHAR path[256];
    for (int i = 0; i < 11; i++) { wsprintf(path, TEXT("assets/idle/%d.png"), i); imgIdle[i].Load(path); }
    for (int i = 0; i < 10; i++) { wsprintf(path, TEXT("assets/walk/%d.png"), i); imgWalk[i].Load(path); }
    for (int i = 0; i < 10; i++) { wsprintf(path, TEXT("assets/run/%d.png"), i); imgRun[i].Load(path); }
    for (int i = 0; i < 4; i++) { wsprintf(path, TEXT("assets/jump/%d.png"), i); imgJumpUp[i].Load(path); }
    for (int i = 0; i < 4; i++) { wsprintf(path, TEXT("assets/fall/%d.png"), i); imgFall[i].Load(path); }
    for (int i = 0; i < 2; i++) { wsprintf(path, TEXT("assets/prevdown/%d.png"), i); imgPrevDown[i].Load(path); }
    for (int i = 0; i < 1; i++) { wsprintf(path, TEXT("assets/down/%d.png"), i); imgDown[i].Load(path); }
    for (int i = 0; i < 2; i++) { wsprintf(path, TEXT("assets/postdown/%d.png"), i); imgPostDown[i].Load(path); }
    for (int i = 0; i < 6; i++) { wsprintf(path, TEXT("assets/roll/%d.png"), i); imgRoll[i].Load(path); }
    for (int i = 0; i < 2; i++) { wsprintf(path, TEXT("assets/wallgrab/%d.png"), i); imgWallGrab[i].Load(path); }
    for (int i = 0; i < 1; i++) { wsprintf(path, TEXT("assets/wallslide/%d.png"), i); imgWallSlide[i].Load(path); }
    for (int i = 0; i < 11; i++) { wsprintf(path, TEXT("assets/wallflip/%d.png"), i); imgWallFlip[i].Load(path); }

    for (int i = 0; i < 4; i++) { wsprintf(path, TEXT("assets/idletowalk/%d.png"), i); imgIdleToWalk[i].Load(path); }
    for (int i = 0; i < 5; i++) { wsprintf(path, TEXT("assets/walktoidle/%d.png"), i); imgWalkToIdle[i].Load(path); }

    for (int i = 0; i < 7; i++) { wsprintf(path, TEXT("assets/attack/%d.png"), i); imgAttack[i].Load(path); }
    for (int i = 0; i < 5; i++) { wsprintf(path, TEXT("assets/slash/%d.png"), i); imgSlashFX[i].Load(path); }
}

void Player::Update(int mouseX, int mouseY, float camX, float camY, float g_renderMapScale, float g_mapOffsetX, float g_mapOffsetY, bool g_isFullMapView) {
    DWORD currentTime = GetTickCount();

    bool isW = GetAsyncKeyState('W') & 0x8000;
    bool isA = GetAsyncKeyState('A') & 0x8000;
    bool isS = GetAsyncKeyState('S') & 0x8000;
    bool isD = GetAsyncKeyState('D') & 0x8000;
    bool isSpace = GetAsyncKeyState(VK_SPACE) & 0x8000;

    int maxStepHeight = 15;

    int touchWallDir = 0;
    if (CheckSpecificCollision(m_x - 3.0f, m_y, m_colW, m_colH, 3)) touchWallDir = -1;
    else if (CheckSpecificCollision(m_x + 3.0f, m_y, m_colW, m_colH, 3)) touchWallDir = 1;

    // 슬로우 모션 (Shift 키) 처리: 누르고 있는 동안 활성화, 5초 제한 시 키를 떼야 재사용 가능
    bool isShiftPressed = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
    
    if (isShiftPressed) {
        if (m_canSlowMo && !m_isSlowMo) {
            // 처음 누르기 시작했을 때
            m_isSlowMo = true;
            m_slowMoStartTime = currentTime;
        }
        else if (m_isSlowMo) {
            // 계속 누르고 있는 중, 5초 제한 체크
            if (currentTime - m_slowMoStartTime >= (DWORD)g_slowMoDurationLimit) {
                m_isSlowMo = false; 
                m_canSlowMo = false; // 5초 경과 시 키를 떼기 전까지 재발동 금지
            }
        }
    }
    else {
        // 키를 떼면 해제 및 재사용 가능 상태로 변경
        m_isSlowMo = false;
        m_canSlowMo = true;
    }

    // 시간 배율 결정
    float timeScale = m_isSlowMo ? g_timeSlowScale : 1.0f;

    // 슬로우 모션 시 시간 배율에 따른 물리 법칙 보정 (v' = v*dt, g' = g*dt^2 => 동일 궤적 유지)
    float dt = timeScale;
    float dtSq = dt * dt;

    // --- [속도 변수들에 시간 배율 적용] ---
    float currentAccel = m_accelRate * dtSq;     // 가속도: dt^2 비례
    float currentFriction = m_frictionRate * dtSq; // 마찰력: dt^2 비례
    float currentDashSpeed = m_dashSpeed * dt;    // 대시 속도: dt 비례
    float currentRollSpeed = m_moveSpeedRoll * dt; // 구르기 속도: dt 비례
    float currentWalkSpeed = m_moveSpeedWalk * dt; // 걷기 속도: dt 비례
    float currentGravityHold = m_gravityHold * dtSq;   // 중력: dt^2 비례
    float currentGravityNormal = m_gravityNormal * dtSq; // 중력: dt^2 비례

    // 사용자가 조정 가능한 튜닝 변수 적용 (기본 1.0)
    float jumpScale = dt * g_slowMoJumpForceScale;
    float wallJumpScale = dt * g_slowMoJumpForceScale; // 벽 점프도 일반 점프 배율 공유

    float currentJumpPower = m_jumpPower * jumpScale;
    float currentWallJumpPowerY = m_wallJumpPowerY * wallJumpScale;
    float currentWallJumpPowerX = m_wallJumpPowerX * (m_isSlowMo ? (dt * g_slowMoMoveForceScale) : 1.0f);

    bool inAir = m_isJumping || (m_vy != 0.0f);
    bool isJumpKeyPressed = isW || isSpace;
    if (!isJumpKeyPressed) m_canJump = true;

    // 0. 점프 & 플립 판정
    if (isJumpKeyPressed && m_canJump) {
        if (m_state == PlayerState::WALL_GRAB || m_state == PlayerState::WALL_SLIDE) {
            m_state = PlayerState::WALL_FLIP;
            m_currentFrame = 0;
            m_vy = currentWallJumpPowerY;
            m_vx = (m_wallDir == 1) ? -currentWallJumpPowerX : currentWallJumpPowerX;
            m_isFacingRight = (m_wallDir == -1);
            m_isJumping = true;
            m_canJump = false;

            m_x += (m_wallDir == 1) ? -2.0f : 2.0f;
            touchWallDir = 0;
            m_jumpHoldTimer = currentTime; // 점프 유지 타이머 시작
        }
        else if (!inAir && touchWallDir != 0 && ((touchWallDir == -1 && isA) || (touchWallDir == 1 && isD))) {
            m_state = PlayerState::WALL_SLIDE;
            m_currentFrame = 0;
            m_wallDir = touchWallDir;
            m_isFacingRight = (m_wallDir == 1);
            m_vy = currentJumpPower;
            m_isJumping = true;
            m_canJump = false;
            m_canAirYDash = true;
            m_jumpHoldTimer = currentTime; // 점프 유지 타이머 시작
        }
        else if (!inAir && m_state != PlayerState::ROLL && m_state != PlayerState::ATTACK && m_state != PlayerState::WALL_FLIP) {
            m_vy = currentJumpPower;
            m_isJumping = true;
            m_canJump = false;
            m_jumpHoldTimer = currentTime; // 점프 유지 타이머 시작
        }
    }

    // 1. 마우스 조준 대시 공격
    bool currentLButton = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    static bool prevLButton = false;
    float worldMouseX = (mouseX - g_mapOffsetX) / g_renderMapScale;
    float worldMouseY = (mouseY - g_mapOffsetY) / g_renderMapScale;
    if (!g_isFullMapView) { worldMouseX += camX; worldMouseY += camY; }

    if (currentLButton && !prevLButton && m_state != PlayerState::ATTACK && m_state != PlayerState::PREVDOWN && m_state != PlayerState::DOWN) {
        if (currentTime - m_lastAttackTime >= m_attackCooldown) {
            m_state = PlayerState::ATTACK;
            m_currentFrame = 0;
            m_lastAttackTime = currentTime;

            float dx = worldMouseX - (m_x + m_colW / 2.0f);
            float dy = worldMouseY - (m_y + m_colH / 2.0f);
            float dist = sqrt(dx * dx + dy * dy);

            m_attackAngle = atan2(dy, dx);
            m_isFacingRight = (dx >= 0);

            if (dist > 0) {
                m_attackDirX = dx / dist;
                m_attackDirY = dy / dist;
            }
            else {
                m_attackDirX = 1.0f; m_attackDirY = 0.0f;
            }

            m_dashDirX = m_attackDirX;
            m_dashDirY = m_attackDirY;

            if (dy < 0) {
                if (m_canAirYDash) {
                    m_canAirYDash = false;
                }
                else {
                    m_dashDirY = 0.0f;
                    m_dashDirX = (dx >= 0) ? 1.0f : -1.0f;
                }
            }

            float dashDist = (std::min)(dist, m_dashRadius);
            m_attackTargetX = m_x + m_dashDirX * dashDist;
            m_attackTargetY = m_y + m_dashDirY * dashDist;
        }
    }
    prevLButton = currentLButton;

    // 2. 공중 벽타기(Grab/Slide) 진입 및 탈출
    if (inAir && touchWallDir != 0 && m_state != PlayerState::ATTACK && m_state != PlayerState::ROLL) {
        if (m_state != PlayerState::WALL_GRAB && m_state != PlayerState::WALL_SLIDE) {
            bool isPressingWall = ((touchWallDir == -1 && isA) || (touchWallDir == 1 && isD));
            bool isFlippingToNewWall = (m_state == PlayerState::WALL_FLIP && touchWallDir != m_wallDir);

            if (isFlippingToNewWall || (m_state != PlayerState::WALL_FLIP && isPressingWall)) {
                m_state = PlayerState::WALL_GRAB;
                m_currentFrame = 0;
                m_wallGrabTime = currentTime;
                m_wallDir = touchWallDir;
                m_isFacingRight = (m_wallDir == 1);
                m_canAirYDash = true;
            }
        }
        else {
            if ((m_wallDir == 1 && isA) || (m_wallDir == -1 && isD)) {
                m_state = PlayerState::FALL;
                m_vx = (m_wallDir == 1) ? -m_moveSpeedWalk : m_moveSpeedWalk;
            }
            else if (touchWallDir != m_wallDir) {
                m_state = PlayerState::FALL;
            }
        }
    }
    else if (m_state == PlayerState::WALL_GRAB || m_state == PlayerState::WALL_SLIDE) {
        if (!inAir) m_state = PlayerState::IDLE;
        else m_state = PlayerState::FALL;
    }

    // 3. 상태별 X, Y축 이동
    float targetVx = 0.0f;
    float currentSpeedLimit = m_moveSpeedWalk;
    bool isWalkAfterRoll = (isS && (isA || isD) && !m_canRoll);

    if (m_state == PlayerState::ATTACK) {
        float distToTarget = sqrt(pow(m_attackTargetX - m_x, 2) + pow(m_attackTargetY - m_y, 2));
        if (distToTarget > currentDashSpeed) {
            float nextX = m_x + m_dashDirX * currentDashSpeed;
            float nextY = m_y + m_dashDirY * currentDashSpeed;
            if (!CheckMapCollision(nextX, m_y, m_colW, m_colH)) m_x += m_dashDirX * currentDashSpeed;
            if (!CheckMapCollision(m_x, nextY, m_colW, m_colH)) m_y += m_dashDirY * currentDashSpeed;
        }
        else { m_x = m_attackTargetX; m_y = m_attackTargetY; }
        m_vy = 0.0f; m_vx = 0.0f;
    }
    else if (m_state == PlayerState::ROLL) {
        m_vx = m_isFacingRight ? currentRollSpeed : -currentRollSpeed;
    }
    else if (m_state == PlayerState::WALL_GRAB || m_state == PlayerState::WALL_SLIDE) {
        m_vx = 0.0f;
    }
    else if (m_state == PlayerState::WALL_FLIP) {
        // 유저 입력 무시하고 날아가기
    }
    else if (m_state == PlayerState::IDLE_TO_WALK) {
        if (isA) { targetVx = -m_speedIdleToWalk * timeScale; m_isFacingRight = false; }
        if (isD) { targetVx = m_speedIdleToWalk * timeScale; m_isFacingRight = true; }
    }
    else if (m_state == PlayerState::WALK_TO_IDLE) {
        targetVx = 0.0f;
    }
    else if ((m_state == PlayerState::PREVDOWN || m_state == PlayerState::DOWN || m_state == PlayerState::POSTDOWN) && !isWalkAfterRoll) {}
    else {
        if (isA) { targetVx = -currentWalkSpeed; m_isFacingRight = false; }
        if (isD) { targetVx = currentWalkSpeed; m_isFacingRight = true; }
    }

    if (targetVx != 0.0f && m_state != PlayerState::ROLL && m_state != PlayerState::ATTACK && m_state != PlayerState::WALL_GRAB && m_state != PlayerState::WALL_SLIDE && m_state != PlayerState::WALL_FLIP) {
        m_vx += (targetVx - m_vx) * currentAccel;
    }
    else if (m_state != PlayerState::ROLL && m_state != PlayerState::ATTACK && m_state != PlayerState::WALL_GRAB && m_state != PlayerState::WALL_SLIDE && m_state != PlayerState::WALL_FLIP) {
        m_vx += (0.0f - m_vx) * currentFriction;
        if (fabs(m_vx) < 0.1f) m_vx = 0.0f;
    }

    if (m_vx != 0.0f && m_state != PlayerState::ATTACK && m_state != PlayerState::WALL_GRAB && m_state != PlayerState::WALL_SLIDE) {
        float nextX = m_x + m_vx;
        if (!CheckMapCollision(nextX, m_y, m_colW, m_colH - 5)) {
            m_x = nextX;
        }
        else {
            bool steppedUp = false;
            for (int step = 1; step <= maxStepHeight; step++) {
                if (!CheckMapCollision(nextX, m_y - step, m_colW, m_colH - 5)) {
                    m_x = nextX; m_y -= step; steppedUp = true; break;
                }
            }
            if (!steppedUp) {
                float sign = (m_vx > 0) ? 1.0f : -1.0f;
                int failsafe = 0;
                while (!CheckMapCollision(m_x + sign, m_y, m_colW, m_colH - 5) && failsafe++ < (int)fabs(m_vx) + 2) {
                    m_x += sign;
                }
                if (m_state != PlayerState::WALL_FLIP) m_vx = 0.0f;
            }
        }
    }

    if (isS && !m_isJumping && m_state != PlayerState::ROLL && m_state != PlayerState::ATTACK && m_state != PlayerState::WALL_GRAB && m_state != PlayerState::WALL_SLIDE && m_state != PlayerState::WALL_FLIP) {
        int fTypeL = GetCollisionType((int)m_x, (int)(m_y + m_colH + 1));
        int fTypeC = GetCollisionType((int)(m_x + m_colW / 2), (int)(m_y + m_colH + 1));
        int fTypeR = GetCollisionType((int)(m_x + m_colW), (int)(m_y + m_colH + 1));
        if (fTypeL == 2 || fTypeC == 2 || fTypeR == 2) {
            m_y += 4.0f; m_isJumping = true; m_vy = 1.0f;
        }
    }

    // Y축 이동
    if (m_state != PlayerState::ATTACK) {
        if (m_state == PlayerState::WALL_GRAB) {
            m_vy = 0.0f;
            if (currentTime - m_wallGrabTime >= m_wallHangTime) {
                m_state = PlayerState::WALL_SLIDE; m_currentFrame = 0;
            }
        }
        else {
            // 점프 키 유지 시간 체크 (슬로우 모션 배율 고려)
            bool isJumpHoldValid = isJumpKeyPressed && (currentTime - m_jumpHoldTimer < (DWORD)(g_maxJumpHoldTime));
            
            float currentGravity = (isJumpHoldValid && m_vy < 0.0f) ? currentGravityHold : currentGravityNormal;
            m_vy += currentGravity;

            float maxFall = m_maxFallSpeed * timeScale;
            if (m_state == PlayerState::WALL_SLIDE && m_vy >= 0.0f) {
                maxFall = isS ? m_wallSlideFastSpeed * timeScale : m_wallSlideSpeed * timeScale;
                m_vy = maxFall;
            }
            else if (m_vy > maxFall) {
                m_vy = maxFall;
            }
        }

        float nextY = m_y + m_vy;

        if (m_vy > 0) { // 하강
            bool hitFloor = false;
            float finalFloorY = nextY;

            if (CheckMapCollision(m_x, nextY, m_colW, m_colH)) hitFloor = true;
            else {
                for (float checkY = m_y; checkY <= nextY; checkY += 1.0f) {
                    int typeL = GetCollisionType((int)m_x, (int)(checkY + m_colH));
                    int typeC = GetCollisionType((int)(m_x + m_colW / 2), (int)(checkY + m_colH));
                    int typeR = GetCollisionType((int)(m_x + m_colW), (int)(checkY + m_colH));

                    if (typeL == 2 || typeC == 2 || typeR == 2) {
                        if (m_y + m_colH <= checkY + m_colH + 2) { hitFloor = true; finalFloorY = checkY; break; }
                    }
                }
            }

            if (hitFloor) {
                m_isJumping = false; m_vy = 0; m_y = finalFloorY; m_canAirYDash = true;
                int failsafe = 0;
                while ((CheckMapCollision(m_x, m_y, m_colW, m_colH) ||
                    GetCollisionType((int)m_x, (int)(m_y + m_colH)) == 2 ||
                    GetCollisionType((int)(m_x + m_colW / 2), (int)(m_y + m_colH)) == 2 ||
                    GetCollisionType((int)(m_x + m_colW), (int)(m_y + m_colH)) == 2) && failsafe++ < 100) {
                    m_y -= 1.0f;
                }
                m_y += 1.0f;
            }
            else {
                m_y = nextY;
                int nL = GetCollisionType((int)m_x, (int)(m_y + m_colH + 1));
                int nC = GetCollisionType((int)(m_x + m_colW / 2), (int)(m_y + m_colH + 1));
                int nR = GetCollisionType((int)(m_x + m_colW), (int)(m_y + m_colH + 1));
                if (!CheckMapCollision(m_x, m_y + 1.0f, m_colW, m_colH) && nL != 2 && nC != 2 && nR != 2) m_isJumping = true;
                else { m_isJumping = false; m_canAirYDash = true; }
            }
        }
        else if (m_vy < 0) { // 상승
            if (CheckMapCollision(m_x, nextY, m_colW, m_colH)) {
                m_vy = 0; m_y = nextY;
                int failsafe = 0;
                while (CheckMapCollision(m_x, m_y, m_colW, m_colH) && failsafe++ < 100) m_y += 1.0f;
            }
            else m_y = nextY;
        }

        int mapLimit = imgMap.IsNull() ? VIRTUAL_HEIGHT : imgMap.GetHeight();
        if (m_y + m_colH > mapLimit - 20) { m_y = mapLimit - m_colH - 20; m_isJumping = false; m_vy = 0; }
    }

    // 4. 애니메이션 상태 머신
    PlayerState newState = m_state;

    if (m_state == PlayerState::ATTACK && m_currentFrame >= 5) {
        newState = m_isJumping ? PlayerState::FALL : PlayerState::IDLE;
    }
    else if (m_state == PlayerState::WALL_FLIP && m_currentFrame >= 10) {
        newState = m_isJumping ? (m_vy < 0.0f ? PlayerState::JUMP_UP : PlayerState::FALL) : PlayerState::IDLE;
    }
    else if (m_state == PlayerState::ROLL && m_currentFrame >= 6) {
        if (isA || isD) newState = PlayerState::WALK;
        else newState = isS ? PlayerState::DOWN : PlayerState::IDLE;
    }
    else if (m_state == PlayerState::PREVDOWN && m_currentFrame >= 2) newState = PlayerState::DOWN;
    else if (m_state == PlayerState::POSTDOWN && m_currentFrame >= 2) newState = PlayerState::IDLE;

    if (!isS) {
        m_canRoll = true;
        if (newState == PlayerState::PREVDOWN || newState == PlayerState::DOWN) newState = PlayerState::POSTDOWN;
    }
    else {
        if (m_canRoll && (isA || isD) && !m_isJumping && newState != PlayerState::ROLL && newState != PlayerState::ATTACK && newState != PlayerState::WALL_GRAB && newState != PlayerState::WALL_SLIDE && newState != PlayerState::WALL_FLIP) {
            newState = PlayerState::ROLL; m_canRoll = false; m_isFacingRight = isD;
        }
        else if (!m_canRoll && (isA || isD) && !m_isJumping && newState != PlayerState::ROLL && newState != PlayerState::ATTACK && newState != PlayerState::WALL_GRAB && newState != PlayerState::WALL_SLIDE && newState != PlayerState::WALL_FLIP) {
            newState = PlayerState::WALK;
        }
        else if (!(isA || isD) && !m_isJumping && newState != PlayerState::ROLL && newState != PlayerState::ATTACK && newState != PlayerState::PREVDOWN && newState != PlayerState::DOWN && newState != PlayerState::POSTDOWN && newState != PlayerState::WALL_GRAB && newState != PlayerState::WALL_SLIDE && newState != PlayerState::WALL_FLIP) {
            newState = PlayerState::PREVDOWN;
        }
        else if (!(isA || isD) && newState == PlayerState::POSTDOWN) {
            newState = PlayerState::PREVDOWN;
        }
    }

    if (newState != PlayerState::ROLL && newState != PlayerState::ATTACK && newState != PlayerState::PREVDOWN && newState != PlayerState::DOWN && newState != PlayerState::POSTDOWN && newState != PlayerState::WALL_GRAB && newState != PlayerState::WALL_SLIDE && newState != PlayerState::WALL_FLIP) {
        if (m_isJumping) {
            newState = (m_vy < 0.0f) ? PlayerState::JUMP_UP : PlayerState::FALL;
        }
        else {
            if (isA || isD) {
                if (m_state == PlayerState::IDLE_TO_WALK) {
                    if (m_currentFrame >= 3) newState = PlayerState::WALK;
                    else newState = PlayerState::IDLE_TO_WALK;
                }
                else if (m_state == PlayerState::WALK || m_state == PlayerState::RUN) {
                    newState = PlayerState::WALK;
                }
                else {
                    newState = PlayerState::IDLE_TO_WALK;
                }
            }
            else {
                if (m_state == PlayerState::WALK_TO_IDLE) {
                    if (m_currentFrame >= 4) newState = PlayerState::IDLE;
                    else newState = PlayerState::WALK_TO_IDLE;
                }
                else if (m_state == PlayerState::WALK || m_state == PlayerState::RUN || m_state == PlayerState::IDLE_TO_WALK || m_state == PlayerState::FALL) {
                    newState = PlayerState::WALK_TO_IDLE;
                }
                else {
                    newState = PlayerState::IDLE;
                }
            }
        }
    }
    else if (!m_isJumping && (newState == PlayerState::WALL_FLIP || newState == PlayerState::WALL_SLIDE || newState == PlayerState::WALL_GRAB)) {
        if (isA || isD) newState = PlayerState::IDLE_TO_WALK;
        else newState = PlayerState::WALK_TO_IDLE;
    }

    if (m_state != newState) {
        m_currentFrame = 0;
        m_state = newState;
    }

    // 잔상 기록 로직: ATTACK, ROLL, WALL_FLIP 상태이거나 슬로우 모션 중일 때 기록
    if (m_state == PlayerState::ATTACK || m_state == PlayerState::ROLL || m_state == PlayerState::WALL_FLIP || m_isSlowMo) {
        if (currentTime - m_lastAfterImageTime >= (DWORD)g_playerAfterImageInterval) { // 튜닝 변수 적용
            m_afterImages[1] = m_afterImages[0]; // 이전 잔상을 뒤로 밀기
            m_afterImages[0].x = m_x;
            m_afterImages[0].y = m_y;
            m_afterImages[0].state = m_state;
            m_afterImages[0].frame = m_currentFrame;
            m_afterImages[0].isFacingRight = m_isFacingRight;
            m_afterImages[0].attackAngle = m_attackAngle;
            m_afterImages[0].active = true;
            m_lastAfterImageTime = currentTime;
        }
    }
    else {
        // 해당 상태가 아니고 슬로우 모션도 아니면 점진적으로 비활성화
        if (currentTime - m_lastAfterImageTime >= (DWORD)g_playerAfterImageInterval) {
            m_afterImages[1] = m_afterImages[0];
            m_afterImages[0].active = false;
            m_lastAfterImageTime = currentTime;
        }
    }
}

void Player::UpdateAnimation() {
    static DWORD lastTime = GetTickCount();
    DWORD currentTime = GetTickCount();
    DWORD targetDelayMs = m_aniDelayIdle;

    switch (m_state) {
    case PlayerState::IDLE: targetDelayMs = m_aniDelayIdle; break;
    case PlayerState::IDLE_TO_WALK: targetDelayMs = m_aniDelayIdleToWalk; break;
    case PlayerState::WALK: targetDelayMs = m_aniDelayWalk; break;
    case PlayerState::WALK_TO_IDLE: targetDelayMs = m_aniDelayWalkToIdle; break;
    case PlayerState::RUN:  targetDelayMs = m_aniDelayRun;  break;
    case PlayerState::JUMP_UP:
    case PlayerState::FALL: targetDelayMs = m_aniDelayJumpFall; break;
    case PlayerState::PREVDOWN:
    case PlayerState::DOWN:
    case PlayerState::POSTDOWN: targetDelayMs = m_aniDelayCrouch; break;
    case PlayerState::ROLL: targetDelayMs = m_aniDelayRoll; break;
    case PlayerState::ATTACK: targetDelayMs = m_aniDelayAttack; break;
    case PlayerState::WALL_GRAB: targetDelayMs = m_aniDelayWallGrab; break;
    case PlayerState::WALL_SLIDE: targetDelayMs = m_aniDelayWallSlide; break;
    case PlayerState::WALL_FLIP: targetDelayMs = m_aniDelayWallFlip; break;
    }

    // 슬로우 모션 시 애니메이션 지연 시간을 늘림 (속도를 늦춤)
    if (m_isSlowMo) {
        targetDelayMs = (DWORD)(targetDelayMs / g_timeSlowScale);
    }

    if (currentTime - lastTime >= targetDelayMs) {
        m_currentFrame++;
        lastTime = currentTime;

        if (m_state == PlayerState::IDLE && m_currentFrame >= 11) m_currentFrame = 0;
        if (m_state == PlayerState::IDLE_TO_WALK && m_currentFrame >= 4) m_currentFrame = 3;
        if (m_state == PlayerState::WALK && m_currentFrame >= 10) m_currentFrame = 0;
        if (m_state == PlayerState::WALK_TO_IDLE && m_currentFrame >= 5) m_currentFrame = 4;
        if (m_state == PlayerState::RUN && m_currentFrame >= 10) m_currentFrame = 0;
        if (m_state == PlayerState::JUMP_UP && m_currentFrame >= 4) m_currentFrame = 3;
        if (m_state == PlayerState::FALL && m_currentFrame >= 4) m_currentFrame = 3;
        if (m_state == PlayerState::DOWN && m_currentFrame >= 1) m_currentFrame = 0;
        if (m_state == PlayerState::ATTACK && m_currentFrame >= 5) m_currentFrame = 5;

        if (m_state == PlayerState::WALL_GRAB && m_currentFrame >= 2) m_currentFrame = 1;
        if (m_state == PlayerState::WALL_SLIDE && m_currentFrame >= 1) m_currentFrame = 0;
        if (m_state == PlayerState::WALL_FLIP && m_currentFrame >= 11) m_currentFrame = 10;
    }
}

void Player::Render(HDC hMemDC, float camX, float camY, float mapScale, float playerScale, float g_renderMapScale, float g_mapOffsetX, float g_mapOffsetY, bool g_isFullMapView, bool g_showDebugRect) {
    int mapW = imgMap.IsNull() ? VIRTUAL_WIDTH : imgMap.GetWidth();
    int mapH = imgMap.IsNull() ? VIRTUAL_HEIGHT : imgMap.GetHeight();
    float pFitScale = mapScale;

    Gdiplus::Graphics graphics(hMemDC);

    // --- 잔상 및 본체 렌더링 통합 관리 ---
    // i = 1 (오래된 잔상: 핑크), i = 0 (최신 잔상: 민트), i = -1 (플레이어 본체)
    for (int i = 1; i >= -1; i--) {
        CImage* img = NULL;
        float curX, curY;
        PlayerState curState;
        int curFrame;
        bool curFacing;
        float curAngle;
        bool isActive = false;

        if (i >= 0) { // 잔상
            if (!m_afterImages[i].active) continue;
            isActive = true;
            curX = m_afterImages[i].x;
            curY = m_afterImages[i].y;
            curState = m_afterImages[i].state;
            curFrame = m_afterImages[i].frame;
            curFacing = m_afterImages[i].isFacingRight;
            curAngle = m_afterImages[i].attackAngle;

            switch (curState) {
            case PlayerState::IDLE:         img = &imgIdle[curFrame]; break;
            case PlayerState::IDLE_TO_WALK: img = &imgIdleToWalk[(std::min)(curFrame, 3)]; break;
            case PlayerState::WALK:         img = &imgWalk[curFrame]; break;
            case PlayerState::WALK_TO_IDLE: img = &imgWalkToIdle[(std::min)(curFrame, 4)]; break;
            case PlayerState::RUN:          img = &imgRun[curFrame]; break;
            case PlayerState::JUMP_UP:      img = &imgJumpUp[(std::min)(curFrame, 3)]; break;
            case PlayerState::FALL:         img = &imgFall[(std::min)(curFrame, 3)]; break;
            case PlayerState::PREVDOWN:     img = &imgPrevDown[(std::min)(curFrame, 1)]; break;
            case PlayerState::DOWN:         img = &imgDown[0]; break;
            case PlayerState::POSTDOWN:     img = &imgPostDown[(std::min)(curFrame, 1)]; break;
            case PlayerState::ROLL:         img = &imgRoll[(std::min)(curFrame, 5)]; break;
            case PlayerState::ATTACK:       img = &imgAttack[(std::min)(curFrame, 6)]; break;
            case PlayerState::WALL_GRAB:    img = &imgWallGrab[(std::min)(curFrame, 1)]; break;
            case PlayerState::WALL_SLIDE:   img = &imgWallSlide[0]; break;
            case PlayerState::WALL_FLIP:    img = &imgWallFlip[(std::min)(curFrame, 10)]; break;
            }
        }
        else { // 플레이어 본체 (i == -1)
            isActive = true;
            curX = m_x;
            curY = m_y;
            curState = m_state;
            curFrame = m_currentFrame;
            curFacing = m_isFacingRight;
            curAngle = m_attackAngle;

            switch (curState) {
            case PlayerState::IDLE:         img = &imgIdle[curFrame]; break;
            case PlayerState::IDLE_TO_WALK: img = &imgIdleToWalk[(std::min)(curFrame, 3)]; break;
            case PlayerState::WALK:         img = &imgWalk[curFrame]; break;
            case PlayerState::WALK_TO_IDLE: img = &imgWalkToIdle[(std::min)(curFrame, 4)]; break;
            case PlayerState::RUN:          img = &imgRun[curFrame]; break;
            case PlayerState::JUMP_UP:      img = &imgJumpUp[(std::min)(curFrame, 3)]; break;
            case PlayerState::FALL:         img = &imgFall[(std::min)(curFrame, 3)]; break;
            case PlayerState::PREVDOWN:     img = &imgPrevDown[(std::min)(curFrame, 1)]; break;
            case PlayerState::DOWN:         img = &imgDown[0]; break;
            case PlayerState::POSTDOWN:     img = &imgPostDown[(std::min)(curFrame, 1)]; break;
            case PlayerState::ROLL:         img = &imgRoll[(std::min)(curFrame, 5)]; break;
            case PlayerState::ATTACK:       img = &imgAttack[(std::min)(curFrame, 6)]; break;
            case PlayerState::WALL_GRAB:    img = &imgWallGrab[(std::min)(curFrame, 1)]; break;
            case PlayerState::WALL_SLIDE:   img = &imgWallSlide[0]; break;
            case PlayerState::WALL_FLIP:    img = &imgWallFlip[(std::min)(curFrame, 10)]; break;
            }
        }

        if (img && !img->IsNull()) {
            float avPX, avPY;
            if (g_isFullMapView) {
                float fitScale = (std::min)((float)VIRTUAL_WIDTH / mapW, (float)VIRTUAL_HEIGHT / mapH);
                float fitX = (VIRTUAL_WIDTH - mapW * fitScale) / 2.0f;
                float fitY = (VIRTUAL_HEIGHT - mapH * fitScale) / 2.0f;
                avPX = curX * fitScale + fitX;
                avPY = curY * fitScale + fitY;
                pFitScale = fitScale;
            }
            else {
                avPX = (curX - camX) * mapScale;
                avPY = (curY - camY) * mapScale;
                pFitScale = mapScale;
            }

            float asPW = img->GetWidth() * playerScale * pFitScale;
            float asPH = img->GetHeight() * playerScale * pFitScale;
            float adrawX = avPX + (m_colW * pFitScale) / 2.0f - (asPW / 2.0f);
            float adrawY = avPY + (m_colH * pFitScale) - asPH;

            Gdiplus::Bitmap gdiImg(img->GetWidth(), img->GetHeight(), img->GetPitch(), PixelFormat32bppARGB, (BYTE*)img->GetBits());
            Gdiplus::ImageAttributes attr;
            Gdiplus::ColorMatrix matrix;

            bool isMintTint = (m_isSlowMo || (i == 0)); // 슬로우 모션 중이거나 첫 번째 잔상

            if (isMintTint) { // 민트 틴트
                float alpha = (i == -1) ? 1.0f : 0.6f; // 본체는 불투명, 잔상은 반투명
                matrix = {
                    0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                    0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                    0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                    0.0f, 1.0f, 1.0f, alpha, 0.0f,
                    0.0f, 0.0f, 0.0f, 0.0f, 1.0f
                };
            }
            else if (i == 1) { // 핑크 틴트 (두 번째 잔상)
                matrix = {
                    0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                    0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                    0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                    1.0f, 0.0f, 1.0f, 0.4f, 0.0f,
                    0.0f, 0.0f, 0.0f, 0.0f, 1.0f
                };
            }
            else { // 일반 렌더링 (슬로우 모션 아닐 때 본체)
                matrix = {
                    1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                    0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
                    0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                    0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                    0.0f, 0.0f, 0.0f, 0.0f, 1.0f
                };
            }
            attr.SetColorMatrix(&matrix, Gdiplus::ColorMatrixFlagsDefault, Gdiplus::ColorAdjustTypeBitmap);

            if (curFacing) {
                graphics.DrawImage(&gdiImg, Gdiplus::RectF(adrawX, adrawY, asPW, asPH), 0, 0, (float)img->GetWidth(), (float)img->GetHeight(), Gdiplus::UnitPixel, &attr);
            }
            else {
                graphics.ScaleTransform(-1.0f, 1.0f);
                graphics.TranslateTransform(-(adrawX * 2 + asPW), 0);
                graphics.DrawImage(&gdiImg, Gdiplus::RectF(adrawX, adrawY, asPW, asPH), 0, 0, (float)img->GetWidth(), (float)img->GetHeight(), Gdiplus::UnitPixel, &attr);
                graphics.ResetTransform();
            }

            // 공격 시 슬래시 이펙트 (본체 그릴 때만)
            if (i == -1 && curState == PlayerState::ATTACK && curFrame < 5) {
                CImage* slashImg = &imgSlashFX[curFrame];
                if (!slashImg->IsNull()) {
                    int sW = (int)(slashImg->GetWidth() * playerScale * pFitScale);
                    int sH = (int)(slashImg->GetHeight() * playerScale * pFitScale);
                    Gdiplus::Bitmap gdiSlash(slashImg->GetWidth(), slashImg->GetHeight(), slashImg->GetPitch(), PixelFormat32bppARGB, (BYTE*)slashImg->GetBits());
                    
                    graphics.TranslateTransform(avPX + (m_colW * pFitScale) / 2.0f, avPY + (m_colH * pFitScale) / 2.0f);
                    graphics.RotateTransform(curAngle * 180.0f / 3.14159f);
                    graphics.DrawImage(&gdiSlash, Gdiplus::RectF(-sW / 2.0f, -sH / 2.0f, (float)sW, (float)sH), 0, 0, (float)slashImg->GetWidth(), (float)slashImg->GetHeight(), Gdiplus::UnitPixel, &attr);
                    graphics.ResetTransform();
                }
            }
        }
    }

    if (g_showDebugRect) {
        float vPX = (m_x - camX) * mapScale;
        float vPY = (m_y - camY) * mapScale;
        if (g_isFullMapView) {
            float fitScale = (std::min)((float)VIRTUAL_WIDTH / mapW, (float)VIRTUAL_HEIGHT / mapH);
            float fitX = (VIRTUAL_WIDTH - mapW * fitScale) / 2.0f;
            float fitY = (VIRTUAL_HEIGHT - mapH * fitScale) / 2.0f;
            vPX = m_x * fitScale + fitX;
            vPY = m_y * fitScale + fitY;
            pFitScale = fitScale;
        }
        HBRUSH greenBrush = CreateSolidBrush(RGB(0, 255, 0));
        RECT pRect = { (int)vPX, (int)vPY, (int)(vPX + m_colW * pFitScale), (int)(vPY + m_colH * pFitScale) };
        FrameRect(hMemDC, &pRect, greenBrush);
        DeleteObject(greenBrush);

        if (m_state == PlayerState::ATTACK) {
            float hitW = m_attackHitW * pFitScale;
            float hitH = m_attackHitH * pFitScale;
            float hitX = vPX + (m_colW * pFitScale) / 2.0f + m_attackDirX * m_attackHitOffset * pFitScale - hitW / 2.0f;
            float hitY = vPY + (m_colH * pFitScale) / 2.0f + m_attackDirY * m_attackHitOffset * pFitScale - hitH / 2.0f;
            HBRUSH redBrush = CreateSolidBrush(RGB(255, 0, 0));
            RECT aRect = { (int)hitX, (int)hitY, (int)(hitX + hitW), (int)(hitY + hitH) };
            FrameRect(hMemDC, &aRect, redBrush);
            DeleteObject(redBrush);
        }
    }
}
