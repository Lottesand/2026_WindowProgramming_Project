#include "Kissyface.h"
#include "Player.h"
#include "Physics.h"
#include "../SceneAndMap/Camera.h"
#include "../UI/UIManager.h"
#include <gdiplus.h>
#include <cstdio>
#include <vector>
#include <cmath>

CImage Kissyface::m_imgIdle;
std::vector<CImage> Kissyface::m_imgBlock;
std::vector<CImage> Kissyface::m_imgThrow;
std::vector<CImage> Kissyface::m_imgTug;
std::vector<CImage> Kissyface::m_imgReturnAxe;
std::vector<CImage> Kissyface::m_imgPreJump;
std::vector<CImage> Kissyface::m_imgJump;
std::vector<CImage> Kissyface::m_imgLand;
std::vector<CImage> Kissyface::m_imgHurt;
std::vector<CImage> Kissyface::m_imgStruggle;
std::vector<CImage> Kissyface::m_imgRecover;
std::vector<CImage> Kissyface::m_imgPreLunge;
std::vector<CImage> Kissyface::m_imgLunge;
std::vector<CImage> Kissyface::m_imgLungeAttack;
CImage Kissyface::m_imgAxe;

Kissyface::Kissyface(float startX, float startY) 
    : Enemy(startX, startY, EnemyType::KISSYFACE) {
    m_colW = 40.0f;
    m_colH = 100.0f;
    m_ActionState = KissyfaceAction::KF_IDLE;
    m_vx = 0.0f;
    m_vy = 0.0f;    
    m_animTimer = 0.0f;
    m_animFrame = 0;
    m_patternDelayTimer = 0.0f;
    m_lungeTargetX = -1.0f; // Initialize with invalid value
    m_nextCloseAttackIsThrow = false; // 기본적으로 첫 근접 공격은 점프로 시작
    m_axe.state = AxeState::INACTIVE;
    m_detectDistance = 200.0f;
    m_hp = m_maxHp;
    m_downedTimer = 0.0f;
    m_struggleTimer = 0.0f;
    m_interactionPossible = false;
}

void Kissyface::Reset() {
    Enemy::Reset();
    m_ActionState = KissyfaceAction::KF_IDLE;
    m_animTimer = 0.0f;
    m_animFrame = 0;
    m_patternDelayTimer = 0.0f;
    m_nextCloseAttackIsThrow = false;
    m_axe.state = AxeState::INACTIVE;
    m_vx = 0.0f;
    m_vy = 0.0f;
    m_hp = m_maxHp;
    m_downedTimer = 0.0f;
    m_struggleTimer = 0.0f;
    m_interactionPossible = false;
}

void Kissyface::Init() {
    if (m_imgIdle.IsNull()) m_imgIdle.Load(TEXT("assets/boss/spr_kissyface_idle.png"));
    
    auto loadFrames = [](std::vector<CImage>& vec, const WCHAR* baseDir, const WCHAR* baseName, int count) {
        if (!vec.empty()) return;
        vec.resize(count);
        for (int i = 0; i < count; i++) {
            WCHAR path[MAX_PATH];
            swprintf_s(path, L"%s/%s_%d.png", baseDir, baseName, i);
            vec[i].Load(path);
        }
    };

    loadFrames(m_imgBlock, L"assets/boss/spr_kissyface_block", L"spr_kissyface_block", 5);
    loadFrames(m_imgThrow, L"assets/boss/spr_kissyface_throw", L"spr_kissyface_throw", 9);
    loadFrames(m_imgTug, L"assets/boss/spr_kissyface_tug", L"spr_kissyface_tug", 6);
    loadFrames(m_imgReturnAxe, L"assets/boss/spr_kissyface_returnaxe", L"spr_kissyface_returnaxe", 5);
    loadFrames(m_imgPreJump, L"assets/boss/spr_kissyface_prejump", L"spr_kissyface_prejump", 4);
    loadFrames(m_imgJump, L"assets/boss/spr_kissyface_jump", L"spr_kissyface_jump", 2);
    loadFrames(m_imgLand, L"assets/boss/spr_kissyface_landattack", L"spr_kissyface_landattack", 6);
    loadFrames(m_imgHurt, L"assets/boss/spr_kissyface_hurt", L"spr_kissyface_hurt", 6);
    loadFrames(m_imgStruggle, L"assets/boss/spr_kissyface_struggle", L"spr_kissyface_struggle", 2);
    loadFrames(m_imgRecover, L"assets/boss/spr_kissyface_recover", L"spr_kissyface_recover", 7);
    loadFrames(m_imgPreLunge, L"assets/boss/spr_kissyface_prelunge", L"spr_kissyface_prelunge", 4);
    loadFrames(m_imgLunge, L"assets/boss/spr_kissyface_lunge", L"spr_kissyface_lunge", 5);
    loadFrames(m_imgLungeAttack, L"assets/boss/spr_kissyface_lungeattack", L"spr_kissyface_lungeattack", 9);

    if (m_imgAxe.IsNull()) m_imgAxe.Load(TEXT("assets/boss/spr_kissyface_axe.png"));
}

