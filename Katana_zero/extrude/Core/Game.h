#pragma once
#include <windows.h>
#include <vector>
#include <atomic>
#include "../Objects/Player.h"
#include "../Objects/Enemy.h"

enum class GameMode { PLAYING, YES_SCENE, REPLAYING };
enum class TransitionState { NONE, ENTERING, WAITING, LEAVING };

class Game {
public:
    Game();

    ~Game();

    void Init(HWND hWnd, HINSTANCE hInst);
    void Update();
    void Render(HDC hDC);

    void SpawnEnemies();
    void LoadStage(int stage);

    int GetWinWidth() const { return m_winWidth; }
    int GetWinHeight() const { return m_winHeight; }
    void SetWinSize(int w, int h) { m_winWidth = w; m_winHeight = h; }

    // Loading system
    static void LoadingThreadProc(Game* pGame);
    void LoadAllAssets();
    bool IsLoaded() const { return m_isLoaded; }
    int GetLoadingProgress() const { return m_loadingProgress; }

private:
    void UpdateScreenScale();

    HWND m_hWnd;
    HINSTANCE m_hInst;

    Player m_player;
    std::vector<Enemy*> m_enemies;

    int m_winWidth;
    int m_winHeight;
    int m_currentStage;
    bool m_isStageCleared;
    bool m_bGameStarted;

    // Loading status
    std::atomic<bool> m_isLoaded{ false };
    std::atomic<int> m_loadingProgress{ 0 };
    CImage m_imgLoading;

    bool m_isTimePaused;
    bool m_isTimeoutDeath;
    bool m_showDebugRect;
    bool m_showGrid;
    bool m_isFullMapView;

    float m_renderMapScale;
    float m_renderPlayerScale;
    float m_mapOffsetX;
    float m_mapOffsetY;

    DWORD m_prevTime;
    float m_stageTimer; // 현재 스테이지 남은 시간

    GameMode m_gameMode = GameMode::PLAYING;
    int m_replayFrame = 0;
    int m_replaySpeed = 2; // 리플레이 재생 속도 (프레임 스킵/배속)
    bool m_isReplayPaused = false;
    CImage m_imgReplayUI[4];
    DWORD m_yesSceneStartTime = 0;
    
    TransitionState m_transitionState = TransitionState::NONE;
    float m_transitionProgress = 0.0f;
    DWORD m_transitionWaitTime = 0;
    bool m_transitionToNextStage = false;

    int m_maxRewindTime = 10; 
    int m_rewindSpeed = 4;    
    int m_initialRewindHistorySize = 0;

    int m_fps = 0;
    int m_frameCount = 0;
    DWORD m_lastFpsTime = 0;

    static constexpr int VIRTUAL_WIDTH = 1280;
    static constexpr int VIRTUAL_HEIGHT = 720;
    float mapScale = 1.02f;
    float playerScale = 2.0f;
};

