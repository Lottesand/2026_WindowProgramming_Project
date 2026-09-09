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
    m_imgHudBatteryUsed.Load(TEXT("assets/hud/used_battery.png"));
    m_imgHudTimer.Load(TEXT("assets/hud/timer.png"));
    m_imgHudTimerGauge.Load(TEXT("assets/hud/timer_gauge.png"));
    m_imgHudInven.Load(TEXT("assets/hud/inven.png"));
    m_imgCursor.Load(TEXT("assets/cursor.png"));
    m_imgDeathBox.Load(TEXT("assets/deathbox.png"));
    m_imgTimeoutBox.Load(TEXT("assets/timeoutbox.png"));
    m_imgGoArrow.Load(TEXT("assets/spr_go_arrow.png"));
    m_imgGoText.Load(TEXT("assets/spr_go_text.png"));
    m_imgHudShift[0].Load(TEXT("assets/hud/keyboard_shift_0.png"));
    m_imgHudShift[1].Load(TEXT("assets/hud/keyboard_shift_1.png"));
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

GoUIConfig UIManager::m_goConfigs[5];
int UIManager::m_goAnimFrame = 0;
DWORD UIManager::m_lastGoAnimTime = 0;

void UIManager::Init() {
    // Stage-specific GO UI Configurations (approximate door locations)
    SetGoConfig(1, 1100, 360, 0);
    SetGoConfig(2, 1150, 360, 0);
    SetGoConfig(3, 1100, 360, 0);
    SetGoConfig(4, 1150, 360, 0);
}

void UIManager::SetGoConfig(int stage, int x, int y, int arrowOffset) {
    if (stage >= 1 && stage <= 4) {
        m_goConfigs[stage] = { x, y, arrowOffset };
    }
}

void UIManager::RenderLeftClickPrompt(HDC hDC, float worldX, float worldY, float camX, float camY, float mapScale) {
    if (m_imgLeftClick.IsNull()) return;
    int w = m_imgLeftClick.GetWidth() * 2;
    int h = m_imgLeftClick.GetHeight() * 2;
    int dx = (int)((worldX - camX) * mapScale) - w / 2;
    int dy = (int)((worldY - camY) * mapScale) - h;
    
    // Bobbing effect
    dy += (int)(sin(GetTickCount() * 0.005f) * 5.0f);
    
    m_imgLeftClick.Draw(hDC, dx, dy, w, h);
}