void Kissyface::Update(float ts, const Player& player) {
    if (!m_isAlive && m_ActionState != KissyfaceAction::KF_HURT_FLY && m_ActionState != KissyfaceAction::KF_HURT_GROUND && m_ActionState != KissyfaceAction::KF_DOWNED && m_ActionState != KissyfaceAction::KF_STRUGGLE && m_ActionState != KissyfaceAction::KF_RECOVER) return;

    DWORD ct = GetTickCount();
    float pDist = (float)fabs(player.GetX() - m_x);

    // AI 패턴 발동 로직 (IDLE 상태이면서 살아있을 때만)
    if (m_isAlive && m_ActionState == KissyfaceAction::KF_IDLE) {
        m_patternDelayTimer += ts;
        if (m_patternDelayTimer > 1.2f) { // 1.2초 딜레이
            if (pDist > m_detectDistance) {
                int r = rand() % 100;
                if (r < 50) { 
                    m_ActionState = KissyfaceAction::KF_THROW;
                } else {
                    m_ActionState = KissyfaceAction::KF_PRELUNGE;
                    m_lungeTargetX = player.GetX() + player.GetColW() / 2.0f; // 플레이어 중앙 좌표를 타겟으로 고정
                }
                m_animFrame = 0;
                m_animTimer = 0;
            } else {
                if (m_nextCloseAttackIsThrow) {
                    m_ActionState = KissyfaceAction::KF_THROW;
                    m_nextCloseAttackIsThrow = false; // 다음은 점프 공격
                } else {
                    m_ActionState = KissyfaceAction::KF_PREJUMP;
                    m_nextCloseAttackIsThrow = true; // 다음은 도끼 투척
                }
                m_animFrame = 0;
                m_animTimer = 0;
            }
        }
    }

    // 애니메이션 및 상태 업데이트
    m_animTimer += ts;
    float frameDelay = m_delayBase / m_globalSpeedRate;

    switch (m_ActionState) {
    case KissyfaceAction::KF_BLOCK:
        if (m_animTimer > frameDelay) {
            m_animTimer = 0; m_animFrame++;
            if (m_animFrame >= 5) { m_animFrame = 0; m_ActionState = KissyfaceAction::KF_IDLE; m_patternDelayTimer = 0.0f; }
        }
        break;

    case KissyfaceAction::KF_THROW:
        if (m_animTimer > m_delayThrow / m_globalSpeedRate) {
            m_animTimer = 0; m_animFrame++;
            if (m_animFrame == 6) {
                m_axe.x = m_x + (m_isFacingLeft ? -20.0f : 60.0f);
                m_axe.y = m_y + 30.0f;
                float throwSpd = m_speedAxeThrow * m_globalSpeedRate;
                m_axe.vx = m_isFacingLeft ? -throwSpd : throwSpd;
                m_axe.vy = 0; m_axe.rotation = 0; m_axe.state = AxeState::FLYING;
            }
            if (m_animFrame >= 9) { m_animFrame = 0; m_ActionState = KissyfaceAction::KF_TUG; }
        }
        break;

    case KissyfaceAction::KF_PREJUMP:
        if (m_animTimer > 0.18f) {
            m_animTimer = 0; m_animFrame++;
            if (m_animFrame >= 4) {
                m_ActionState = KissyfaceAction::KF_JUMP;
                m_animFrame = 0; m_y -= 100.0f; m_vy = 0; 
                m_axe.state = AxeState::ORBITING; m_axe.orbitAngle = 0; m_axe.rotation = 0;
            }
        }
        break;

    case KissyfaceAction::KF_JUMP:
        if (m_animTimer > 0.18f) {
            m_animTimer = 0; m_animFrame++;
            if (m_animFrame >= 2) m_animFrame = 1; 
        }
        if (m_axe.state == AxeState::ORBITING) m_vy = 0;
        break;

    case KissyfaceAction::KF_LAND:
        if (m_animTimer > frameDelay) {
            m_animTimer = 0; m_animFrame++;
            if (m_animFrame >= 6) { m_animFrame = 0; m_ActionState = KissyfaceAction::KF_IDLE; m_patternDelayTimer = 0.0f; }
        }
        break;

    case KissyfaceAction::KF_PRELUNGE:
        // 준비 동작 동안 플레이어의 위치를 계속 추적 (실제 돌진 직전까지)
        m_lungeTargetX = player.GetX() + player.GetColW() / 2.0f;
        m_isFacingLeft = (m_lungeTargetX < m_x + m_colW / 2.0f); // 돌진 준비 중에도 항상 타겟을 바라보도록 설정

        if (m_animTimer > 0.15f) {
            m_animTimer = 0; m_animFrame++;
            if (m_animFrame >= 4) {
                m_ActionState = KissyfaceAction::KF_LUNGE;
                m_animFrame = 0;
                
                // 포물선 점프 계산
                float diff = m_lungeTargetX - (m_x + m_colW / 2.0f);
                float flightFrames = fabs(diff) / 20.0f; // 기본 수평 속도 20 기준 체공 프레임
                if (flightFrames < 15.0f) flightFrames = 15.0f; // 최소 체공 프레임
                if (flightFrames > 45.0f) flightFrames = 45.0f; // 최대 체공 프레임 제한
                
                m_vx = diff / flightFrames;
                m_vy = -0.5f * flightFrames; // 중력(1.0) 기준 낮게 뛰도록 계수 수정 (0.75 -> 0.5)
                m_isFacingLeft = (m_vx < 0);
            }
        }
        break;

    case KissyfaceAction::KF_LUNGE:
    {
        // 커스텀 포물선 물리 연산 (메인 물리 엔진 우회)
        m_vy += 1.0f * ts; // 낮아진 초기 상승 속도에 맞춰 중력 감소 (1.5 -> 1.0)
        m_x += m_vx * ts;
        m_y += m_vy * ts;

        // 속도에 따른 자연스러운 애니메이션 프레임 매핑 (움찔거림 방지)
        if (m_vy < -15.0f) m_animFrame = 0;      // 상승 시작
        else if (m_vy < -5.0f) m_animFrame = 1;  // 상승 중
        else if (m_vy < 5.0f) m_animFrame = 2;   // 최고점
        else if (m_vy < 15.0f) m_animFrame = 3;  // 하강 중
        else m_animFrame = 4;                    // 하강 끝

        // 타겟 X 좌표를 통과했는지 검사
        bool passedTarget = (m_vx > 0 && (m_x + m_colW / 2.0f) >= m_lungeTargetX) || 
                            (m_vx < 0 && (m_x + m_colW / 2.0f) <= m_lungeTargetX);
        
        if (passedTarget) {
            m_x = m_lungeTargetX - m_colW / 2.0f; // 타겟 X에 정확히 안착
            
            // 현재 X 좌표에서 바닥 높이 찾기 (착지 보정)
            for (float sy = m_y - 50.0f; sy <= m_y + 100.0f; sy += 1.0f) {
                int tc = GetCollisionType((int)(m_x + m_colW / 2.0f), (int)(sy + m_colH));
                if (tc == 1 || tc == 3) {
                    m_y = sy;
                    break;
                }
            }

            m_ActionState = KissyfaceAction::KF_LUNGEATTACK;
            m_animFrame = 0; m_animTimer = 0;
            m_vx = 0.0f;
            m_vy = 0.0f;
        }
    }
        break;

    case KissyfaceAction::KF_LUNGEATTACK:
        if (m_animTimer > 0.08f) {
            m_animTimer = 0; m_animFrame++;
            if (m_animFrame == 4) { // Attack frame
                float px = player.GetX(), py = player.GetY(), pw = player.GetColW(), ph = player.GetColH();
                float ex = m_x + (m_isFacingLeft ? -30.0f : 30.0f), ey = m_y + 30.0f;
                float dist = sqrt(pow(px + pw / 2 - ex, 2) + pow(py + ph / 2 - ey, 2));
                if (dist < 100.0f && !player.IsGodMode() && !player.IsDead() && player.GetState() != PlayerState::PS_ROLL) {
                    const_cast<Player&>(player).OnTakeDamage(1.0f, m_isFacingLeft ? -15.0f : 15.0f, -8.0f, m_x + m_colW / 2.0f, m_y + m_colH / 2.0f);
                }
            }
            if (m_animFrame >= 9) {
                m_ActionState = KissyfaceAction::KF_IDLE;
                m_animFrame = 0;
                m_patternDelayTimer = -0.8f; // 돌진 공격 후에는 총 2.0초(0.8 + 1.2) 대기
                m_lungeTargetX = 0.0f; // 타겟 좌표 초기화
            }
        }
        break;

    case KissyfaceAction::KF_HURT_FLY:
        if (m_animTimer > frameDelay) {
            m_animTimer = 0; m_animFrame++;
            if (m_animFrame >= 2) m_animFrame = 0; // Loop fly animation
        }
        break;

    case KissyfaceAction::KF_HURT_GROUND:
        if (m_animTimer > frameDelay) {
            m_animTimer = 0; m_animFrame++;
            if (m_animFrame >= 6) {
                m_animFrame = 5; // Stay on last frame (downed)
                m_ActionState = KissyfaceAction::KF_DOWNED;
                m_downedTimer = 0;
            }
        }
        break;

    case KissyfaceAction::KF_DOWNED:
        m_downedTimer += ts;
        if (m_downedTimer > 1.0f) {
            m_interactionPossible = true;
            // Interaction Check
            float dx = player.GetX() - m_x;
            float dy = player.GetY() - m_y;
            float dist = sqrt(dx*dx + dy*dy);
            if (dist < 100.0f && (GetAsyncKeyState(VK_LBUTTON) & 0x8000)) {
                m_ActionState = KissyfaceAction::KF_STRUGGLE;
                m_struggleTimer = 0;
                m_animFrame = 0;
                m_interactionPossible = false;
                const_cast<Player&>(player).SetVisible(false);
                const_cast<Player&>(player).ForceStop();
            }
        }
        break;

    case KissyfaceAction::KF_STRUGGLE:
        m_struggleTimer += ts;
        if (m_animTimer > m_delayPreLunge / m_globalSpeedRate) { // 0.15f -> m_delayPreLunge 기반
            m_animTimer = 0;
            m_animFrame = (m_animFrame + 1) % 2;
        }
        if (m_struggleTimer > 3.0f) {
            m_ActionState = KissyfaceAction::KF_RECOVER;
            m_animFrame = 0;
            const_cast<Player&>(player).SetVisible(true);
            const_cast<Player&>(player).Stun(0.5f, m_isFacingLeft ? 15.0f : -15.0f, -10.0f);
        }
        break;

    case KissyfaceAction::KF_RECOVER:
        if (m_animTimer > frameDelay) {
            m_animTimer = 0;
            m_animFrame++;
            if (m_animFrame >= 7) {
                m_animFrame = 0;
                m_ActionState = KissyfaceAction::KF_IDLE;
                m_patternDelayTimer = 0.0f;
                m_hp = m_maxHp; // Reset HP for next phase
                m_isAlive = true;
            }
        }
        break;

    case KissyfaceAction::KF_TUG:
        if (m_animTimer > frameDelay) {
            m_animTimer = 0; m_animFrame++;
            if (m_animFrame >= 6) m_animFrame = 0;
        }
        break;

    case KissyfaceAction::KF_RETURN_AXE:
        if (m_animTimer > frameDelay) {
            m_animTimer = 0; m_animFrame++;
            if (m_animFrame >= 5) { m_animFrame = 0; m_ActionState = KissyfaceAction::KF_IDLE; m_patternDelayTimer = 0.0f; }
        }
        break;

    default:
        break;
    }

    UpdateAxe(ts, player);

    // Physics
    bool useGravity = true;
    if (m_ActionState == KissyfaceAction::KF_JUMP && m_axe.state == AxeState::ORBITING) useGravity = false;
    if (m_ActionState == KissyfaceAction::KF_DOWNED || m_ActionState == KissyfaceAction::KF_STRUGGLE || m_ActionState == KissyfaceAction::KF_RECOVER) {
        useGravity = false; m_vx = 0; m_vy = 0;
    }

    if (useGravity) {
        m_vy += 1.5f * ts;
        if (m_vy > 30.0f) m_vy = 30.0f;
    }

    float ny = m_y + m_vy * ts;
    float nx = m_x + m_vx * ts;

    // Wall collision for horizontal movement
    if (m_vx != 0.0f) {
        if (CheckMapCollision(nx + (m_vx > 0 ? 10.0f : -10.0f), m_y, m_colW, m_colH)) {
            nx = m_x; // Stop horizontal movement
        }
    }

    bool floorHit = false;
    if (m_vy > 0.0f) {
        for (float sy = m_y; sy <= ny; sy += 1.0f) {
            int tc = GetCollisionType((int)(nx + m_colW / 2.0f), (int)(sy + m_colH));
            if (tc == 1 || tc == 3) {
                m_y = sy; m_vy = 0; floorHit = true;
                if (m_ActionState == KissyfaceAction::KF_HURT_FLY) {
                    m_ActionState = KissyfaceAction::KF_HURT_GROUND;
                    m_animFrame = 0; m_animTimer = 0;
                }
                if (m_ActionState == KissyfaceAction::KF_JUMP && m_axe.state == AxeState::INACTIVE) {
                    m_ActionState = KissyfaceAction::KF_LAND;
                    m_animFrame = 0; m_animTimer = 0;
                }
                break;
            }
        }
    }
    
    if (!floorHit) m_y = ny;
    m_x = nx; // Horizontal movement should always be applied (unless blocked by wall above)

    if (m_ActionState == KissyfaceAction::KF_IDLE || m_ActionState == KissyfaceAction::KF_WALK || m_ActionState == KissyfaceAction::KF_DOWNED) {
        m_isFacingLeft = (player.GetX() < m_x);
    }
}

