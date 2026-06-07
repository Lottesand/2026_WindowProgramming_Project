#pragma once
#include <windows.h>
#include <atlimage.h>
#include <math.h>

class Player;

enum class EnemyType { GANGSTER, GRUNT, POMP, SHIELDCOP };
enum class EnemyState { ES_IDLE, ES_WALK, ES_FALL, ES_ALERT, ES_ATTACK, ES_DEAD };
enum class GangsterAction { GA_NONE, GA_AIM, GA_FIRE, GA_TURN, GA_RUN, GA_HURT_FLY, GA_HURT_GROUND };
enum class GruntAction { GR_NONE, GR_ATTACK, GR_SLASH, GR_TURN, GR_RUN, GR_HURT_FLY, GR_HURT_GROUND };
enum class PompAction { PA_NONE, PA_ATTACK, PA_BOX_IDLE, PA_BOX_HIT, PA_TURN, PA_RUN, PA_HURT_FLY, PA_HURT_GROUND };
enum class ShieldCopAction { SA_NONE, SA_AIM, SA_BASH, SA_TURN, SA_RUN, SA_HURT_FLY, SA_HURT_GROUND };

class Enemy {
protected:
    float m_x, m_y, m_startX, m_startY, m_vx, m_vy, m_colW, m_colH;
    bool m_isAlive, m_isFacingLeft, m_isImmortal, m_isWaiting;
    float m_bloodDistance;
    EnemyType m_Type; EnemyState m_State;
    int m_CurrentFrame; DWORD m_LastTime, m_patternTimer;
    float m_friction, m_knockbackVx, m_walkDistance, m_patrolRange;

    // Detection & Alert
    bool m_isPlayerDetected;
    DWORD m_alertStartTime;
    int m_exclaimFrame;
    static CImage m_ImgExclaim[2];

    // Detection constants
    float m_detectRange = 400.0f;
    float m_detectAngle = 45.0f; // Cone angle (half of total field)

public:
    Enemy(float startX, float startY, EnemyType type, float patrolRange = 150.0f);
    virtual ~Enemy();
    virtual void Init() = 0;
    virtual void Reset();
    virtual void Update(float ts, const class Player& player);
    virtual void Render(HDC hdc, Gdiplus::Graphics* g, float camX, float camY, float mapScale, bool showDebugRect, bool isSlowMo = false) = 0;
    virtual void RenderDetectionRange(HDC hdc, float camX, float camY, float mapScale);
    virtual void RenderExclaim(HDC hdc, float camX, float camY, float mapScale);
    virtual void OnTakeDamage(float kvx, float kvy);
    virtual void ApplyKnockback(float vx);
    
    bool IsPlayerInCone(float px, float py, float pw, float ph);
    void UpdateDetection(float px, float py, float pw, float ph, float ts);
    void RenderDebug(HDC hdc, float camX, float camY, float mapScale);

    static void ReleaseAll();
    EnemyType GetType() const { return m_Type; }
    bool GetIsAlive() const { return m_isAlive; }
    void SetImmortal(bool immortal) { m_isImmortal = immortal; }
    float GetX() const { return m_x; }
    float GetY() const { return m_y; }
    float GetColW() const { return m_colW; }
    float GetColH() const { return m_colH; }
    RECT GetRect() const { return { (int)m_x, (int)m_y, (int)(m_x + m_colW), (int)(m_y + m_colH) }; }
    void SetPatrolRange(float range) { m_patrolRange = range; }
};

class Gangster : public Enemy {
private:
    GangsterAction m_ActionState;
    static CImage m_ImgIdle_R[8], m_ImgIdle_L[8], m_ImgWalk_R[8], m_ImgWalk_L[8], m_ImgAim_R[4], m_ImgAim_L[4], m_ImgTurn_R[6], m_ImgTurn_L[6], m_ImgFall_R[12], m_ImgFall_L[12], m_ImgHurtFly_R[2], m_ImgHurtFly_L[2], m_ImgHurtGround_R[14], m_ImgHurtGround_L[14], m_ImgRun_R[10], m_ImgRun_L[10], m_ImgGun_R[2], m_ImgGun_L[2], m_ImgArm[2];
public:

    Gangster(float x, float y); virtual ~Gangster();
    virtual void Init() override;
    virtual void Reset() override;
    virtual void Update(float ts, const Player& player) override;
    virtual void Render(HDC hdc, Gdiplus::Graphics* g, float camX, float camY, float mapScale, bool showDebugRect, bool isSlowMo) override;
    virtual void OnTakeDamage(float kvx, float kvy) override;
    static void Release();
};

class Grunt : public Enemy {
private:
    GruntAction m_ActionState;
    static CImage m_ImgIdle_R[8], m_ImgIdle_L[8], m_ImgWalk_R[10], m_ImgWalk_L[10], m_ImgAttack_R[8], m_ImgAttack_L[8], m_ImgSlash_R[5], m_ImgSlash_L[5], m_ImgTurn_R[8], m_ImgTurn_L[8], m_ImgFall_R[13], m_ImgFall_L[13], m_ImgHurtFly_R[2], m_ImgHurtFly_L[2], m_ImgHurtGround_R[16], m_ImgHurtGround_L[16], m_ImgRun_R[10], m_ImgRun_L[10];
public:
    Grunt(float x, float y); virtual ~Grunt();
    virtual void Init() override;
    virtual void Reset() override;
    virtual void Update(float ts, const Player& player) override;
    virtual void Render(HDC hdc, Gdiplus::Graphics* g, float camX, float camY, float mapScale, bool showDebugRect, bool isSlowMo) override;
    virtual void OnTakeDamage(float kvx, float kvy) override;
    static void Release();
};

class Pomp : public Enemy {
private:
    PompAction m_ActionState;
    static CImage m_ImgIdle_R[8], m_ImgIdle_L[8], m_ImgWalk_R[10], m_ImgWalk_L[10], m_ImgAttack_R[6], m_ImgAttack_L[6], m_ImgBoxIdle_R[10], m_ImgBoxIdle_L[10], m_ImgBoxHit_R[14], m_ImgBoxHit_L[14], m_ImgTurn_R[6], m_ImgTurn_L[6], m_ImgFall_R[13], m_ImgFall_L[13], m_ImgHurtFly_R[2], m_ImgHurtFly_L[2], m_ImgHurtGround_R[15], m_ImgHurtGround_L[15], m_ImgRun_R[10], m_ImgRun_L[10];
public:
    Pomp(float x, float y); virtual ~Pomp();
    virtual void Init() override;
    virtual void Reset() override;
    virtual void Update(float ts, const Player& player) override;
    virtual void Render(HDC hdc, Gdiplus::Graphics* g, float camX, float camY, float mapScale, bool showDebugRect, bool isSlowMo) override;
    virtual void OnTakeDamage(float kvx, float kvy) override;
    static void Release();
};

class ShieldCop : public Enemy {
private:
    ShieldCopAction m_ActionState;
    static CImage m_ImgIdle_R[6], m_ImgIdle_L[6], m_ImgWalk_R[10], m_ImgWalk_L[10], m_ImgRun_R[10], m_ImgRun_L[10], m_ImgTurn_R[8], m_ImgTurn_L[8], m_ImgAim_R[19], m_ImgAim_L[19], m_ImgBash_R[6], m_ImgBash_L[6], m_ImgKnockback_R[2], m_ImgKnockback_L[2], m_ImgTragedyDie_R[15], m_ImgTragedyDie_L[15];
public:
    ShieldCop(float x, float y); virtual ~ShieldCop();
    virtual void Init() override;
    virtual void Reset() override;
    virtual void Update(float ts, const Player& player) override;
    virtual void Render(HDC hdc, Gdiplus::Graphics* g, float camX, float camY, float mapScale, bool showDebugRect, bool isSlowMo) override;
    virtual void OnTakeDamage(float kvx, float kvy) override;
    static void Release();
};
