#include "Game.h"
#include "Input.h"
#include "SoundManager.h"
#include "../SceneAndMap/Camera.h"
#include "../SceneAndMap/StageManager.h"
#include "../Objects/Door.h"
#include "../Objects/Bullet.h"
#include "../Effects/EffectManager.h"
#include "../UI/UIManager.h"
#include "../UI/StartScene.h"
#include "../Objects/Item.h"
#include "../Objects/Kissyface.h"
#include <time.h>
#include <stdlib.h>
#include <gdiplus.h>
#include <algorithm>
#include <cmath>
#include <thread>
#include <objbase.h>

Game* g_pGame = nullptr;

Game::Game() : m_hWnd(NULL), m_hInst(NULL), m_winWidth(1280), m_winHeight(720),
    m_isTimePaused(false), m_isTimeoutDeath(false), m_showDebugRect(false), m_isFullMapView(false),
    m_renderMapScale(1.0f), m_renderPlayerScale(2.0f), m_mapOffsetX(0.0f), m_mapOffsetY(0.0f),
    m_prevTime(0), m_currentStage(1), m_isStageCleared(false), m_stageTimer(0.0f), m_bGameStarted(false),
    m_fps(0), m_frameCount(0), m_lastFpsTime(0) {
    m_isLoaded = false;
    m_loadingProgress = 0;
    g_pGame = this;
}

Game::~Game() {
    for (auto& e : m_enemies) if (e) delete e;
    m_enemies.clear();
    Bullet::Release();
    Item::ReleaseAssets();
    SoundManager::Release();
}

void Game::Init(HWND hWnd, HINSTANCE hInst) {
    m_hWnd = hWnd; m_hInst = hInst; m_prevTime = GetTickCount();
    std::thread loadingThread(LoadingThreadProc, this);
    loadingThread.detach();
}

void Game::LoadingThreadProc(Game* pGame) {
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    pGame->LoadAllAssets();
    CoUninitialize();
}

void Game::LoadAllAssets() {
    m_loadingProgress = 0;
    m_imgLoading.Load(TEXT("assets/loading.png"));
    m_imgReplayUI[0].Load(TEXT("assets/Replay/0.png"));
    m_imgReplayUI[1].Load(TEXT("assets/Replay/1.png"));
    m_imgReplayUI[2].Load(TEXT("assets/Replay/3.png"));
    m_imgReplayUI[3].Load(TEXT("assets/Replay/yes.png"));
    m_loadingProgress = 5; Sleep(50);
    UIManager::LoadAssets(); m_loadingProgress = 15; Sleep(50);
    EffectManager::LoadAssets(); m_loadingProgress = 20; Sleep(50);
    Gangster(0, 0).Init(); Grunt(0, 0).Init(); m_loadingProgress = 30; Sleep(50);
    Pomp(0, 0).Init(); ShieldCop(0, 0).Init(); Bullet::Init(); m_loadingProgress = 40; Sleep(50);
    Enemy::LoadCommonAssets();
    m_player.Init(); Item::LoadAssets(); m_loadingProgress = 50; Sleep(50);
    UIManager::Init(); m_loadingProgress = 60; Sleep(50);
    StartScene::LoadAssets(); m_loadingProgress = 70; Sleep(50);

    SoundManager::Load("BGM_MAIN", L"assets/sound/song_katanazero.wav");
    SoundManager::Load("BGM_BOSS", L"assets/sound/bgm_boss.mp3");
    SoundManager::Load("SFX_PLAYER_DIE", L"assets/sound/player/playerdie.wav");
    SoundManager::Load("SFX_SLASH1", L"assets/sound/player/playerslash1.wav");
    SoundManager::Load("SFX_SLASH2", L"assets/sound/player/playerslash2.wav");
    SoundManager::Load("SFX_SLASH3", L"assets/sound/player/playerslash3.wav");
    SoundManager::Load("SFX_SLASH_BULLET", L"assets/sound/player/playerslashbullet.wav");
    SoundManager::Load("SFX_ROLL", L"assets/sound/player/playerroll.wav");
    SoundManager::Load("SFX_LAND", L"assets/sound/player/playerland.wav");
    SoundManager::Load("SFX_WALK", L"assets/sound/player/playerfootstep.wav");
    SoundManager::Load("SFX_PRERUN", L"assets/sound/player/playerprerun.wav");
    SoundManager::Load("SFX_KICK_DOOR", L"assets/sound/player/playerkickdoor.wav");
    SoundManager::Load("SFX_JUMP", L"assets/sound/player/playerjump.wav");
    SoundManager::Load("SFX_WALLKICK", L"assets/sound/player/playerwallkick.wav");
    SoundManager::Load("SFX_DEFLECT", L"assets/sound/player/playerslashbullet.wav");
    SoundManager::Load("SFX_THROW", L"assets/sound/player/playerthrow.wav");
    SoundManager::Load("SFX_WALLSLIDE_LOOP", L"assets/sound/player/playerwallslide.wav");
    SoundManager::Load("SFX_REWIND_LOOP", L"assets/sound/rewind.wav");
    SoundManager::Load("SFX_LEVEL_START", L"assets/sound/level_start.wav");
    SoundManager::Load("SFX_PARRY", L"assets/sound/player/playerparring.wav");
    SoundManager::Load("SFX_HIT", L"assets/sound/enemy/grunt_punchhit.wav");
    SoundManager::Load("SFX_PUNCH", L"assets/sound/enemy/grunt_punch.wav");
    SoundManager::Load("SFX_SWING", L"assets/sound/enemy/pomp_swing.wav");
    SoundManager::Load("SFX_BLOODSPLAT", L"assets/sound/enemy/enemy_bloodsplat.wav");
    SoundManager::Load("SFX_HITFLOOR", L"assets/sound/enemy/enemy_hitfloor.wav");
    SoundManager::Load("SFX_GUNFIRE", L"assets/sound/enemy/enemy_gun_fire.wav");
    SoundManager::Load("SFX_BULLETDIE", L"assets/sound/enemy/bulletdie.wav");
    SoundManager::Load("SFX_ENEMY_DIE_BOTTLE", L"assets/sound/enemy/enemy_death_bottle.wav");
    SoundManager::Load("SFX_ENEMY_DIE_BULLET", L"assets/sound/enemy/enemy_death_bullet.wav");
    SoundManager::Load("SFX_ENEMY_DIE_KNIFE", L"assets/sound/enemy/enemy_death_knife.wav");
    SoundManager::Load("SFX_ENEMY_DIE_SWORD1", L"assets/sound/enemy/enemy_death_sword1.wav");
    SoundManager::Load("SFX_ENEMY_DIE_SWORD2", L"assets/sound/enemy/enemy_death_sword2.wav");
    SoundManager::Load("SFX_GO", L"assets/sound/go.wav");
    SoundManager::Load("SFX_REPLAY_EJECT", L"assets/sound/replay_eject.wav");
    SoundManager::Load("SFX_REPLAY_PLAY_LOOP", L"assets/sound/replay_play.wav");
    SoundManager::Load("SFX_REPLAY_PAUSE_LOOP", L"assets/sound/replay_pause.wav");
    SoundManager::Load("SFX_EXPLOSION_1", L"assets/sound/explosion1.wav");
    SoundManager::Load("SFX_EXPLOSION_2", L"assets/sound/explosion2.wav");
    SoundManager::Load("vial_explosion", L"assets/sound/vial_explosion.wav");
    SoundManager::Load("SFX_FLAMETHROWER", L"assets/sound/flamethrower.wav");

    m_loadingProgress = 80; Sleep(50);
    StageManager::LoadAllStages(&m_loadingProgress);
    LoadStage(1); m_player.SetMaxHistory(m_maxRewindTime); m_loadingProgress = 100; Sleep(500);
    m_isLoaded = true;
}

