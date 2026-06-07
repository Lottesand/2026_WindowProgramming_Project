#pragma once
#include <windows.h>
#include <atlimage.h>

struct GoUIConfig {
    int x, y;
    int arrowOffset;
};

class UIManager {
public:
    static void Init();
    static void LoadAssets();
    static void ReleaseAssets();
    static void Render(HDC hDC, int virtualWidth, int virtualHeight, int mouseX, int mouseY, float batteryLevel, float stageTimer, float stageLimitTime, bool bGameStarted, bool isShiftPressed, bool isDead, bool isTimeout = false, bool showDeathMessage = false, bool isStageCleared = false, int currentStage = 1);

    static void SetGoConfig(int stage, int x, int y, int arrowOffset);

private:
    static CImage m_imgHudBase;
    static CImage m_imgHudBattery;
    static CImage m_imgHudBatteryPart;
    static CImage m_imgHudBatteryUsed;
    static CImage m_imgHudTimer;
    static CImage m_imgHudTimerGauge;
    static CImage m_imgHudInven;
    static CImage m_imgCursor;
    static CImage m_imgHudShift[2];
    static CImage m_imgLeftClick;
    static CImage m_imgRightClick;
    static CImage m_imgDeathBox;
    static CImage m_imgTimeoutBox;

    static CImage m_imgGoText;
    static CImage m_imgGoArrow;
    static GoUIConfig m_goConfigs[5]; // 1-based indexing for 4 stages
    static int m_goAnimFrame;
    static DWORD m_lastGoAnimTime;
};

