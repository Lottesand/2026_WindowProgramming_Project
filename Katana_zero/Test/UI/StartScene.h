#pragma once
#include <windows.h>
#include <atlimage.h>
#include <vector>
#include <gdiplus.h>

class StartScene {
public:
    enum class State {
        WAIT_CAMERA,
        PLAYER_ANIM,
        BOX_EXPAND,
        PNG_FADE_IN,
        OVERLAY_FADE,
        TITLE_MERGE,
        WAIT_CLICK,
        ENDING
    };

    static void Init();
    static void LoadAssets();
    static void LoadAllSounds();
    static void ReleaseAssets();
    
    static void Reset();
    static void Update(float dT, bool& bGameStarted);
    static void Render(HDC hDC, int virtualWidth, int virtualHeight, float playerX, float playerY, float playerColW, float playerColH, float mapScale);

    static bool IsActive() { return m_active; }
    static bool ShouldShowInGamePlayer() { return !m_active || m_state >= State::WAIT_CLICK; }
    static void SetActive(bool active) { m_active = active; if (active) Reset(); }

private:
    static bool m_active;
    static State m_state;
    static float m_timer;
    
    static CImage m_imgPlayerPlay[31];
    static CImage m_imgPlaySong;
    static CImage m_imgSongName;
    static CImage m_imgMansionTitle;
    static CImage m_imgAttackStart;
    static CImage m_imgLeftClick;

    static int m_playerFrame;
    static float m_boxWidthProgress;
    static float m_pngAlpha;
    static float m_overlayAlpha;
    static float m_titleProgress;
    static bool m_showFinalPrompts;

    static constexpr int VIRTUAL_WIDTH = 1280;
    static constexpr int VIRTUAL_HEIGHT = 720;
};
