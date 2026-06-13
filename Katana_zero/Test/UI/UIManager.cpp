#include "UIManager.h"
#include "../Objects/Enemy.h"
#include "../Objects/Item.h"
#include <gdiplus.h>
#include <algorithm>

CImage UIManager::m_imgHudBase;
CImage UIManager::m_imgHudBattery;
CImage UIManager::m_imgHudBatteryPart;
CImage UIManager::m_imgHudBatteryUsed;
CImage UIManager::m_imgHudTimer;
CImage UIManager::m_imgHudTimerGauge;
CImage UIManager::m_imgHudInven;
CImage UIManager::m_imgCursor;
CImage UIManager::m_imgDeathBox;
CImage UIManager::m_imgTimeoutBox;
CImage UIManager::m_imgGoArrow;
CImage UIManager::m_imgGoText;
CImage UIManager::m_imgHudShift[2];
CImage UIManager::m_imgLeftClick;
CImage UIManager::m_imgRightClick;

void UIManager::LoadAssets() {
    if (!m_imgHudBase.IsNull()) return;
    m_imgHudBase.Load(TEXT("assets/hud/base.png"));
    m_imgHudBattery.Load(TEXT("assets/hud/battery.png"));
    m_imgHudBatteryPart.Load(TEXT("assets/hud/battery_part.png"));
    m_imgHudBatteryUsed.Load(TEXT("assets/hud/battery_used.png"));
    m_imgHudTimer.Load(TEXT("assets/hud/timer.png"));
    m_imgHudTimerGauge.Load(TEXT("assets/hud/timer_gauge.png"));
    m_imgHudInven.Load(TEXT("assets/hud/inven_0.png"));
    m_imgCursor.Load(TEXT("assets/cursor.png"));
    m_imgDeathBox.Load(TEXT("assets/deathbox.png"));
    m_imgTimeoutBox.Load(TEXT("assets/timeoutbox.png"));
    m_imgGoArrow.Load(TEXT("assets/spr_go_arrow.png"));
    m_imgGoText.Load(TEXT("assets/spr_go_text.png"));
    m_imgHudShift[0].Load(TEXT("assets/hud/shift_0.png"));
    m_imgHudShift[1].Load(TEXT("assets/hud/shift_1.png"));
    m_imgLeftClick.Load(TEXT("assets/hud/left_click.png"));
    m_imgRightClick.Load(TEXT("assets/hud/right_click.png"));
}

void UIManager::ReleaseAssets() {
    m_imgHudBase.Destroy();
    m_imgHudBattery.Destroy();
    m_imgHudBatteryPart.Destroy();
    m_imgHudBatteryUsed.Destroy();
    m_imgHudTimer.Destroy();
    m_imgHudTimerGauge.Destroy();
    m_imgHudInven.Destroy();
    m_imgCursor.Destroy();
    m_imgDeathBox.Destroy();
    m_imgTimeoutBox.Destroy();
    m_imgGoArrow.Destroy();
    m_imgGoText.Destroy();
    m_imgHudShift[0].Destroy();
    m_imgHudShift[1].Destroy();
    m_imgLeftClick.Destroy();
    m_imgRightClick.Destroy();
}

void UIManager::Init() {}

void UIManager::Render(HDC hDC, int virtualWidth, int virtualHeight, int mouseX, int mouseY, float battery, float stageTimer, float maxTimer, bool gameStarted, bool isShift, bool isDead, bool isTimeout, bool isAnimFinished, bool isStageCleared, int currentStage, int heldItemType) {
    if (!gameStarted) return;
    Gdiplus::Graphics g(hDC);

    // 1. HUD Base
    if (!m_imgHudBase.IsNull()) m_imgHudBase.Draw(hDC, 20, 20);

    // 2. Battery
    if (!m_imgHudBattery.IsNull()) {
        m_imgHudBattery.Draw(hDC, 44, 40);
        int partW = m_imgHudBatteryPart.GetWidth();
        int totalParts = 11;
        int activeParts = (int)(battery * totalParts);
        for (int i = 0; i < totalParts; i++) {
            int px = 55 + i * (partW + 1);
            if (i < activeParts) m_imgHudBatteryPart.Draw(hDC, px, 43);
            else m_imgHudBatteryUsed.Draw(hDC, px, 43);
        }
    }

    // 3. Timer
    if (!m_imgHudTimer.IsNull()) {
        int tx = 20, ty = 85;
        m_imgHudTimer.Draw(hDC, tx, ty);
        if (!m_imgHudTimerGauge.IsNull()) {
            float ratio = stageTimer / maxTimer;
            int gw = m_imgHudTimerGauge.GetWidth();
            int gh = m_imgHudTimerGauge.GetHeight();
            int currentGW = (int)(gw * ratio);
            if (currentGW > 0) m_imgHudTimerGauge.Draw(hDC, tx + 24, ty + 5, currentGW, gh, 0, 0, currentGW, gh);
        }
    }

    // 4. Inventory
    if (!m_imgHudInven.IsNull()) {
        int ix = 20, iy = 120;
        m_imgHudInven.Draw(hDC, ix, iy);
        if (heldItemType != -1) {
            CImage& heldImg = Item::GetHUDImage(static_cast<ItemType>(heldItemType));
            if (!heldImg.IsNull()) {
                int iw = 30, ih = 30;
                heldImg.Draw(hDC, ix + 10, iy + 10, iw, ih);
            }
        }
    }

    // 5. Shift Indicator
    CImage& shiftImg = m_imgHudShift[isShift ? 1 : 0];
    if (!shiftImg.IsNull()) shiftImg.Draw(hDC, 180, 35);

    // 6. Death Box
    if (isDead && isAnimFinished) {
        CImage& box = isTimeout ? m_imgTimeoutBox : m_imgDeathBox;
        if (!box.IsNull()) {
            int bw = box.GetWidth() * 2, bh = box.GetHeight() * 2;
            box.Draw(hDC, (virtualWidth - bw) / 2, (virtualHeight - bh) / 2, bw, bh);
        }
    }

    // 7. Stage Clear "GO"
    if (isStageCleared) {
        if (!m_imgGoText.IsNull()) {
            int gw = m_imgGoText.GetWidth() * 2, gh = m_imgGoText.GetHeight() * 2;
            m_imgGoText.Draw(hDC, (virtualWidth - gw) / 2, 100, gw, gh);
        }
        if (!m_imgGoArrow.IsNull()) {
            int aw = m_imgGoArrow.GetWidth() * 2, ah = m_imgGoArrow.GetHeight() * 2;
            int animOffset = (GetTickCount() / 200) % 2 * 10;
            m_imgGoArrow.Draw(hDC, virtualWidth - aw - 50 + animOffset, (virtualHeight - ah) / 2, aw, ah);
        }
    }

    // 8. Cursor
    if (!m_imgCursor.IsNull()) m_imgCursor.Draw(hDC, mouseX - m_imgCursor.GetWidth(), mouseY - m_imgCursor.GetHeight(), m_imgCursor.GetWidth() * 2, m_imgCursor.GetHeight() * 2);
}