void Game::LoadStage(int stage) {
    m_player.DiscardHeldItem();
    m_currentStage = stage; m_isStageCleared = false; EffectManager::SetReplayMode(false); Bullet::SetReplayMode(false);
    if (m_currentStage == 1) { m_bGameStarted = false; StartScene::SetActive(true); } else { m_bGameStarted = true; StartScene::SetActive(false); }
    StageManager::LoadAssets(m_currentStage);
    if (m_currentStage == 1) m_maxRewindTime = 20; else m_maxRewindTime = 5;
    if (StageManager::GetMap().IsNull() || StageManager::GetColMap().IsNull()) return; 
    StageManager::Init(); SpawnEnemies(); Bullet::ClearAll();
    StageManager::StageData& data = StageManager::GetStageData(m_currentStage); mapScale = data.mapScale; Camera::SetLookAheadX(data.camLookAheadX); Camera::SetFixedY(data.camFixedY); Camera::SetLerpSpeedX(data.camLerpSpeedX); Camera::SetLerpSpeedY(data.camLerpSpeedY);
    m_stageTimer = StageManager::GetStageLimitTime(); m_player.SetMaxHistory((int)m_stageTimer); m_player.SetPos(StageManager::GetPlayerStartX(), StageManager::GetPlayerStartY() - m_player.GetColH() - 2.0f); Camera::Init();
    Camera::Update(m_player.GetX(), m_player.GetY(), m_player.GetColW(), m_player.GetColH(), Input::GetMouseX(), Input::GetMouseY(), m_renderMapScale, StageManager::GetMapWidth(), StageManager::GetMapHeight(), m_isFullMapView);
}

void Game::SpawnEnemies() {
    for (auto& e : m_enemies) if (e) delete e;
    m_enemies.clear();
    const auto& stageData = StageManager::GetStageData(m_currentStage);
    for (const auto& info : stageData.enemySpawns) {
        Enemy* ne = nullptr;
        switch (info.type) {
        case 0: ne = new Gangster(info.x, info.y); break;
        case 1: ne = new Grunt(info.x, info.y); break;
        case 2: ne = new Pomp(info.x, info.y); break;
        case 3: ne = new ShieldCop(info.x, info.y); break;
        case 4: ne = new Kissyface(info.x, info.y); break;
        default: ne = new Gangster(info.x, info.y); break;
        }
        if (ne) { ne->Init(); ne->SetPatrolRange(info.patrolRange); m_enemies.push_back(ne); }
    }

    if (m_currentStage == 5) {
        Kissyface* boss = new Kissyface(400.0f, 300.0f);
        boss->Init();
        m_enemies.push_back(boss);
    }
}

