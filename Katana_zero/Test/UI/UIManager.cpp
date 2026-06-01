#include "UIManager.h"
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

void UIManager::Init() {
}

void UIManager::LoadAssets() {
    m_imgHudBase.Load(TEXT("assets/hud/base.png"));
    m_imgHudBattery.Load(TEXT("assets/hud/battery.png"));
    m_imgHudBatteryPart.Load(TEXT("assets/hud/battery_part.png"));
    m_imgHudBatteryUsed.Load(TEXT("assets/hud/used_battery.png"));
    m_imgHudTimer.Load(TEXT("assets/hud/timer.png"));
    m_imgHudTimerGauge.Load(TEXT("assets/hud/timer_gauge.png"));
    m_imgHudInven.Load(TEXT("assets/hud/inven.png"));
    m_imgCursor.Load(TEXT("assets/cursor.png"));
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
}
void UIManager::Render(HDC hDC, int virtualWidth, int virtualHeight, int mouseX, int mouseY, float batteryLevel, float stageTimer, float stageLimitTime, bool bGameStarted) {
    if (!hDC) return;

    // 게임 시작 전 메시지 출력
    if (!bGameStarted) {
        SetBkMode(hDC, TRANSPARENT);
        SetTextColor(hDC, RGB(255, 255, 255));
        
        HFONT hFont = CreateFont(40, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
            CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH | FF_SWISS, TEXT("Arial"));
        HFONT hOldFont = (HFONT)SelectObject(hDC, hFont);

        RECT rect = { 0, 0, virtualWidth, virtualHeight };
        DrawText(hDC, TEXT("Left Click to Start"), -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        SelectObject(hDC, hOldFont);
        DeleteObject(hFont);
    }

    // HUD 요소 렌더링
    if (!m_imgHudBase.IsNull())
        m_imgHudBase.Draw(hDC, 0, 0, m_imgHudBase.GetWidth() * 2, m_imgHudBase.GetHeight() * 2);

    if (!m_imgHudBattery.IsNull())
        m_imgHudBattery.Draw(hDC, 10, 4, m_imgHudBattery.GetWidth() * 2, m_imgHudBattery.GetHeight() * 2);

    // 배터리 칸 렌더링
    int startX = 32; 
    int startY = 10;
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

    if (!m_imgHudTimer.IsNull()) {
        float timerScale = 2.0f; // 프레임은 원래대로 (2.0)
        int timerW = (int)(m_imgHudTimer.GetWidth() * timerScale);
        int timerH = (int)(m_imgHudTimer.GetHeight() * timerScale);
        int timerX = (virtualWidth / 2 - timerW / 2); // 중앙 정렬
        int timerY = 0; 
        
        // 프레임을 먼저 그립니다
        m_imgHudTimer.Draw(hDC, timerX, timerY, timerW, timerH);

        // [타이머 게이지 로직] 
        float timeRatio = stageTimer / (stageLimitTime > 0 ? stageLimitTime : 1.0f);
        if (timeRatio < 0) timeRatio = 0;
        if (timeRatio > 1.0f) timeRatio = 1.0f;

        // 게이지가 프레임 내부에서 차지하는 실제 픽셀 영역
        int gaugeMarginX = 10; // 좌우 여백
        int gaugeMarginY = 8;  // 상하 여백
        int gaugeW = timerW - (gaugeMarginX * 2) - 15;
        int gaugeH = timerH - (gaugeMarginY * 2);
        int gaugeX = timerX + gaugeMarginX + 23;
        int gaugeY = timerY + gaugeMarginY - 4;

        // 게이지 이미지(timer_gauge.png)를 프레임 위에 그립니다 (남은 시간에 비례해서 출력)
        if (!m_imgHudTimerGauge.IsNull()) {
            int currentGaugeW = (int)(gaugeW * timeRatio);
            int srcW = (int)(m_imgHudTimerGauge.GetWidth() * timeRatio);
            
            // CImage::Draw의 assertion 오류를 방지하기 위해 너비가 0보다 클 때만 그립니다.
            if (currentGaugeW > 0 && srcW > 0) {
                m_imgHudTimerGauge.Draw(hDC, gaugeX, gaugeY, currentGaugeW, gaugeH,
                    0, 0, srcW, m_imgHudTimerGauge.GetHeight());
            }
        }
    }

    if (!m_imgHudInven.IsNull())
        m_imgHudInven.Draw(hDC, virtualWidth - m_imgHudInven.GetWidth() - 80, 0, m_imgHudInven.GetWidth() * 2, m_imgHudInven.GetHeight() * 2);

    if (!m_imgCursor.IsNull()) 
        m_imgCursor.Draw(hDC, mouseX - m_imgCursor.GetWidth(), mouseY - m_imgCursor.GetHeight(), m_imgCursor.GetWidth() * 2, m_imgCursor.GetHeight() * 2);
}


