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
    KF_HURT_GROUND,
    KF_DIE,        // Dying animation after final struggle
    KF_DEAD,       // Dead on the ground (idle)
    KF_NOHEAD      // Final blow (decapitation)
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

struct KissyfaceAfterImage {
    float x, y;
    int frame;
    KissyfaceAction state;
    bool isFacingLeft;
    float alpha; // 0.0 to 1.0
    Gdiplus::Color color;
};

class Kissyface : public Enemy {
public:
    // ==========================================
    // [ 속도 조절 설정 (Speed Configuration) ]
    // 전체 속도 마스터 변수 (1.0: 표준, 낮을수록 전체적으로 느려짐)
    float m_globalSpeedRate = 0.5f; 

    // [ 개별 애니메이션 기준 딜레이 (초) ]
    float m_delayBase       = 0.15f;
    float m_delayThrow      = 0.12f;
    float m_delayPreJump    = 1.2f;
    float m_delayPreLunge   = 1.8f; 
    float m_delayLunge      = 0.12f;
    float m_delayLungeAttack= 0.1f;

    // [ 개별 물리/이동 기준 속도 ]
    // 도끼 속도는 그대로 유지하기 위해 베이스 값을 높임 (0.5 곱해질 것 감안)
    float m_speedAxeThrow   = 55.0f;
    float m_speedAxeReturn  = 65.0f;
    float m_speedAxeOrbit   = 0.55f; 
    float m_lungeFlightDiv  = 25.0f; 
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
    static std::vector<CImage> m_imgDie;
    static std::vector<CImage> m_imgDead;
    static std::vector<CImage> m_imgNoHead;
    static CImage m_imgAxe;

    std::vector<KissyfaceAfterImage> m_afterImages;
    DWORD m_lastAfterImageTime;
    const int m_maxAfterImages = 5;
    const int m_afterImageInterval = 30; // ms (기존 60 -> 30으로 간격 좁힘)

    KissyfaceAction m_ActionState;
    float m_animTimer;
    int m_animFrame;
    float m_patternDelayTimer; // Timer for delay before next pattern
    float m_lungeTargetX;      // Target X position for lunge pattern
    float m_throwProbability;  // Probability for Throw attack (0-100)
    bool m_nextCloseAttackIsThrow; // Tracks the alternating close-range pattern

    float m_detectDistance = 150.0f; // Distance threshold for triggering patterns
    AxeProjectile m_axe;

    // Struggle mechanic members
    float m_downedTimer;
    float m_struggleTimer;
    float m_struggleProgress;  // 0.0 to 1.0 (QTE progress)
    float m_struggleCircleProgress; // 0.0 to 1.0 (Current phase progress)
    int m_strugglePhase;       // 1 to 4 (Phase of the boss)
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
    void Parry();
    
    static void ReleaseAll();

private:
    void UpdateAxe(float ts, const class Player& player);
};
