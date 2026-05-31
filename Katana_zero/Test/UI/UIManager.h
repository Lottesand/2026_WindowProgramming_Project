#pragma once
#include <windows.h>
#include <atlimage.h>

class UIManager {
public:
    static void Init();
    static void LoadAssets();
    static void ReleaseAssets();
    static void Render(HDC hDC, int virtualWidth, int virtualHeight, int mouseX, int mouseY, float batteryLevel);

private:
    static CImage m_imgHudBase;
    static CImage m_imgHudBattery;
    static CImage m_imgHudBatteryPart;
    static CImage m_imgHudBatteryUsed;
    static CImage m_imgHudTimer;
    static CImage m_imgHudInven;
    static CImage m_imgCursor;
};

