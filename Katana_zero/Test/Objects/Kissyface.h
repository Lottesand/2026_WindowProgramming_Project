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
    float m_globalSpeedRate = 0.5f; 

    float m_delayBase       = 0.15f;
    float m_delayThrow      = 0.12f;
    float m_delayPreJump    = 1.2f;
    float m_delayPreLunge   = 1.8f; 
    float m_delayLunge      = 0.12f;
    float m_delayLungeAttack= 0.1f;

    float m_speedAxeThrow   = 55.0f;
    float m_speedAxeReturn  = 65.0f;
    float m_speedAxeOrbit   = 0.55f; 
    float m_lungeFlightDiv  = 25.0f; 
    // ==========================================

private:
    static std::vector<CImage> m_imgIdle;
    static std::vector<CImage> m_imgWalk;
    static std::vector<CImage> m_imgSlash;
    static std::vector<CImage> m_imgBlock;
    static std::vector<CImage> m_imgThrow;
    static std::vector<CImage> m_imgTug;
    static std::vector<CImage> m_imgReturnAxe;
    static std::vector<CImage> m_imgPreJump;
    static std::vector<CImage> m_imgJump;
    static std::vector<CImage> m_imgLand;
    static std::vector<CImage> m_imgHurtFly;
    static std::vector<CImage> m_imgHurtGround;
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
    const int m_afterImageInterval = 30;

    KissyfaceAction m_ActionState;
    KissyfaceAction m_lastActionState;
    float m_animTimer;
    int m_animFrame;
    float m_patternDelayTimer;
    float m_lungeTargetX;
    float m_throwProbability;
    bool m_nextCloseAttackIsThrow;

    float m_detectDistance = 150.0f;
    AxeProjectile m_axe;

    float m_downedTimer;
    float m_struggleTimer;
    float m_struggleProgress;
    float m_struggleCircleProgress;
    int m_strugglePhase;
    bool m_interactionPossible;
    float m_hp;
    const float m_maxHp = 100.0f;

    static constexpr float FrontHitboxRatio = 0.5f; 
public:
    Kissyface(float startX, float startY);
    virtual ~Kissyface();
    
    RECT GetVulnerableRect() const;
    RECT GetInvincibleRect() const;
    
    virtual void Init() override;
    virtual void Reset() override;
    virtual void Update(float ts, const class Player& player) override;
    virtual void Render(HDC hdc, Gdiplus::Graphics* g, float camX, float camY, float mapScale, bool showDebugRect, bool isSlowMo = false) override;
    virtual void RenderSilhouette(Gdiplus::Graphics* g, float camX, float camY, float mapScale) override; // 추가
    virtual bool OnTakeDamage(float kvx, float kvy, DeathCause cause = DeathCause::SWORD) override;
    void Parry();
    
    KissyfaceAction GetActionState() const { return m_ActionState; }
    
    static void ReleaseAll();

private:
    void UpdateAxe(float ts, const class Player& player);
};
