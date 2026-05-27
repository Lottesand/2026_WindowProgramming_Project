#include "Player.h"

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
    m_attackCooldown = 350;
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
    m_aniDelayWalk = 100;
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

    m_wallGrabTime = 0;
    m_wallDir = 0;

    m_attackTargetX = 0.0f; m_attackTargetY = 0.0f;
    m_attackDirX = 0.0f; m_attackDirY = 0.0f;
    m_attackAngle = 0.0f;
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

    bool inAir = m_isJumping || (m_vy != 0.0f);
    bool isJumpKeyPressed = isW || isSpace;
    if (!isJumpKeyPressed) m_canJump = true;

    // 0. 점프 & 플립 판정
    if (isJumpKeyPressed && m_canJump) {
        if (m_state == PlayerState::WALL_GRAB || m_state == PlayerState::WALL_SLIDE) {
            m_state = PlayerState::WALL_FLIP;
            m_currentFrame = 0;
            m_vy = m_wallJumpPowerY;
            m_vx = (m_wallDir == 1) ? -m_wallJumpPowerX : m_wallJumpPowerX;
            m_isFacingRight = (m_wallDir == -1);
            m_isJumping = true;
            m_canJump = false;

            m_x += (m_wallDir == 1) ? -2.0f : 2.0f;
            touchWallDir = 0;
        }
        else if (!inAir && touchWallDir != 0 && ((touchWallDir == -1 && isA) || (touchWallDir == 1 && isD))) {
            m_state = PlayerState::WALL_SLIDE;
            m_currentFrame = 0;
            m_wallDir = touchWallDir;
            m_isFacingRight = (m_wallDir == 1);
            m_vy = m_jumpPower;
            m_isJumping = true;
            m_canJump = false;
            m_canAirYDash = true;
        }
        else if (!inAir && m_state != PlayerState::ROLL && m_state != PlayerState::ATTACK && m_state != PlayerState::WALL_FLIP) {
            m_vy = m_jumpPower;
            m_isJumping = true;
            m_canJump = false;
        }
    }

    // 1. 마우스 조준 대시 공격
    bool currentLButton = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    static bool prevLButton = false;
    float worldMouseX = (mouseX - g_mapOffsetX) / g_renderMapScale;
    float worldMouseY = (mouseY - g_mapOffsetY) / g_renderMapScale;
    if (!g_isFullMapView) { worldMouseX += camX; worldMouseY += camY; }

    if (currentLButton && !prevLButton && m_state != PlayerState::ATTACK && m_state != PlayerState::ROLL && m_state != PlayerState::PREVDOWN && m_state != PlayerState::DOWN) {
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

            if (dy < 0) {
                if (m_canAirYDash) {
                    m_canAirYDash = false;
                }
                else {
                    m_attackDirY = 0.0f;
                    m_attackDirX = (dx >= 0) ? 1.0f : -1.0f;
                }
            }

            float dashDist = (std::min)(dist, m_dashRadius);
            m_attackTargetX = m_x + m_attackDirX * dashDist;
            m_attackTargetY = m_y + m_attackDirY * dashDist;
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
        if (distToTarget > m_dashSpeed) {
            float nextX = m_x + m_attackDirX * m_dashSpeed;
            float nextY = m_y + m_attackDirY * m_dashSpeed;
            if (!CheckMapCollision(nextX, m_y, m_colW, m_colH)) m_x += m_attackDirX * m_dashSpeed;
            if (!CheckMapCollision(m_x, nextY, m_colW, m_colH)) m_y += m_attackDirY * m_dashSpeed;
        }
        else { m_x = m_attackTargetX; m_y = m_attackTargetY; }
        m_vy = 0.0f; m_vx = 0.0f;
    }
    else if (m_state == PlayerState::ROLL) {
        m_vx = m_isFacingRight ? m_moveSpeedRoll : -m_moveSpeedRoll;
    }
    else if (m_state == PlayerState::WALL_GRAB || m_state == PlayerState::WALL_SLIDE) {
        m_vx = 0.0f;
    }
    else if (m_state == PlayerState::WALL_FLIP) {
        // 유저 입력 무시하고 날아가기
    }
    else if (m_state == PlayerState::IDLE_TO_WALK) {
        if (isA) { targetVx = -m_speedIdleToWalk; m_isFacingRight = false; }
        if (isD) { targetVx = m_speedIdleToWalk; m_isFacingRight = true; }
    }
    else if (m_state == PlayerState::WALK_TO_IDLE) {
        targetVx = 0.0f;
    }
    else if ((m_state == PlayerState::PREVDOWN || m_state == PlayerState::DOWN || m_state == PlayerState::POSTDOWN) && !isWalkAfterRoll) {}
    else {
        if (isA) { targetVx = -currentSpeedLimit; m_isFacingRight = false; }
        if (isD) { targetVx = currentSpeedLimit; m_isFacingRight = true; }
    }

    if (targetVx != 0.0f && m_state != PlayerState::ROLL && m_state != PlayerState::ATTACK && m_state != PlayerState::WALL_GRAB && m_state != PlayerState::WALL_SLIDE && m_state != PlayerState::WALL_FLIP) {
        m_vx += (targetVx - m_vx) * m_accelRate;
    }
    else if (m_state != PlayerState::ROLL && m_state != PlayerState::ATTACK && m_state != PlayerState::WALL_GRAB && m_state != PlayerState::WALL_SLIDE && m_state != PlayerState::WALL_FLIP) {
        m_vx += (0.0f - m_vx) * m_frictionRate;
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
            float currentGravity = (isJumpKeyPressed && m_vy < 0.0f) ? m_gravityHold : m_gravityNormal;
            m_vy += currentGravity;

            float maxFall = m_maxFallSpeed;
            if (m_state == PlayerState::WALL_SLIDE && m_vy >= 0.0f) {
                maxFall = isS ? m_wallSlideFastSpeed : m_wallSlideSpeed;
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

    if (m_state == PlayerState::ATTACK && m_currentFrame >= 7) {
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
        if (m_state == PlayerState::ATTACK && m_currentFrame >= 7) m_currentFrame = 7;

        if (m_state == PlayerState::WALL_GRAB && m_currentFrame >= 2) m_currentFrame = 1;
        if (m_state == PlayerState::WALL_SLIDE && m_currentFrame >= 1) m_currentFrame = 0;
        if (m_state == PlayerState::WALL_FLIP && m_currentFrame >= 11) m_currentFrame = 10;
    }
}

void Player::Render(HDC hMemDC, float camX, float camY, float mapScale, float playerScale, float g_renderMapScale, float g_mapOffsetX, float g_mapOffsetY, bool g_isFullMapView, bool g_showDebugRect) {
    CImage* currentImg = NULL;
    switch (m_state) {
    case PlayerState::IDLE:         currentImg = &imgIdle[m_currentFrame]; break;
    case PlayerState::IDLE_TO_WALK: currentImg = &imgIdleToWalk[(std::min)(m_currentFrame, 3)]; break;
    case PlayerState::WALK:         currentImg = &imgWalk[m_currentFrame]; break;
    case PlayerState::WALK_TO_IDLE: currentImg = &imgWalkToIdle[(std::min)(m_currentFrame, 4)]; break;
    case PlayerState::RUN:          currentImg = &imgRun[m_currentFrame]; break;
    case PlayerState::JUMP_UP:      currentImg = &imgJumpUp[(std::min)(m_currentFrame, 3)]; break;
    case PlayerState::FALL:         currentImg = &imgFall[(std::min)(m_currentFrame, 3)]; break;
    case PlayerState::PREVDOWN:     currentImg = &imgPrevDown[(std::min)(m_currentFrame, 1)]; break;
    case PlayerState::DOWN:         currentImg = &imgDown[0]; break;
    case PlayerState::POSTDOWN:     currentImg = &imgPostDown[(std::min)(m_currentFrame, 1)]; break;
    case PlayerState::ROLL:         currentImg = &imgRoll[(std::min)(m_currentFrame, 5)]; break;
    case PlayerState::ATTACK:       currentImg = &imgAttack[(std::min)(m_currentFrame, 6)]; break;
    case PlayerState::WALL_GRAB:    currentImg = &imgWallGrab[(std::min)(m_currentFrame, 1)]; break;
    case PlayerState::WALL_SLIDE:   currentImg = &imgWallSlide[0]; break;
    case PlayerState::WALL_FLIP:    currentImg = &imgWallFlip[(std::min)(m_currentFrame, 10)]; break;
    }

    float vPX = 0.0f, vPY = 0.0f;
    float pFitScale = mapScale;

    int mapW = imgMap.IsNull() ? VIRTUAL_WIDTH : imgMap.GetWidth();
    int mapH = imgMap.IsNull() ? VIRTUAL_HEIGHT : imgMap.GetHeight();

    if (g_isFullMapView) {
        float fitScale = (std::min)((float)VIRTUAL_WIDTH / mapW, (float)VIRTUAL_HEIGHT / mapH);
        float fitX = (VIRTUAL_WIDTH - mapW * fitScale) / 2.0f;
        float fitY = (VIRTUAL_HEIGHT - mapH * fitScale) / 2.0f;
        vPX = m_x * fitScale + fitX;
        vPY = m_y * fitScale + fitY;
        pFitScale = fitScale;
    }
    else {
        vPX = (m_x - camX) * mapScale;
        vPY = (m_y - camY) * mapScale;
    }

    float sPW = 0.0f, sPH = 0.0f, drawX = 0.0f, drawY = 0.0f;

    if (currentImg && !currentImg->IsNull()) {
        sPW = currentImg->GetWidth() * playerScale * pFitScale;
        sPH = currentImg->GetHeight() * playerScale * pFitScale;

        drawX = vPX + (m_colW * pFitScale) / 2.0f - (sPW / 2.0f);
        drawY = vPY + (m_colH * pFitScale) - sPH;

        if (m_isFacingRight) {
            currentImg->Draw(hMemDC, (int)drawX, (int)drawY, (int)sPW, (int)sPH);
        }
        else {
            XFORM xForm = { -1.0f, 0.0f, 0.0f, 1.0f, drawX + sPW, drawY };
            SetWorldTransform(hMemDC, &xForm);
            currentImg->Draw(hMemDC, 0, 0, (int)sPW, (int)sPH);
            XFORM xFormIdentity = { 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f };
            SetWorldTransform(hMemDC, &xFormIdentity);
        }
    }

    if (m_state == PlayerState::ATTACK && m_currentFrame < 5) {
        CImage* slashImg = &imgSlashFX[m_currentFrame];
        if (!slashImg->IsNull()) {
            int sW = (int)(slashImg->GetWidth() * playerScale * pFitScale);
            int sH = (int)(slashImg->GetHeight() * playerScale * pFitScale);

            XFORM xForm;
            xForm.eM11 = cos(m_attackAngle);
            xForm.eM12 = sin(m_attackAngle);
            xForm.eM21 = -sin(m_attackAngle);
            xForm.eM22 = cos(m_attackAngle);
            xForm.eDx = vPX + (m_colW * pFitScale) / 2.0f;
            xForm.eDy = vPY + (m_colH * pFitScale) / 2.0f;
            SetWorldTransform(hMemDC, &xForm);

            slashImg->Draw(hMemDC, -sW / 2, -sH / 2, sW, sH);

            XFORM xFormIdentity = { 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f };
            SetWorldTransform(hMemDC, &xFormIdentity);
        }
    }

    if (g_showDebugRect) {
        HBRUSH greenBrush = CreateSolidBrush(RGB(0, 255, 0));
        RECT pRect = { (int)vPX, (int)vPY, (int)(vPX + m_colW * pFitScale), (int)(vPY + m_colH * pFitScale) };
        FrameRect(hMemDC, &pRect, greenBrush);
        DeleteObject(greenBrush);

        if (m_state == PlayerState::ATTACK) {
            float hitW = 80.0f * pFitScale;
            float hitH = 60.0f * pFitScale;
            float hitX = vPX + (m_colW * pFitScale) / 2.0f + m_attackDirX * 40.0f * pFitScale - hitW / 2.0f;
            float hitY = vPY + (m_colH * pFitScale) / 2.0f + m_attackDirY * 40.0f * pFitScale - hitH / 2.0f;
            HBRUSH redBrush = CreateSolidBrush(RGB(255, 0, 0));
            RECT aRect = { (int)hitX, (int)hitY, (int)(hitX + hitW), (int)(hitY + hitH) };
            FrameRect(hMemDC, &aRect, redBrush);
            DeleteObject(redBrush);
        }
    }
}