void UIManager::Render(HDC hDC, int virtualWidth, int virtualHeight, int mouseX, int mouseY, float batteryLevel, float stageTimer, float stageLimitTime, bool bGameStarted, bool isShiftPressed, bool isDead, bool isTimeout, bool showDeathMessage, bool isStageCleared, int currentStage, int heldItemType) {
    if (!hDC) return;

    if (isDead && showDeathMessage) {
        CImage* boxImg = isTimeout ? &m_imgTimeoutBox : &m_imgDeathBox;
        if (boxImg && !boxImg->IsNull()) {
            float baseW = boxImg->GetWidth();
            float baseH = boxImg->GetHeight();
            float scale = 0.65f;
            int targetW = (int)(baseW * scale);
            int targetH = (int)(baseH * scale);
            int startX = (virtualWidth - targetW) / 2;
            int startY = (virtualHeight - targetH) / 2;

            Gdiplus::Graphics graphics(hDC);
            graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

            Gdiplus::GraphicsPath path;
            Gdiplus::Rect gradientRect(startX - 40, startY - 20, targetW + 80, targetH + 40);
            path.AddRectangle(gradientRect);

            Gdiplus::PathGradientBrush pgb(&path);
            pgb.SetCenterColor(Gdiplus::Color(230, 0, 0, 0));
            pgb.SetCenterPoint(Gdiplus::PointF(virtualWidth / 2.0f, virtualHeight / 2.0f));
            
            Gdiplus::Color colors[] = { Gdiplus::Color(0, 0, 0, 0) }; 
            int count = 1;
            pgb.SetSurroundColors(colors, &count);
            pgb.SetFocusScales(0.3f, 0.3f); 

            graphics.FillRectangle(&pgb, gradientRect);

            if (boxImg->IsDIBSection()) {
                void* bits = boxImg->GetBits();
                if (bits) {
                    Gdiplus::Bitmap bitmap(boxImg->GetWidth(), boxImg->GetHeight(), boxImg->GetPitch(), PixelFormat32bppARGB, (BYTE*)bits);
                    Gdiplus::ImageAttributes attr;
                    attr.SetColorKey(Gdiplus::Color(0, 0, 0), Gdiplus::Color(20, 20, 20), Gdiplus::ColorAdjustTypeBitmap);
                    graphics.DrawImage(&bitmap, Gdiplus::Rect(startX, startY, targetW, targetH), 
                        0, 0, boxImg->GetWidth(), boxImg->GetHeight(), Gdiplus::UnitPixel, &attr);
                }
            } else {
                boxImg->Draw(hDC, startX, startY, targetW, targetH);
            }
        }
    }

    if (!m_imgHudBase.IsNull() && m_imgHudBase.GetWidth() > 0 && m_imgHudBase.GetHeight() > 0)
        m_imgHudBase.Draw(hDC, 0, 0, m_imgHudBase.GetWidth() * 2, m_imgHudBase.GetHeight() * 2);

    if (!m_imgHudBattery.IsNull() && m_imgHudBattery.GetWidth() > 0 && m_imgHudBattery.GetHeight() > 0)
        m_imgHudBattery.Draw(hDC, 10, 4, m_imgHudBattery.GetWidth() * 2, m_imgHudBattery.GetHeight() * 2);

    // Shift Indicator
    int shiftIconX = 165; 
    int shiftIconY = 7;   
    float shiftScale = 2.0f; 
    CImage* imgShift = isShiftPressed ? &m_imgHudShift[1] : &m_imgHudShift[0];
    if (imgShift && !imgShift->IsNull() && imgShift->GetWidth() > 0 && imgShift->GetHeight() > 0) {
        imgShift->Draw(hDC, shiftIconX, shiftIconY, 
            (int)(imgShift->GetWidth() * shiftScale), (int)(imgShift->GetHeight() * shiftScale));
    }

    // Battery Parts
    int startX = 32; int startY = 12; int gap = 10; float partScale = 2.0f; 
    for (int i = 0; i < 11; i++) {
        CImage* targetImg = (i >= (int)batteryLevel) ? &m_imgHudBatteryUsed : &m_imgHudBatteryPart;
        if (targetImg && !targetImg->IsNull() && targetImg->GetWidth() > 0 && targetImg->GetHeight() > 0) {
            targetImg->Draw(hDC, (int)(startX + i * gap), startY, (int)(targetImg->GetWidth() * partScale), (int)(targetImg->GetHeight() * partScale));
        }
    }

    if (!m_imgHudTimer.IsNull() && m_imgHudTimer.GetWidth() > 0 && m_imgHudTimer.GetHeight() > 0) {
        float timerScale = 2.0f;
        int timerW = (int)(m_imgHudTimer.GetWidth() * timerScale), timerH = (int)(m_imgHudTimer.GetHeight() * timerScale);
        int timerX = (virtualWidth / 2 - timerW / 2), timerY = 0; 
        m_imgHudTimer.Draw(hDC, timerX, timerY, timerW, timerH);
        float timeRatio = stageTimer / (stageLimitTime > 0 ? stageLimitTime : 1.0f);
        if (timeRatio < 0) timeRatio = 0; if (timeRatio > 1.0f) timeRatio = 1.0f;
        int gaugeMarginX = 10, gaugeMarginY = 8;
        int gaugeW = timerW - (gaugeMarginX * 2) - 15, gaugeH = timerH - (gaugeMarginY * 2), gaugeX = timerX + gaugeMarginX + 23, gaugeY = timerY + gaugeMarginY - 4;
        if (!m_imgHudTimerGauge.IsNull() && m_imgHudTimerGauge.GetWidth() > 0 && m_imgHudTimerGauge.GetHeight() > 0) {
            int currentGaugeW = (int)(gaugeW * timeRatio), srcW = (int)(m_imgHudTimerGauge.GetWidth() * timeRatio), srcH = m_imgHudTimerGauge.GetHeight();
            if (currentGaugeW > 0 && gaugeH > 0 && srcW > 0 && srcH > 0) 
                m_imgHudTimerGauge.Draw(hDC, gaugeX, gaugeY, currentGaugeW, gaugeH, 0, 0, srcW, srcH);
        }
    }

    // Inventory
    if (!m_imgHudInven.IsNull() && m_imgHudInven.GetWidth() > 0 && m_imgHudInven.GetHeight() > 0) {
        int invX = virtualWidth - m_imgHudInven.GetWidth() * 2 - 40;
        int invY = 0;
        
        CImage* targetInvenImg = &m_imgHudInven;
        if (heldItemType != -1) {
            CImage& itemHudIcon = Item::GetHUDImage(static_cast<ItemType>(heldItemType));
            if (!itemHudIcon.IsNull()) {
                targetInvenImg = &itemHudIcon;
            }
        }
        targetInvenImg->Draw(hDC, invX, invY, targetInvenImg->GetWidth() * 2, targetInvenImg->GetHeight() * 2);
    }

    // Mouse Icons
    int mouseIconScale = 2, mouseIconY = 30, mouseIconX = virtualWidth - 110;
    if (!m_imgLeftClick.IsNull() && m_imgLeftClick.GetWidth() > 0) m_imgLeftClick.Draw(hDC, mouseIconX, mouseIconY, m_imgLeftClick.GetWidth() * mouseIconScale, m_imgLeftClick.GetHeight() * mouseIconScale);
    if (!m_imgRightClick.IsNull() && m_imgRightClick.GetWidth() > 0) m_imgRightClick.Draw(hDC, mouseIconX + 70, mouseIconY, m_imgRightClick.GetWidth() * mouseIconScale, m_imgRightClick.GetHeight() * mouseIconScale);

    // Stage Clear "GO"
    if (bGameStarted && isStageCleared) {
        float slideAnim = sin(GetTickCount() * 0.008f) * 20.0f; // Slide right and back

        if (!m_imgGoText.IsNull() && m_imgGoText.GetWidth() > 0) {
            int gw = m_imgGoText.GetWidth() * 2, gh = m_imgGoText.GetHeight() * 2;
            int tx = virtualWidth - gw - 120;
            int ty = (virtualHeight / 2) - gh; // Upper part of the pair
            
            m_imgGoText.Draw(hDC, (int)(tx + slideAnim), ty, gw, gh);
            
            if (!m_imgGoArrow.IsNull() && m_imgGoArrow.GetWidth() > 0) {
                int aw = m_imgGoArrow.GetWidth() * 2, ah = m_imgGoArrow.GetHeight() * 2;
                int ax = tx + (gw - aw) / 2;
                int ay = ty + gh + 10; // Directly below text
                m_imgGoArrow.Draw(hDC, (int)(ax + slideAnim), ay, aw, ah);
            }
        }
    }

    // Cursor
    if (!m_imgCursor.IsNull() && m_imgCursor.GetWidth() > 0 && m_imgCursor.GetHeight() > 0) 
        m_imgCursor.Draw(hDC, mouseX - m_imgCursor.GetWidth(), mouseY - m_imgCursor.GetHeight(), m_imgCursor.GetWidth() * 2, m_imgCursor.GetHeight() * 2);
}
