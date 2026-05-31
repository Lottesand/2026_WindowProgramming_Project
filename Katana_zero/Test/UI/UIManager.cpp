#include "UIManager.h"
#include <gdiplus.h>
#include <algorithm>

CImage UIManager::m_imgHudBase;
CImage UIManager::m_imgHudBattery;
CImage UIManager::m_imgHudBatteryPart;
CImage UIManager::m_imgHudBatteryUsed;
CImage UIManager::m_imgHudTimer;
CImage UIManager::m_imgHudInven;
CImage UIManager::m_imgCursor;

void UIManager::Init() {
}

void UIManager::LoadAssets() {
    m_imgHudBase.Load(TEXT("assets/hud/base.png"));
    m_imgHudBattery.Load(TEXT("assets/hud/battery.png"));
    m_imgHudBatteryPart.Load(TEXT("assets/hud/battery_part.png"));
    m_imgHudBatteryUsed.Load(TEXT("assets/hud/used_battery.png"));
    m_imgHudTimer.Load(TEXT("assets/hud/timer.png"));
    m_imgHudInven.Load(TEXT("assets/hud/inven.png"));
    m_imgCursor.Load(TEXT("assets/cursor.png"));
}

void UIManager::ReleaseAssets() {
    m_imgHudBase.Destroy();
    m_imgHudBattery.Destroy();
    m_imgHudBatteryPart.Destroy();
    m_imgHudBatteryUsed.Destroy();
    m_imgHudTimer.Destroy();
    m_imgHudInven.Destroy();
    m_imgCursor.Destroy();
}
void UIManager::Render(HDC hDC, int virtualWidth, int virtualHeight, int mouseX, int mouseY, float batteryLevel) {
    if (!hDC) return;

    // HUD ?붿냼 罹먯뒰 蹂??
    if (!m_imgHudBase.IsNull())
        m_imgHudBase.Draw(hDC, 0, 0, m_imgHudBase.GetWidth() * 2, m_imgHudBase.GetHeight() * 2);

    if (!m_imgHudBattery.IsNull())
        m_imgHudBattery.Draw(hDC, 10, 9.5, m_imgHudBattery.GetWidth() * 2, m_imgHudBattery.GetHeight() * 2);

    // 諛고꽣由?移??뚮뜑留?
    int startX = 32; 
    int startY = 14;
    int gap = 10;    
    float partScale = 2.2f; 

    for (int i = 0; i < 11; i++) {
        CImage* targetImg = &m_imgHudBatteryPart;
        if (i >= (int)batteryLevel) {
            targetImg = &m_imgHudBatteryUsed;
        }

        if (targetImg && !targetImg->IsNull()) {
            targetImg->Draw(hDC, 
                (int)(startX + i * gap), 
                startY, 
                (int)(targetImg->GetWidth() * partScale), 
                (int)(targetImg->GetHeight() * partScale));
        }
    }

    if (!m_imgHudTimer.IsNull()) 
        m_imgHudTimer.Draw(hDC, (virtualWidth / 2 - m_imgHudTimer.GetWidth() - 10), 0, m_imgHudTimer.GetWidth() * 2, m_imgHudTimer.GetHeight() * 2);

    if (!m_imgHudInven.IsNull())
        m_imgHudInven.Draw(hDC, virtualWidth - m_imgHudInven.GetWidth() - 80, 0, m_imgHudInven.GetWidth() * 2, m_imgHudInven.GetHeight() * 2);

    if (!m_imgCursor.IsNull()) 
        m_imgCursor.Draw(hDC, mouseX - m_imgCursor.GetWidth(), mouseY - m_imgCursor.GetHeight(), m_imgCursor.GetWidth() * 2, m_imgCursor.GetHeight() * 2);
}