void Kissyface::UpdateAxe(float ts, const Player& player) {
    if (m_axe.state == AxeState::INACTIVE) return;
    DWORD ct = GetTickCount();
    switch (m_axe.state) {
    case AxeState::FLYING:
        m_axe.x += m_axe.vx * ts; m_axe.rotation += 60.0f * ts;
        if (CheckMapCollision(m_axe.x + (m_axe.vx > 0 ? 10.0f : -10.0f), m_axe.y, 10, 10)) {
            m_axe.state = AxeState::STUCK; m_axe.stuckStartTime = ct;
        }
        break;
    case AxeState::STUCK:
        if (ct - m_axe.stuckStartTime > 800) m_axe.state = AxeState::RETURNING;
        break;
    case AxeState::RETURNING:
    {
        float targetX = m_x + (m_isFacingLeft ? 10.0f : 30.0f);
        float targetY = m_y + 40.0f;
        float dx = targetX - m_axe.x, dy = targetY - m_axe.y;
        float dist = (float)sqrt(dx * dx + dy * dy);
        if (dist < 40.0f) {
            m_axe.state = AxeState::INACTIVE;
            m_ActionState = KissyfaceAction::KF_RETURN_AXE;
            m_animFrame = 0; m_animTimer = 0;
        } else {
            float speed = 30.0f; // 도끼 회수 속도 느려짐 (40 -> 30)
            m_axe.vx = (dx / dist) * speed; m_axe.vy = (dy / dist) * speed;
            m_axe.x += m_axe.vx * ts; m_axe.y += m_axe.vy * ts;
            m_axe.rotation -= 60.0f * ts;
        }
        break;
    }
    case AxeState::ORBITING:
    {
        float radius = 100.0f; // 도끼 회전 반경 좁게 (150 -> 100)
        float orbitSpeed = m_speedAxeOrbit * m_globalSpeedRate * ts; 
        m_axe.orbitAngle += orbitSpeed;
        m_axe.rotation = (m_axe.orbitAngle * 180.0f / 3.141592f) + 90.0f;
        float centerX = m_x + m_colW / 2.0f, centerY = m_y + m_colH / 2.0f;
        m_axe.x = centerX + cos(m_axe.orbitAngle) * radius;
        m_axe.y = centerY + sin(m_axe.orbitAngle) * radius;
        if (m_axe.orbitAngle >= 6.28318f) {
            m_axe.state = AxeState::INACTIVE;
            if (m_ActionState == KissyfaceAction::KF_JUMP) m_vy = 2.0f;
        }
        break;
    }
    }
    if (m_axe.state != AxeState::STUCK) {
        float pX = player.GetX(), pY = player.GetY(), pW = player.GetColW(), pH = player.GetColH();
        float aw = 60.0f, ah = 60.0f;
        RECT axeR = { (int)(m_axe.x - aw/2), (int)(m_axe.y - ah/2), (int)(m_axe.x + aw/2), (int)(m_axe.y + ah/2) };
        RECT playerR = { (int)pX, (int)pY, (int)(pX + pW), (int)(pY + pH) };
        RECT overlap;
        if (IntersectRect(&overlap, &axeR, &playerR)) {
            if (!player.IsDead() && !player.IsGodMode() && player.GetState() != PlayerState::PS_ROLL) {
                const_cast<Player&>(player).OnTakeDamage(1.0f, (m_axe.x < pX ? 15.0f : -15.0f), -8.0f, m_x + m_colW / 2.0f, m_y + m_colH / 2.0f);
            }
        }
    }
}