void Game::Update() {
    DWORD ct = GetTickCount(); m_frameCount++; if (ct - m_lastFpsTime >= 1000) { m_fps = m_frameCount; m_frameCount = 0; m_lastFpsTime = ct; }
    if (!m_isLoaded || m_displayedProgress < 100.0f) {
        int target = m_loadingProgress.load();
        if (m_displayedProgress < target) { m_displayedProgress += 0.4f; if (m_displayedProgress > target) m_displayedProgress = (float)target; }
        if (!m_isLoaded || m_displayedProgress < 100.0f) return;
    }

    static bool s_devicesInitialized = false;
    if (!s_devicesInitialized) {
        SoundManager::InitDevices();
        s_devicesInitialized = true;
    }
    if (ct - m_prevTime < 16) return;
    float dT = (ct - m_prevTime) / 1000.0f; Input::Update(); UpdateScreenScale();
    if (m_transitionState != TransitionState::NONE) { float transitionSpeed = 3.5f; if (m_transitionState == TransitionState::ENTERING) { m_transitionProgress += dT * transitionSpeed; if (m_transitionProgress >= 1.0f) { m_transitionProgress = 1.0f; m_transitionState = TransitionState::WAITING; m_transitionWaitTime = ct; if (m_transitionToNextStage) { 
            SoundManager::Stop("SFX_REPLAY_PLAY_LOOP");
            SoundManager::Stop("SFX_REPLAY_PAUSE_LOOP");
            SoundManager::SetGlobalVolume(1.0f);
            m_gameMode = GameMode::PLAYING; m_player.ClearSnapshots(); if (m_currentStage < 5) LoadStage(m_currentStage + 1); else LoadStage(1); m_transitionToNextStage = false; } } } else if (m_transitionState == TransitionState::WAITING) { if (ct - m_transitionWaitTime > 150) { m_transitionState = TransitionState::LEAVING; m_transitionProgress = 0.0f; } } else if (m_transitionState == TransitionState::LEAVING) { m_transitionProgress += dT * transitionSpeed; if (m_transitionProgress >= 1.0f) { m_transitionState = TransitionState::NONE; m_transitionProgress = 0.0f; } } m_prevTime = ct; return; }
    if (!m_bGameStarted) {
        if (StartScene::IsActive()) { bool wasActive = StartScene::IsActive(); StartScene::Update(dT, m_bGameStarted); }
        else { if (Input::GetKeyDown(VK_LBUTTON)) { m_bGameStarted = true; m_prevTime = GetTickCount(); } }
        Camera::Update(m_player.GetX(), m_player.GetY(), m_player.GetColW(), m_player.GetColH(), Input::GetMouseX(), Input::GetMouseY(), m_renderMapScale, StageManager::GetMapWidth(), StageManager::GetMapHeight(), m_isFullMapView);
        m_prevTime = ct; return;
    }
    
    bool anyAlive = false; int activeEnemies = 0; for (auto& e : m_enemies) { if (e) { activeEnemies++; if (e->GetIsAlive()) { anyAlive = true; break; } } } 
    
    if (m_currentStage == 5) {
        bool kissyNoHead = false;
        for (auto& e : m_enemies) {
            if (e && e->GetType() == EnemyType::KISSYFACE) {
                Kissyface* k = static_cast<Kissyface*>(e);
                if (k->GetActionState() == KissyfaceAction::KF_NOHEAD) { kissyNoHead = true; break; }
            }
        }
        if (kissyNoHead && !m_isStageCleared) { m_isStageCleared = true; SoundManager::Play("SFX_GO"); }
    } else {
        if (anyAlive) m_isStageCleared = false; 
        else { if (activeEnemies > 0 && !m_isStageCleared) { m_isStageCleared = true; SoundManager::Play("SFX_GO"); } }
    }
    
    if (m_isStageCleared && m_gameMode == GameMode::PLAYING) { if (StageManager::IsInClearZone(m_player.GetX(), m_player.GetY(), m_player.GetColW(), m_player.GetColH())) { m_gameMode = GameMode::YES_SCENE; m_yesSceneStartTime = GetTickCount(); m_player.SetState(PlayerState::PS_IDLE); EffectManager::Init(); m_player.ClearAfterImages(); m_prevTime = GetTickCount(); return; } }
    if (m_gameMode == GameMode::YES_SCENE) { 
        if (ct - m_yesSceneStartTime > 2000) { 
            m_gameMode = GameMode::REPLAYING; m_replayFrame = 0; EffectManager::SetReplayMode(true); Bullet::SetReplayMode(true); StageManager::Reset(); for (auto& e : m_enemies) if (e) e->Reset(); const auto& fullHistory = m_player.GetSnapshots(); if (!fullHistory.empty()) { m_player.SetPos(fullHistory[0].x, fullHistory[0].y); m_player.SetState(fullHistory[0].state); Camera::Update(m_player.GetX(), m_player.GetY(), m_player.GetColW(), m_player.GetColH(), Input::GetMouseX(), Input::GetMouseY(), m_renderMapScale, StageManager::GetMapWidth(), StageManager::GetMapHeight(), m_isFullMapView, true); } 
            SoundManager::SetGlobalVolume(0.5f);
            SoundManager::Play("SFX_REPLAY_EJECT"); SoundManager::Play("SFX_REPLAY_PLAY_LOOP", true);
        } m_prevTime = ct; return; 
    }
    if (m_gameMode == GameMode::REPLAYING) { 
        if (Input::GetKeyDown(VK_LBUTTON)) { SoundManager::Stop("SFX_REPLAY_PLAY_LOOP"); SoundManager::Stop("SFX_REPLAY_PAUSE_LOOP"); SoundManager::SetGlobalVolume(1.0f); m_transitionState = TransitionState::ENTERING; m_transitionProgress = 0.0f; m_transitionToNextStage = true; m_prevTime = ct; return; } 
        if (Input::GetKeyDown(VK_SPACE)) { m_isReplayPaused = !m_isReplayPaused; if (m_isReplayPaused) { SoundManager::Stop("SFX_REPLAY_PLAY_LOOP"); SoundManager::Play("SFX_REPLAY_PAUSE_LOOP", true); } else { SoundManager::Stop("SFX_REPLAY_PAUSE_LOOP"); SoundManager::Play("SFX_REPLAY_PLAY_LOOP", true); } }
        if (!m_isReplayPaused) { const auto& fullHistory = m_player.GetSnapshots(); if (m_replayFrame < (int)fullHistory.size()) { for (int i = 0; i < m_replaySpeed; i++) { int targetFrame = m_replayFrame + i; if (targetFrame < (int)fullHistory.size()) { for (const auto& ev : fullHistory[targetFrame].events) { if (ev.type == Player::ReplayEvent::ENEMY_DIE) { if (ev.targetIdx >= 0 && ev.targetIdx < (int)m_enemies.size()) m_enemies[ev.targetIdx]->OnTakeDamage(5.0f, -5.0f); } else if (ev.type == Player::ReplayEvent::DOOR_OPEN) StageManager::GetStageData(m_currentStage).doors[ev.targetIdx].Open(true, ct); } } } const auto& d = fullHistory[m_replayFrame]; m_player.SetPos(d.x, d.y); m_player.SetState(d.state); for (auto& e : m_enemies) { if (e) { for (int i = 0; i < m_replaySpeed; i++) e->Update(1.0f, m_player); } } bool forceSnap = (m_replayFrame == 0); m_replayFrame += m_replaySpeed; Camera::Update(m_player.GetX(), m_player.GetY(), m_player.GetColW(), m_player.GetColH(), Input::GetMouseX(), Input::GetMouseY(), m_renderMapScale, StageManager::GetMapWidth(), StageManager::GetMapHeight(), m_isFullMapView, forceSnap); } else { m_transitionState = TransitionState::ENTERING; m_transitionProgress = 0.0f; m_transitionToNextStage = true; } } else Camera::Update(m_player.GetX(), m_player.GetY(), m_player.GetColW(), m_player.GetColH(), Input::GetMouseX(), Input::GetMouseY(), m_renderMapScale, StageManager::GetMapWidth(), StageManager::GetMapHeight(), m_isFullMapView, false); m_prevTime = ct; return; }
    if (m_player.IsDead() && !m_player.IsRewinding()) {
        static bool isDeadShaken = false; static DWORD deadStartTime = 0;
        if (!isDeadShaken) { Camera::AddShake(1.5f); Camera::AddPush(0.0f, 40.0f); isDeadShaken = true; deadStartTime = ct; m_isTimePaused = false; SoundManager::Play("SFX_PLAYER_DIE"); }
        if (Input::GetKeyDown(VK_LBUTTON) && m_player.IsDeathAnimationFinished() && (ct - deadStartTime > 800)) {
            isDeadShaken = false; deadStartTime = 0; m_isTimeoutDeath = false; SoundManager::Stop("SFX_PLAYER_DIE"); m_initialRewindHistorySize = m_player.GetHistorySize(); float historySeconds = (float)m_initialRewindHistorySize / 60.0f; m_rewindSpeed = (int)(4.0f + historySeconds * 2.5f); if (m_rewindSpeed < 4) m_rewindSpeed = 4; if (m_rewindSpeed > 100) m_rewindSpeed = 100; m_player.StartRewind(m_rewindSpeed); StageManager::SoftReset(); m_stageTimer = StageManager::GetStageLimitTime(); EffectManager::Init(); for (auto& e : m_enemies) if (e) e->Reset(); 
            SoundManager::Pause("BGM_MAIN"); SoundManager::Play("SFX_REWIND_LOOP", true);
        }
    }

    if (Input::GetKeyDown('F')) m_isFullMapView = !m_isFullMapView; if (Input::GetKeyDown('E')) { m_showDebugRect = !m_showDebugRect; } if (Input::GetKeyDown('I')) m_player.SetGodMode(!m_player.IsGodMode()); if (Input::GetKeyDown('Q')) { int r = rand() % 5; ItemType randomType = static_cast<ItemType>(r); Item* newItem = new Item(randomType, m_player.GetX(), m_player.GetY()); m_player.PickUpItem(newItem); } if (Input::GetKeyDown('1')) LoadStage(1); if (Input::GetKeyDown('2')) LoadStage(2); if (Input::GetKeyDown('3')) LoadStage(3); if (Input::GetKeyDown('4')) LoadStage(4); if (Input::GetKeyDown('5')) LoadStage(5);
    if (Input::GetKeyDown(VK_OEM_PLUS) || Input::GetKeyDown(0xBB)) { SoundManager::SetGlobalVolume(SoundManager::GetGlobalVolume() + 0.1f); } if (Input::GetKeyDown(VK_OEM_MINUS) || Input::GetKeyDown(0xBD)) { SoundManager::SetGlobalVolume(SoundManager::GetGlobalVolume() - 0.1f); }
    static bool prevR_local = false; bool cuR = GetAsyncKeyState('R') & 0x8000; if (cuR && !prevR_local) { m_isTimeoutDeath = false; 
        m_player.DiscardHeldItem();
        m_initialRewindHistorySize = m_player.GetHistorySize(); float elapsed = StageManager::GetStageLimitTime() - m_stageTimer; float rewindDur = (std::max)(1.0f, (std::min)(5.0f, elapsed / 6.0f)); m_rewindSpeed = (rewindDur > 0) ? (int)(m_initialRewindHistorySize / (rewindDur * 60.0f)) : 4; if (m_rewindSpeed < 1) m_rewindSpeed = 1; m_player.StartRewind(m_rewindSpeed); StageManager::SoftReset(); m_stageTimer = StageManager::GetStageLimitTime(); EffectManager::Init(); for (auto& e : m_enemies) if (e) e->Reset(); 
        SoundManager::Pause("BGM_MAIN"); SoundManager::Play("SFX_REWIND_LOOP", true);
    } prevR_local = cuR;
    if (!m_player.IsRewinding() && !m_player.GetIsSlowMo() && !m_isTimePaused && !m_player.IsGodMode() && !m_player.IsDead()) { float dT_timer = (ct - m_prevTime) / 1000.0f; m_stageTimer -= dT_timer; if (m_stageTimer <= 0) { m_stageTimer = 0; if (!m_player.IsGodMode() && !m_player.IsDead()) { m_isTimeoutDeath = true; m_player.SetState(PlayerState::PS_DEAD); } } }
    if (m_player.IsRewinding()) { 
        int currentHist = m_player.GetHistorySize(); float progress = (float)currentHist / m_initialRewindHistorySize; if (progress <= 0.2f && !Camera::IsRewindEffectActive()) { float remainingSec = (float)currentHist / (m_rewindSpeed * 60.0f); Camera::StartRewindEffect(remainingSec > 0 ? remainingSec : 1.0f); } if (currentHist > (int)(m_initialRewindHistorySize * 0.2f)) m_player.Update(Input::GetMouseX(), Input::GetMouseY(), Camera::GetCamX(), Camera::GetCamY(), m_renderMapScale, m_mapOffsetX, m_mapOffsetY, m_isFullMapView); else { m_player.SetPos(StageManager::GetPlayerStartX(), StageManager::GetPlayerStartY() - m_player.GetColH() - 2.0f); m_player.ClearHistory(); } m_player.UpdateAnimation(); bool forceSnap = (m_player.GetHistorySize() == 0); Camera::Update(m_player.GetX(), m_player.GetY(), m_player.GetColW(), m_player.GetColH(), Input::GetMouseX(), Input::GetMouseY(), m_renderMapScale, StageManager::GetMapWidth(), StageManager::GetMapHeight(), m_isFullMapView, forceSnap); 
        if (currentHist == 0 && !Camera::IsRewindEffectActive()) { 
            m_player.StopRewind(); m_player.SetState(PlayerState::PS_IDLE); m_player.SetPos(StageManager::GetPlayerStartX(), StageManager::GetPlayerStartY() - m_player.GetColH() - 2.0f); m_player.ClearHistory(); m_player.ClearSnapshots(); Camera::Update(m_player.GetX(), m_player.GetY(), m_player.GetColW(), m_player.GetColH(), Input::GetMouseX(), Input::GetMouseY(), m_renderMapScale, StageManager::GetMapWidth(), StageManager::GetMapHeight(), m_isFullMapView, true); 
            SoundManager::Stop("SFX_REWIND_LOOP"); SoundManager::Resume("BGM_MAIN"); 
        } m_prevTime = ct; return; 
    }
    float ts = m_player.GetIsSlowMo() ? 0.3f : 1.0f;
    if (m_player.GetIsSlowMo() != m_prevSlowMo) {
        if (m_player.GetIsSlowMo()) { SoundManager::Pause("BGM_MAIN"); SoundManager::Pause("BGM_BOSS"); SoundManager::SetGlobalVolume(0.0f); }
        else { if (!m_player.IsRewinding()) { SoundManager::Resume("BGM_MAIN"); SoundManager::Resume("BGM_BOSS"); } SoundManager::SetGlobalVolume(1.0f); }
        m_prevSlowMo = m_player.GetIsSlowMo();
    }
    if (!m_isTimePaused) {
        m_player.Update(Input::GetMouseX(), Input::GetMouseY(), Camera::GetCamX(), Camera::GetCamY(), m_renderMapScale, m_mapOffsetX, m_mapOffsetY, m_isFullMapView);
        if (m_player.GetState() == PlayerState::PS_ATTACK) {
            RECT attackRect = m_player.GetAttackRect(); InflateRect(&attackRect, 15, 15);
            for (auto b : Bullet::GetBullets()) { if (b && b->IsActive() && !b->IsDeflected()) { RECT bulletRect = b->GetRect(), overlap; if (IntersectRect(&overlap, &attackRect, &bulletRect)) { b->Deflect(-b->GetVX(), -b->GetVY()); EffectManager::AddBulletReflectVFX((float)(overlap.left + overlap.right) / 2.0f, (float)(overlap.top + overlap.bottom) / 2.0f, atan2(-b->GetVY(), -b->GetVX()), ct); SoundManager::Play("SFX_DEFLECT"); } } }
        }
        Bullet::UpdateAll(ts, m_player, m_enemies); 
        StageManager::UpdateItems(ts, m_player, m_enemies);
        StageManager::UpdateOilDrums(ts, m_enemies, &m_player);
        if (!m_player.IsDead()) { for (auto& e : m_enemies) if (e) e->Update(ts, m_player); }

        if (m_player.IsFiringFlamethrower()) {
            float fDirX = m_player.GetFlameDirX();
            float fDirY = m_player.GetFlameDirY();
            float holdTime = m_player.GetFlamethrowerHoldTime();
            float ratio = (std::min)(1.0f, holdTime / 2.0f);
            int numSprites = 3 + (int)(4.0f * ratio);
            float pX = m_player.GetX() + m_player.GetColW() / 2.0f;
            float pY = m_player.GetY() + m_player.GetColH() / 2.0f;
            float maxDist = numSprites * 40.0f; 
            for (int i = 0; i < (int)m_enemies.size(); i++) {
                Enemy* e = m_enemies[i];
                if (e && e->GetIsAlive()) {
                    float ex = e->GetX() + e->GetColW() / 2.0f;
                    float ey = e->GetY() + e->GetColH() / 2.0f;
                    float dx = ex - pX, dy = ey - pY;
                    float dist = sqrt(dx*dx + dy*dy);
                    if (dist < maxDist && dist > 0.0f) {
                        float dot = (dx * fDirX + dy * fDirY) / dist;
                        if (dot > 0.85f) { // Within cone
                            e->OnTakeDamage(fDirX * 15.0f, -5.0f, DeathCause::FIRE);
                            m_player.AddReplayEvent(Player::ReplayEvent::ENEMY_DIE, i);
                        }
                    }
                }
            }
            
            // Check collision with Oil Drums
            auto oilDrums = StageManager::GetCurrentOilDrums();
            if (oilDrums) {
                for (auto& drum : *oilDrums) {
                    if (drum.IsExploded() || drum.IsPending()) continue;
                    float dx = (drum.GetX() + drum.GetWidth() / 2.0f) - pX;
                    float dy = (drum.GetY() + drum.GetHeight() / 2.0f) - pY;
                    float dist = sqrt(dx * dx + dy * dy);
                    if (dist < maxDist && dist > 0.0f) {
                        float dot = (dx * fDirX + dy * fDirY) / dist;
                        if (dot > 0.85f) { // Within cone
                            drum.Trigger(100 + rand() % 200);
                        }
                    }
                }
            }
        }
    }
    static int laF = -1;
    if (!m_isTimePaused && m_player.GetState() == PlayerState::PS_ATTACK) {
        int cf = m_player.GetCurrentFrame(); if (cf >= 1 && cf <= 3 && cf != laF) {
            float cX = m_player.GetX() + m_player.GetColW() / 2.0f, cY = m_player.GetY() + m_player.GetColH() / 2.0f, hX = cX + m_player.GetAttackDirX() * 40.0f - 40.0f, hY = cY + m_player.GetAttackDirY() * 40.0f - 30.0f; RECT aR = { (int)hX, (int)hY, (int)(hX + 80.0f), (int)(hY + 60.0f) };
            
            EffectManager::RemoveSmokeInArea(hX, hY, 80.0f, 60.0f);

            if (m_player.GetAttackDirY() > 0.5f) { auto glassDomes = StageManager::GetCurrentGlassDomes(); if (glassDomes) { for (auto& gd : *glassDomes) { if (!gd.IsBroken()) { RECT gdR = { (int)gd.GetX(), (int)gd.GetY(), (int)(gd.GetX() + gd.GetW()), (int)(gd.GetY() + gd.GetH()) }, ol; if (IntersectRect(&ol, &aR, &gdR)) { gd.Break(ct); Camera::AddShake(2.0f); } } } } }
            for (int i = 0; i < (int)m_enemies.size(); i++) {
                Enemy* e = m_enemies[i]; 
                if (e) {
                    bool isKissyDead = (e->GetType() == EnemyType::KISSYFACE && static_cast<Kissyface*>(e)->GetActionState() == KissyfaceAction::KF_DEAD);
                    if (e->GetIsAlive() || isKissyDead) {
                        bool hitDetected = false, parryDetected = false; 
                        if (e->GetType() == EnemyType::KISSYFACE) { 
                            if (m_player.HasHitThisSwing()) continue; 
                            Kissyface* k = static_cast<Kissyface*>(e); 
                            RECT vulR = k->GetVulnerableRect(), invR = k->GetInvincibleRect(), ol; 
                            if (IntersectRect(&ol, &aR, &vulR)) { hitDetected = true; m_player.SetHasHitThisSwing(true); } 
                            else if (IntersectRect(&ol, &aR, &invR)) { parryDetected = true; m_player.SetHasHitThisSwing(true); } 
                        } else { 
                            RECT eR = e->GetRect(), ol; if (IntersectRect(&ol, &aR, &eR)) hitDetected = true; 
                        }
                        if (hitDetected || parryDetected) {
                            m_isTimePaused = true; m_player.AddReplayEvent(Player::ReplayEvent::ENEMY_DIE, i); float ex = e->GetX() + e->GetColW() / 2.0f, ey = e->GetY() + e->GetColH() / 2.0f, dx = ex - cX, dy = ey - cY, dist = (std::max)(1.0f, (float)sqrt(dx * dx + dy * dy)), ux = dx / dist, uy = dy / dist; EffectManager::AddNeonTrail(ex, ey, ux, uy, atan2(uy, ux)); EffectManager::AddHitVFX(ex, ey, atan2(uy, ux), ct); Camera::AddPush(ux * 30.0f, uy * 30.0f); Camera::AddShake(1.0f); float kbPower = 25.0f, attackDx = m_player.GetAttackDirX(), attackDy = m_player.GetAttackDirY(), kvx = attackDx * kbPower, kvy = attackDy * kbPower; if (kvy > -5.0f) kvy -= 8.0f;
                            if (parryDetected) { m_player.Stun(0.5f, -kvx * 1.8f, -10.0f); EffectManager::AddBulletReflectVFX((float)aR.left + (aR.right - aR.left) / 2.0f, (float)aR.top + (aR.bottom - aR.top) / 2.0f, atan2(-kvy, -kvx), GetTickCount()); static_cast<Kissyface*>(e)->Parry(); SoundManager::Play("SFX_PARRY"); }
                            else e->OnTakeDamage(kvx, kvy, DeathCause::SWORD);
                        }
                    }
                }
            }
            laF = cf;
        }
    } else if (!m_isTimePaused) laF = -1;
    if (!m_isTimePaused) {
        int openedDoorIdx = StageManager::UpdateDoors(m_player.GetX(), m_player.GetY(), m_player.GetColW(), m_player.GetColH(), (GetAsyncKeyState('A') & 0x8000) != 0, (GetAsyncKeyState('D') & 0x8000) != 0, m_player.GetState() == PlayerState::PS_ATTACK, m_player.GetAttackHitX(), m_player.GetAttackHitY(), m_player.GetAttackHitW(), m_player.GetAttackHitH(), ct, ts); StageManager::UpdateGlassDomes(m_player.GetState() == PlayerState::PS_ATTACK, m_player.GetAttackHitX(), m_player.GetAttackHitY(), m_player.GetAttackHitW(), m_player.GetAttackHitH(), ct);
        if (openedDoorIdx != -1) { m_player.AddReplayEvent(Player::ReplayEvent::DOOR_OPEN, openedDoorIdx); if (m_player.GetState() != PlayerState::PS_ATTACK && m_player.GetState() != PlayerState::PS_DOOR_KICK && m_player.GetState() != PlayerState::PS_DOOR_KICK_FULL) m_player.SetState(PlayerState::PS_DOOR_KICK); SoundManager::Play("SFX_KICK_DOOR"); }
        auto& stageData = StageManager::GetStageData(m_currentStage); for (int i = 0; i < (int)stageData.doors.size(); i++) { Door& d = stageData.doors[i]; if (d.GetCurrentFrame() >= 1 && d.GetCurrentFrame() <= 5) { RECT dR = { (int)d.GetX(), (int)d.GetY(), (int)(d.GetX() + d.GetW()), (int)(d.GetY() + d.GetH()) }; dR.left -= 30; dR.right += 30; for (int j = 0; j < (int)m_enemies.size(); j++) { Enemy* e = m_enemies[j]; if (e && e->GetIsAlive()) { RECT eR = e->GetRect(), ol; if (IntersectRect(&ol, &dR, &eR)) { float ex = e->GetX() + e->GetColW() / 2.0f, ey = e->GetY() + e->GetColH() / 2.0f, kbx = (ex > d.GetX() + d.GetW() / 2.0f) ? 15.0f : -15.0f; e->OnTakeDamage(kbx, -5.0f, DeathCause::SWORD); m_player.AddReplayEvent(Player::ReplayEvent::ENEMY_DIE, j); float angle = (kbx > 0) ? 0.0f : 3.14159f; EffectManager::AddNeonTrail(ex, ey, (kbx > 0 ? 1.0f : -1.0f), 0.0f, angle); EffectManager::AddHitVFX(ex, ey, angle, ct); Camera::AddShake(0.8f); } } } } }
    }
    EffectManager::Update(ts, ct); if (m_isTimePaused && !EffectManager::HasActiveHitVFX()) m_isTimePaused = false; if (!m_isTimePaused) m_player.UpdateAnimation(); Camera::Update(m_player.GetX(), m_player.GetY(), m_player.GetColW(), m_player.GetColH(), Input::GetMouseX(), Input::GetMouseY(), m_renderMapScale, StageManager::GetMapWidth(), StageManager::GetMapHeight(), m_isFullMapView); m_prevTime = ct;
}

