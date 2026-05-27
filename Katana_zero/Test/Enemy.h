#pragma once
#include <windows.h>
#include <atlimage.h>
#include <math.h>

enum class EnemyType
{
    GANGSTER,
    GRUNT,
    POMP,
    SHIELDCOP
};

enum class EnemyState
{
    IDLE,
    WALK,
    FALL,
    DEAD
};

enum class GangsterAction
{
    NONE,
    AIM,
    FIRE,
    TURN,
    RUN,
    HURT_FLY,
    HURT_GROUND
};

enum class GruntAction
{
    NONE,
    ATTACK,
    SLASH,
    TURN,
    RUN,
    HURT_FLY,
    HURT_GROUND
};

enum class PompAction
{
    NONE,
    ATTACK,
    BOX_IDLE,
    BOX_HIT,
    TURN,
    RUN,
    HURT_FLY,
    HURT_GROUND
};

enum class ShieldCopAction
{
    NONE,
    AIM,
    BASH,
    TURN,
    RUN,
    HURT_FLY,
    HURT_GROUND
};

class Enemy
{
protected:
    float m_x, m_y;
    float m_vx, m_vy;
    float m_colW, m_colH;
    bool m_isAlive;
    bool m_isFacingLeft;
    EnemyType m_Type;
    EnemyState m_State;
    int m_CurrentFrame;
    DWORD m_LastTime;

    float m_friction;
    float m_knockbackVx;

public:
    Enemy(float startX, float startY, EnemyType type);
    virtual ~Enemy();

    virtual void Init() = 0;
    virtual void Update() = 0;
    virtual void Render(HDC hdc) = 0;
    virtual void OnTakeDamage(float damage);
    virtual void ApplyKnockback(float vx);

    EnemyType GetType() const;
    bool GetIsAlive() const;

    float GetX() const { return m_x; }
    float GetY() const { return m_y; }
    float GetColW() const { return m_colW; }
    float GetColH() const { return m_colH; }

    RECT GetRect() const {
        return { (int)m_x, (int)m_y, (int)(m_x + m_colW), (int)(m_y + m_colH) };
    }
};

class Gangster : public Enemy
{
private:
    GangsterAction m_ActionState;
    CImage m_ImgIdle_R[8], m_ImgIdle_L[8];
    CImage m_ImgWalk_R[8], m_ImgWalk_L[8];
    CImage m_ImgAim_R[7], m_ImgAim_L[7];
    CImage m_ImgFire_R[6], m_ImgFire_L[6];
    CImage m_ImgTurn_R[6], m_ImgTurn_L[6];
    CImage m_ImgFall_R[12], m_ImgFall_L[12];
    CImage m_ImgHurtFly_R[2], m_ImgHurtFly_L[2];
    CImage m_ImgHurtGround_R[14], m_ImgHurtGround_L[14];
    CImage m_ImgRun_R[10], m_ImgRun_L[10];

public:
    Gangster(float startX, float startY);
    virtual ~Gangster();

    virtual void Init() override;
    virtual void Update() override;
    virtual void Render(HDC hdc) override;
    virtual void OnTakeDamage(float damage) override;
};

class Grunt : public Enemy
{
private:
    GruntAction m_ActionState;
    CImage m_ImgIdle_R[8], m_ImgIdle_L[8];
    CImage m_ImgWalk_R[10], m_ImgWalk_L[10];
    CImage m_ImgAttack_R[8], m_ImgAttack_L[8];
    CImage m_ImgSlash_R[5], m_ImgSlash_L[5];
    CImage m_ImgTurn_R[8], m_ImgTurn_L[8];
    CImage m_ImgFall_R[13], m_ImgFall_L[13];
    CImage m_ImgHurtFly_R[2], m_ImgHurtFly_L[2];
    CImage m_ImgHurtGround_R[16], m_ImgHurtGround_L[16];
    CImage m_ImgRun_R[10], m_ImgRun_L[10];

public:
    Grunt(float startX, float startY);
    virtual ~Grunt();

    virtual void Init() override;
    virtual void Update() override;
    virtual void Render(HDC hdc) override;
    virtual void OnTakeDamage(float damage) override;
};

class Pomp : public Enemy
{
private:
    PompAction m_ActionState;
    CImage m_ImgIdle_R[8], m_ImgIdle_L[8];
    CImage m_ImgWalk_R[10], m_ImgWalk_L[10];
    CImage m_ImgAttack_R[6], m_ImgAttack_L[6];
    CImage m_ImgBoxIdle_R[10], m_ImgBoxIdle_L[10];
    CImage m_ImgBoxHit_R[14], m_ImgBoxHit_L[14];
    CImage m_ImgTurn_R[6], m_ImgTurn_L[6];
    CImage m_ImgFall_R[13], m_ImgFall_L[13];
    CImage m_ImgHurtFly_R[2], m_ImgHurtFly_L[2];
    CImage m_ImgHurtGround_R[15], m_ImgHurtGround_L[15];
    CImage m_ImgRun_R[10], m_ImgRun_L[10];

public:
    Pomp(float startX, float startY);
    virtual ~Pomp();

    virtual void Init() override;
    virtual void Update() override;
    virtual void Render(HDC hdc) override;
    virtual void OnTakeDamage(float damage) override;
};

class ShieldCop : public Enemy
{
private:
    ShieldCopAction m_ActionState;
    CImage m_ImgIdle_R[6], m_ImgIdle_L[6];
    CImage m_ImgWalk_R[10], m_ImgWalk_L[10];
    CImage m_ImgRun_R[10], m_ImgRun_L[10];
    CImage m_ImgTurn_R[8], m_ImgTurn_L[8];
    CImage m_ImgAim_R[19], m_ImgAim_L[19];
    CImage m_ImgBash_R[6], m_ImgBash_L[6];
    CImage m_ImgKnockback_R[2], m_ImgKnockback_L[2];
    CImage m_ImgTragedyDie_R[15], m_ImgTragedyDie_L[15];

public:
    ShieldCop(float startX, float startY);
    virtual ~ShieldCop();

    virtual void Init() override;
    virtual void Update() override;
    virtual void Render(HDC hdc) override;
    virtual void OnTakeDamage(float damage) override;
};