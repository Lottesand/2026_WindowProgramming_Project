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
    static CImage m_imgAxe;

    KissyfaceAction m_ActionState;
    float m_animTimer;
    int m_animFrame;

    float m_detectDistance = 350.0f; // Distance threshold for triggering patterns
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
