#pragma once
#include "Enemy.h"
#include <atlimage.h>
#include <vector>

namespace Gdiplus { class Graphics; }

enum class KissyfaceAction {
    KF_NONE,
    KF_IDLE,
    KF_WALK,
    KF_ATTACK,
    KF_BLOCK,      // Parry response
    KF_THROW,      // Axe throw animation
    KF_TUG,        // Holding the rope while axe is out
    KF_RETURN_AXE, // Catching the axe
    KF_PREJUMP,    // Preparing to jump
    KF_JUMP,       // In the air
    KF_LAND,       // Landing after jump
    KF_PRELUNGE,   // Preparing to lunge
    KF_LUNGE,      // Lunging towards player
    KF_LUNGEATTACK, // Attacking after lunge
    KF_DOWNED,     // Downed on the ground
    KF_STRUGGLE,   // Struggling with player (player hidden)
    KF_RECOVER,    // Standing back up
    KF_HURT_FLY,
    KF_HURT_GROUND
};

enum class AxeState { INACTIVE, FLYING, STUCK, RETURNING, ORBITING };

struct AxeProjectile {
    float x, y;
    float vx, vy;
    float rotation;
    float orbitAngle;
    AxeState state;
    DWORD stuckStartTime;
};

class Kissyface : public Enemy {
public:
    // ==========================================
    // [ 속도 조절 설정 (Speed Configuration) ]
    // 전체 속도 마스터 변수 (1.0: 표준, 낮을수록 전체적으로 느려짐, 높을수록 빨라짐)
    float m_globalSpeedRate = 0.75f; 

    // [ 개별 애니메이션 기준 딜레이 (초) ]
    // 실제 적용 = 딜레이 / m_globalSpeedRate
    float m_delayBase       = 0.12f;
    float m_delayThrow      = 0.11f;
    float m_delayPreJump    = 0.18f;
    float m_delayPreLunge   = 0.35f; // 준비 모션 더더욱 느리게 (0.25 -> 0.35)
    float m_delayLunge      = 0.1f;
    float m_delayLungeAttack= 0.08f;

    // [ 개별 물리/이동 기준 속도 ]
    // 실제 적용 = 속도 * m_globalSpeedRate
    float m_speedAxeThrow   = 30.0f;
    float m_speedAxeReturn  = 40.0f;
    float m_speedAxeOrbit   = 0.40f; // 점프 공격 도끼 회전 속도 빠르게 (0.22 -> 0.40)
    float m_lungeFlightDiv  = 20.0f; // 포물선 체공 계수 (작을수록 길게 뜀)
    // ==========================================

private:
    static CImage m_imgIdle;
    static std::vector<CImage> m_imgBlock;
    static std::vector<CImage> m_imgThrow;
    static std::vector<CImage> m_imgTug;
    static std::vector<CImage> m_imgReturnAxe;
    static std::vector<CImage> m_imgPreJump;
    static std::vector<CImage> m_imgJump;
    static std::vector<CImage> m_imgLand;
    static std::vector<CImage> m_imgHurt;
    static std::vector<CImage> m_imgStruggle;
    static std::vector<CImage> m_imgRecover;
    static std::vector<CImage> m_imgPreLunge;
    static std::vector<CImage> m_imgLunge;
    static std::vector<CImage> m_imgLungeAttack;
    static CImage m_imgAxe;

    KissyfaceAction m_ActionState;
    float m_animTimer;
    int m_animFrame;
    float m_patternDelayTimer; // Timer for delay before next pattern
    float m_lungeTargetX;      // Target X position for lunge pattern
    bool m_nextCloseAttackIsThrow; // Tracks the alternating close-range pattern

    float m_detectDistance = 200.0f; // Distance threshold for triggering patterns
    AxeProjectile m_axe;

    // Struggle mechanic members
    float m_downedTimer;
    float m_struggleTimer;
    bool m_interactionPossible;
    float m_hp;
    const float m_maxHp = 100.0f;

    // Kissyface hitbox configuration
    static constexpr float FrontHitboxRatio = 0.5f; 

public:
    Kissyface(float startX, float startY);
    
    // Returns the vulnerable part of the hitbox
    RECT GetVulnerableRect() const;
    // Returns the invincible part of the hitbox
    RECT GetInvincibleRect() const;
    
    virtual void Init() override;
    virtual void Reset() override;
    virtual void Update(float ts, const class Player& player) override;
    virtual void Render(HDC hdc, Gdiplus::Graphics* g, float camX, float camY, float mapScale, bool showDebugRect, bool isSlowMo = false) override;
    virtual bool OnTakeDamage(float kvx, float kvy) override;
    
    static void ReleaseAll();

private:
    void UpdateAxe(float ts, const class Player& player);
};
