#pragma once
#include "Enemy.h"
#include <atlimage.h>

enum class KissyfaceAction {
    KF_NONE,
    KF_IDLE,
    KF_WALK,
    KF_ATTACK,
    KF_HURT_FLY,
    KF_HURT_GROUND
};

class Kissyface : public Enemy {
private:
    static CImage m_imgIdle;
    KissyfaceAction m_ActionState;

public:
    Kissyface(float startX, float startY);
    virtual void Init() override;
    virtual void Update(float ts, const class Player& player) override;
    virtual void Render(HDC hdc, Gdiplus::Graphics* g, float camX, float camY, float mapScale, bool showDebugRect, bool isSlowMo = false) override;
    
    static void ReleaseAll();
};