RECT Kissyface::GetInvincibleRect() const {
    float splitX = m_x + (m_isFacingLeft ? (m_colW * (1.0f - FrontHitboxRatio)) : 0.0f);
    float width = m_colW * FrontHitboxRatio;
    return { (int)splitX, (int)m_y, (int)(splitX + width), (int)(m_y + m_colH) };
}

RECT Kissyface::GetVulnerableRect() const {
    float splitX = m_x + (m_isFacingLeft ? 0.0f : (m_colW * FrontHitboxRatio));
    float width = m_colW * (1.0f - FrontHitboxRatio);
    return { (int)splitX, (int)m_y, (int)(splitX + width), (int)(m_y + m_colH) };
}

bool Kissyface::OnTakeDamage(float kvx, float kvy) {
    if (!m_isAlive && m_ActionState != KissyfaceAction::KF_IDLE) return false;
    bool isFrontHit = m_isFacingLeft ? (kvx > 0) : (kvx < 0);
    if (isFrontHit && m_axe.state == AxeState::INACTIVE && m_ActionState != KissyfaceAction::KF_JUMP) {
        m_ActionState = KissyfaceAction::KF_BLOCK;
        m_animTimer = 0; m_animFrame = 0; return true;
    } else {
        m_hp -= 20.0f; // 임시 데미지
        if (m_hp <= 0) {
            m_isAlive = false;
            m_ActionState = KissyfaceAction::KF_HURT_FLY;
            m_vx = kvx; m_vy = kvy;
            m_animFrame = 0; m_animTimer = 0;
        } else {
            // Normal hurt without flying if needed
        }
        return false;
    }
}

