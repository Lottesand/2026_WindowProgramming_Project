#pragma once
#include <windows.h>
#include <vector>
#include "../Objects/Player.h"
#include "../Objects/Enemy.h"

class Game {
public:
    Game();
    ~Game();

    void Init(HWND hWnd, HINSTANCE hInst);
    void Update();
    void Render(HDC hDC);

    void SpawnEnemies();

    int GetWinWidth() const { return m_winWidth; }
    int GetWinHeight() const { return m_winHeight; }
    void SetWinSize(int w, int h) { m_winWidth = w; m_winHeight = h; }

private:
    void UpdateScreenScale();

    HWND m_hWnd;
    HINSTANCE m_hInst;

    Player m_player;
    std::vector<Enemy*> m_enemies;

    int m_winWidth;
    int m_winHeight;

    bool m_isTimePaused;
    bool m_showDebugRect;
    bool m_showGrid;
    bool m_isFullMapView;

    float m_renderMapScale;
    float m_renderPlayerScale;
    float m_mapOffsetX;
    float m_mapOffsetY;

    DWORD m_prevTime;

    int m_maxRewindTime = 10; 
    int m_rewindSpeed = 4;    

    static constexpr int VIRTUAL_WIDTH = 1280;
    static constexpr int VIRTUAL_HEIGHT = 720;
    static constexpr float mapScale = 1.02f;
    static constexpr float playerScale = 2.0f;
};