void Game::Render(HDC hDC) {
    HDC hMemDC = CreateCompatibleDC(hDC); HBITMAP hMemBmp = CreateCompatibleBitmap(hDC, VIRTUAL_WIDTH, VIRTUAL_HEIGHT); HBITMAP hOldBmp = (HBITMAP)SelectObject(hMemDC, hMemBmp); PatBlt(hMemDC, 0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, BLACKNESS);
    if (!m_isLoaded) { if (!m_imgLoading.IsNull()) m_imgLoading.Draw(hMemDC, 0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT); int barW = 400, barH = 10, margin = 60; RECT barBg = { VIRTUAL_WIDTH - barW - margin, VIRTUAL_HEIGHT - barH - margin, VIRTUAL_WIDTH - margin, VIRTUAL_HEIGHT - margin }; HBRUSH hBgBrush = CreateSolidBrush(RGB(40, 40, 40)); FillRect(hMemDC, &barBg, hBgBrush); DeleteObject(hBgBrush); int progressWidth = (int)((float)barW * (m_displayedProgress / 100.0f)); RECT barProgress = { barBg.left, barBg.top, barBg.left + progressWidth, barBg.bottom }; HBRUSH hPrgBrush = CreateSolidBrush(RGB(0, 255, 255)); FillRect(hMemDC, &barProgress, hPrgBrush); DeleteObject(hPrgBrush); SetBkMode(hMemDC, TRANSPARENT); SetTextColor(hMemDC, RGB(200, 200, 200)); HFONT hFont = CreateFont(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH | FF_SWISS, TEXT("Arial")); HFONT hOldFont = (HFONT)SelectObject(hMemDC, hFont); TCHAR szProgress[32]; wsprintf(szProgress, TEXT("%d%%"), (int)m_displayedProgress); RECT textRect = { barBg.left, barBg.top - 25, barBg.right, barBg.top }; DrawText(hMemDC, szProgress, -1, &textRect, DT_RIGHT | DT_SINGLELINE); SelectObject(hMemDC, hOldFont); DeleteObject(hFont); SetStretchBltMode(hDC, HALFTONE); StretchBlt(hDC, 0, 0, m_winWidth, m_winHeight, hMemDC, 0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, SRCCOPY); SelectObject(hMemDC, hOldBmp); DeleteObject(hMemBmp); DeleteDC(hMemDC); return; }
    float cX = Camera::GetCamX(), cY = Camera::GetCamY(); Camera::ApplyShake(cX, cY); if (Camera::IsRewindEffectActive()) { int mapH = StageManager::GetMapHeight(); if (mapH > 0) { float wrappedY = fmod(cY, (float)mapH); if (wrappedY < 0) wrappedY += (float)mapH; cY = wrappedY; } }
    Gdiplus::Graphics g(hMemDC); StageManager::Render(hMemDC, &g, m_isFullMapView, m_showDebugRect, mapScale, m_renderMapScale, m_mapOffsetX, m_mapOffsetY, cX, cY, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, m_player.GetIsSlowMo());
    if (m_showDebugRect) { TCHAR szFps[32]; wsprintf(szFps, TEXT("FPS: %d"), m_fps); SetTextColor(hMemDC, RGB(255, 255, 0)); SetBkMode(hMemDC, TRANSPARENT); TextOut(hMemDC, 10, 10, szFps, lstrlen(szFps)); }
    for (auto& e : m_enemies) if (e) e->Render(hMemDC, &g, cX, cY, mapScale, m_showDebugRect, m_player.GetIsSlowMo());
    Bullet::RenderAll(hMemDC, cX, cY, mapScale); if (m_player.GetIsSlowMo() && m_gameMode != GameMode::REPLAYING) { Gdiplus::Rect fr(0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT); Gdiplus::GraphicsPath p; p.AddRectangle(fr); Gdiplus::PathGradientBrush pgb(&p); pgb.SetCenterColor(Gdiplus::Color(0, 0, 0, 0)); pgb.SetCenterPoint(Gdiplus::PointF(VIRTUAL_WIDTH / 2.0f, VIRTUAL_HEIGHT / 2.0f)); Gdiplus::Color ec[] = { Gdiplus::Color(180, 0, 0, 0) }; int cnt = 1; pgb.SetSurroundColors(ec, &cnt); pgb.SetFocusScales(0.2f, 0.2f); g.FillRectangle(&pgb, fr); }
    if (m_stageTimer <= 3.0f && m_bGameStarted && !m_player.IsRewinding() && m_gameMode != GameMode::REPLAYING && !m_player.IsDeathAnimationFinished()) { float alphaRatio = 1.0f - (m_stageTimer / 3.0f); if (alphaRatio < 0) alphaRatio = 0; if (alphaRatio > 1.0f) alphaRatio = 1.0f; int alpha = m_player.IsDead() ? 200 : (int)(alphaRatio * 150); Gdiplus::SolidBrush warningBrush(Gdiplus::Color(alpha, 255, 50, 200)); g.FillRectangle(&warningBrush, 0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT); }
    float fW = (float)VIRTUAL_WIDTH, fH = (float)VIRTUAL_HEIGHT, fCW = (float)StageManager::GetMapWidth(), fCH = (float)StageManager::GetMapHeight(), scX = fW / fCW, scY = fH / fCH, cFS = (scX < scY) ? scX : scY, cFX = (fW - fCW * cFS) / 2.0f, cFY = (fH - fCH * cFS) / 2.0f;
    EffectManager::Render(hMemDC, cX, cY, mapScale, m_isFullMapView, cFS, cFX, cFY); if (StartScene::ShouldShowInGamePlayer()) m_player.Render(hMemDC, &g, cX, cY, mapScale, playerScale, m_renderMapScale, m_mapOffsetX, m_mapOffsetY, m_isFullMapView, m_showDebugRect, m_stageTimer);
    auto drawTransition = [&](HDC dc) { if (m_transitionState == TransitionState::NONE) return; HBRUSH hBlack = CreateSolidBrush(RGB(0, 0, 0)); RECT tr = { 0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT }; if (m_transitionState == TransitionState::ENTERING || m_transitionState == TransitionState::WAITING) tr.left = (int)(VIRTUAL_WIDTH * (1.0f - m_transitionProgress)); else if (m_transitionState == TransitionState::LEAVING) tr.right = (int)(VIRTUAL_WIDTH * (1.0f - m_transitionProgress)); FillRect(dc, &tr, hBlack); DeleteObject(hBlack); };
    if (m_gameMode == GameMode::REPLAYING || m_gameMode == GameMode::YES_SCENE) {
        HDC hPostDC = CreateCompatibleDC(hDC); HBITMAP hPostBmp = CreateCompatibleBitmap(hDC, VIRTUAL_WIDTH, VIRTUAL_HEIGHT); HBITMAP hOldPostBmp = (HBITMAP)SelectObject(hPostDC, hPostBmp); Gdiplus::Graphics g(hPostDC); Gdiplus::Bitmap bmp(hMemBmp, NULL); Gdiplus::ImageAttributes attr; Gdiplus::ColorMatrix mat = { 0.3f, 0.3f, 0.3f, 0, 0, 0.59f, 0.59f, 0.59f, 0, 0, 0.11f, 0.11f, 0.11f, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1 }; attr.SetColorMatrix(&mat); g.DrawImage(&bmp, Gdiplus::Rect(0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT), 0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, Gdiplus::UnitPixel, &attr);
        if (m_gameMode == GameMode::REPLAYING) { if (!m_imgReplayUI[0].IsNull()) { int w = m_imgReplayUI[0].GetWidth(), h = m_imgReplayUI[0].GetHeight(); m_imgReplayUI[0].Draw(hPostDC, VIRTUAL_WIDTH - w - 20, VIRTUAL_HEIGHT - h - 20, w, h); } int topUIIdx = m_isReplayPaused ? 2 : 1; if (!m_imgReplayUI[topUIIdx].IsNull()) { int w = m_imgReplayUI[topUIIdx].GetWidth(), h = m_imgReplayUI[topUIIdx].GetHeight(); m_imgReplayUI[topUIIdx].Draw(hPostDC, 20, 20, w, h); } } else if (m_gameMode == GameMode::YES_SCENE) { DWORD elapsed = GetTickCount() - m_yesSceneStartTime; int alpha = 255; if (elapsed > 2000) { alpha = 255 - (int)((elapsed - 2000) / 500.0f * 255); if (alpha < 0) alpha = 0; } if (alpha > 0) { Gdiplus::SolidBrush blackBrush(Gdiplus::Color(alpha, 0, 0, 0)); g.FillRectangle(&blackBrush, 0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT); if (!m_imgReplayUI[3].IsNull()) { int w = m_imgReplayUI[3].GetWidth() / 2, h = m_imgReplayUI[3].GetHeight() / 2; Gdiplus::ImageAttributes yesAttr; Gdiplus::ColorMatrix yesMat = { 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, alpha / 255.0f, 0, 0, 0, 0, 0, 1 }; yesAttr.SetColorMatrix(&yesMat); HBITMAP hBmpYes = m_imgReplayUI[3]; Gdiplus::Bitmap bmpYes(hBmpYes, NULL); int x = (VIRTUAL_WIDTH - w) / 2, y = (VIRTUAL_HEIGHT - h) / 2; g.DrawImage(&bmpYes, Gdiplus::Rect(x, y, w, h), 0, 0, bmpYes.GetWidth(), bmpYes.GetHeight(), Gdiplus::UnitPixel, &yesAttr); } } }
        drawTransition(hPostDC); SetStretchBltMode(hDC, HALFTONE); StretchBlt(hDC, 0, 0, m_winWidth, m_winHeight, hPostDC, 0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, SRCCOPY); SelectObject(hPostDC, hOldPostBmp); DeleteObject(hPostBmp); DeleteDC(hPostDC);
    } else {
        if (!m_player.IsRewinding()) { 
            bool isShift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0; 
            if (!StartScene::IsActive()) {
                UIManager::Render(hMemDC, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, Input::GetMouseX(), Input::GetMouseY(), m_player.GetBatteryLevel(), m_stageTimer, StageManager::GetStageLimitTime(), m_bGameStarted, isShift, m_player.IsDead(), m_isTimeoutDeath, m_player.IsDeathAnimationFinished(), m_isStageCleared, m_currentStage, m_player.GetHeldItemType());
            }
            if (StartScene::IsActive()) StartScene::Render(hMemDC, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, m_player.GetX(), m_player.GetY(), m_player.GetColW(), m_player.GetColH(), mapScale); 
        }
        drawTransition(hMemDC); SetStretchBltMode(hDC, HALFTONE); StretchBlt(hDC, 0, 0, m_winWidth, m_winHeight, hMemDC, 0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, SRCCOPY);
    }
    SelectObject(hMemDC, hOldBmp); DeleteObject(hMemBmp); DeleteDC(hMemDC);
}

void Game::UpdateScreenScale() { m_renderMapScale = mapScale; m_renderPlayerScale = playerScale; m_mapOffsetX = 0.0f; m_mapOffsetY = 0.0f; int mapW = StageManager::GetMapWidth(), mapH = StageManager::GetMapHeight(); if (m_isFullMapView && mapW > 0) { float sX = (float)VIRTUAL_WIDTH / mapW, sY = (float)VIRTUAL_HEIGHT / mapH; m_renderMapScale = (sX < sY) ? sX : sY; m_renderPlayerScale = playerScale * (m_renderMapScale / mapScale); m_mapOffsetX = (VIRTUAL_WIDTH - (mapW * m_renderMapScale)) / 2.0f; m_mapOffsetY = (VIRTUAL_HEIGHT - (mapH * m_renderMapScale)) / 2.0f; } }