void Kissyface::Render(HDC hdc, Gdiplus::Graphics* g, float camX, float camY, float mapScale, bool showDebugRect, bool isSlowMo) {
    if (!m_isAlive && m_ActionState == KissyfaceAction::KF_NONE) return;
    float pFS = mapScale;
    float vx = (m_x - camX) * pFS, vy = (m_y - camY) * pFS;

    CImage* currentImg = &m_imgIdle;
    switch (m_ActionState) {
    case KissyfaceAction::KF_BLOCK:      currentImg = &m_imgBlock[m_animFrame]; break;
    case KissyfaceAction::KF_THROW:      currentImg = &m_imgThrow[m_animFrame]; break;
    case KissyfaceAction::KF_TUG:        currentImg = &m_imgTug[m_animFrame]; break;
    case KissyfaceAction::KF_RETURN_AXE: currentImg = &m_imgReturnAxe[m_animFrame]; break;
    case KissyfaceAction::KF_PREJUMP:    currentImg = &m_imgPreJump[m_animFrame]; break;
    case KissyfaceAction::KF_JUMP:       currentImg = &m_imgJump[m_animFrame]; break;
    case KissyfaceAction::KF_LAND:       currentImg = &m_imgLand[m_animFrame]; break;
    case KissyfaceAction::KF_HURT_FLY:
    case KissyfaceAction::KF_HURT_GROUND: currentImg = &m_imgHurt[m_animFrame]; break;
    case KissyfaceAction::KF_DOWNED:     currentImg = &m_imgHurt[5]; break;
    case KissyfaceAction::KF_STRUGGLE:   currentImg = &m_imgStruggle[m_animFrame]; break;
    case KissyfaceAction::KF_RECOVER:    currentImg = &m_imgRecover[m_animFrame]; break;
    case KissyfaceAction::KF_PRELUNGE:   currentImg = &m_imgPreLunge[m_animFrame]; break;
    case KissyfaceAction::KF_LUNGE:      currentImg = &m_imgLunge[m_animFrame]; break;
    case KissyfaceAction::KF_LUNGEATTACK: currentImg = &m_imgLungeAttack[m_animFrame]; break;
    }

    if (currentImg && !currentImg->IsNull()) {
        int imgW = currentImg->GetWidth(), imgH = currentImg->GetHeight();
        float ds = 2.0f * pFS;
        int fw = (int)(imgW * ds), fh = (int)(imgH * ds);
        int dx = (int)(vx + (m_colW * pFS) / 2.0f - fw / 2.0f), dy = (int)(vy + (m_colH * pFS) - fh);
        
        if (m_isFacingLeft) {
            int om = SetGraphicsMode(hdc, GM_ADVANCED);
            XFORM xo;
            GetWorldTransform(hdc, &xo);
            XFORM xl = { -1.0f, 0.0f, 0.0f, 1.0f, (float)(2 * dx + fw), 0.0f };
            SetWorldTransform(hdc, &xl);
            currentImg->Draw(hdc, dx, dy, fw, fh);
            SetWorldTransform(hdc, &xo);
            SetGraphicsMode(hdc, om);
        } else {
            currentImg->Draw(hdc, dx, dy, fw, fh);
        }
    }

    if (m_interactionPossible) {
        UIManager::RenderLeftClickPrompt(hdc, m_x + m_colW / 2.0f, m_y - 20.0f, camX, camY, mapScale);
    }

    if (m_axe.state != AxeState::INACTIVE && !m_imgAxe.IsNull()) {
        float ax = (m_axe.x - camX) * pFS, ay = (m_axe.y - camY) * pFS;
        float ps = 1.2f * pFS;
        int aw = (int)(m_imgAxe.GetWidth() * ps), ah = (int)(m_imgAxe.GetHeight() * ps);
        Gdiplus::Bitmap bmp(m_imgAxe, NULL);
        Gdiplus::Matrix xo; g->GetTransform(&xo);
        g->TranslateTransform(ax, ay); g->RotateTransform(m_axe.rotation);
        Gdiplus::ImageAttributes attr;
        attr.SetColorKey(Gdiplus::Color(0, 0, 0), Gdiplus::Color(10, 10, 10));
        g->DrawImage(&bmp, Gdiplus::RectF(-(float)aw / 2.0f, -(float)ah / 2.0f, (float)aw, (float)ah), 0, 0, (float)bmp.GetWidth(), (float)bmp.GetHeight(), Gdiplus::UnitPixel, &attr);
        g->SetTransform(&xo);
    }

    if (showDebugRect) {
        RECT invRect = GetInvincibleRect(), vulRect = GetVulnerableRect();
        RECT dInv = { (int)((invRect.left - camX) * mapScale), (int)((invRect.top - camY) * mapScale), (int)((invRect.right - camX) * mapScale), (int)((invRect.bottom - camY) * mapScale) };
        RECT dVul = { (int)((vulRect.left - camX) * mapScale), (int)((vulRect.top - camY) * mapScale), (int)((vulRect.right - camX) * mapScale), (int)((vulRect.bottom - camY) * mapScale) };
        FrameRect(hdc, &dInv, CreateSolidBrush(RGB(255, 0, 0))); FrameRect(hdc, &dVul, CreateSolidBrush(RGB(0, 255, 0)));
        float lEX = m_x + (m_isFacingLeft ? -m_detectDistance : m_detectDistance);
        int dsx = (int)((m_x - camX) * mapScale), dex = (int)((lEX - camX) * mapScale), dsy = (int)((m_y + m_colH / 2.0f - camY) * mapScale);
        HPEN hP = CreatePen(PS_DOT, 1, RGB(0, 100, 255)); HPEN hO = (HPEN)SelectObject(hdc, hP);
        MoveToEx(hdc, dsx, dsy, NULL); LineTo(hdc, dex, dsy); SelectObject(hdc, hO); DeleteObject(hP);

        // Lunge Target Debug Line
        if (m_lungeTargetX > 0) {
            int tx = (int)((m_lungeTargetX - camX) * mapScale);
            HPEN hTP = CreatePen(PS_SOLID, 2, RGB(255, 100, 0)); HPEN hTO = (HPEN)SelectObject(hdc, hTP);
            MoveToEx(hdc, tx, 0, NULL); LineTo(hdc, tx, 720); // VIRTUAL_HEIGHT 대신 720 사용
            SelectObject(hdc, hTO); DeleteObject(hTP);
        }

        if (m_axe.state == AxeState::FLYING || m_axe.state == AxeState::RETURNING || m_axe.state == AxeState::ORBITING) {
            float aw = 60.0f, ah = 60.0f;
            RECT dAx = { (int)((m_axe.x - aw/2 - camX) * mapScale), (int)((m_axe.y - ah/2 - camY) * mapScale), (int)((m_axe.x + aw/2 - camX) * mapScale), (int)((m_axe.y + ah/2 - camY) * mapScale) };
            FrameRect(hdc, &dAx, CreateSolidBrush(RGB(255, 165, 0)));
        }
    }
}

void Kissyface::ReleaseAll() {
    m_imgIdle.Destroy();
    auto destroyVec = [](std::vector<CImage>& vec) { for (auto& img : vec) img.Destroy(); vec.clear(); };
    destroyVec(m_imgBlock); destroyVec(m_imgThrow); destroyVec(m_imgTug); destroyVec(m_imgReturnAxe);
    destroyVec(m_imgPreJump); destroyVec(m_imgJump); destroyVec(m_imgLand);
    destroyVec(m_imgHurt); destroyVec(m_imgStruggle); destroyVec(m_imgRecover);
    destroyVec(m_imgPreLunge); destroyVec(m_imgLunge); destroyVec(m_imgLungeAttack);
    m_imgAxe.Destroy();
}
