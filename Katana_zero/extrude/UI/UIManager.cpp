#include "UIManager.h"
#include "../Objects/Enemy.h"
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
CImage UIManager::m_imgHudShift[2];
CImage UIManager::m_imgLeftClick;
CImage UIManager::m_imgRightClick;
CImage UIManager::m_imgDeathBox;

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
    m_imgHudShift[0].Load(TEXT("assets/hud/keyboard_shift_0.png"));
    m_imgHudShift[1].Load(TEXT("assets/hud/keyboard_shift_1.png"));
    m_imgLeftClick.Load(TEXT("assets/hud/left_click.png"));
    m_imgRightClick.Load(TEXT("assets/hud/right_click.png"));
    m_imgDeathBox.Load(TEXT("assets/deathbox.png"));
}

void UIManager::ReleaseAssets() {
    Enemy::ReleaseAll();
    m_imgHudBase.Destroy();
    m_imgHudBattery.Destroy();
    m_imgHudBatteryPart.Destroy();
    m_imgHudBatteryUsed.Destroy();
    m_imgHudTimer.Destroy();
    m_imgHudTimerGauge.Destroy();
    m_imgHudInven.Destroy();
    m_imgCursor.Destroy();
    m_imgHudShift[0].Destroy();
    m_imgHudShift[1].Destroy();
    m_imgLeftClick.Destroy();
    m_imgRightClick.Destroy();
    m_imgDeathBox.Destroy();
}
void UIManager::Render(HDC hDC, int virtualWidth, int virtualHeight, int mouseX, int mouseY, float batteryLevel, float stageTimer, float stageLimitTime, bool bGameStarted, bool isShiftPressed, bool isDead) {
    if (!hDC) return;

    if (isDead) {
        if (!m_imgDeathBox.IsNull()) {
            int dbW = m_imgDeathBox.GetWidth();
            int dbH = m_imgDeathBox.GetHeight();
            // 0.75배에서 50% 줄여서 0.375배로 설정
            float scale = 0.65f; 
            int targetW = (int)(dbW * scale);
            int targetH = (int)(dbH * scale);
            int startX = (virtualWidth - targetW) / 2;
            int startY = (virtualHeight - targetH) / 2;

            Gdiplus::Graphics graphics(hDC);
            graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

            // 1. 배경 그라데이션 Rect 그리기
            Gdiplus::GraphicsPath path;
            // 배경 크기도 글자 크기에 맞춰 조정
            Gdiplus::Rect gradientRect(startX - 40, startY - 20, targetW + 80, targetH + 40);
            path.AddRectangle(gradientRect);

            Gdiplus::PathGradientBrush pgb(&path);
            pgb.SetCenterColor(Gdiplus::Color(230, 0, 0, 0)); // 중앙을 조금 더 진하게 (알파 230)
            pgb.SetCenterPoint(Gdiplus::PointF(virtualWidth / 2.0f, virtualHeight / 2.0f));
            
            Gdiplus::Color colors[] = { Gdiplus::Color(0, 0, 0, 0) }; 
            int count = 1;
            pgb.SetSurroundColors(colors, &count);
            // 안쪽에서 바깥쪽으로 진해지는(투명해지는) 비율을 높임 (중심 집중도 강화)
            pgb.SetFocusScales(0.3f, 0.3f); 

            graphics.FillRectangle(&pgb, gradientRect);

            // 2. 글자(DeathBox) 그리기
            void* bits = m_imgDeathBox.GetBits();
            if (bits) {
                Gdiplus::Bitmap bitmap(dbW, dbH, m_imgDeathBox.GetPitch(), PixelFormat32bppARGB, (BYTE*)bits);
                
                Gdiplus::ImageAttributes attr;
                attr.SetColorKey(Gdiplus::Color(0, 0, 0), Gdiplus::Color(20, 20, 20), Gdiplus::ColorAdjustTypeBitmap);

                graphics.DrawImage(&bitmap, Gdiplus::Rect(startX, startY, targetW, targetH), 
                    0, 0, dbW, dbH, Gdiplus::UnitPixel, &attr);
            }
        }
    }

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
    if (!m_imgHudBase.IsNull() && m_imgHudBase.GetWidth() > 0 && m_imgHudBase.GetHeight() > 0)
        m_imgHudBase.Draw(hDC, 0, 0, m_imgHudBase.GetWidth() * 2, m_imgHudBase.GetHeight() * 2);

    if (!m_imgHudBattery.IsNull() && m_imgHudBattery.GetWidth() > 0 && m_imgHudBattery.GetHeight() > 0)
        m_imgHudBattery.Draw(hDC, 10, 4, m_imgHudBattery.GetWidth() * 2, m_imgHudBattery.GetHeight() * 2);

    // [Shift 아이콘 렌더링]
    // battery_part가 시작되는 startX(32)보다 왼쪽이나 배터리 프레임(10,4) 옆에 배치
    int shiftIconX = 165; // 배터리 칸 끝나는 지점 근처 혹은 옆 좌표 (조정 가능)
    int shiftIconY = 7.5;   // 상하 좌표 (조정 가능)
    float shiftScale = 2.0f; // 크기 배율 (조정 가능)
    
    CImage* imgShift = isShiftPressed ? &m_imgHudShift[1] : &m_imgHudShift[0];
    if (imgShift && !imgShift->IsNull() && imgShift->GetWidth() > 0 && imgShift->GetHeight() > 0) {
        imgShift->Draw(hDC, shiftIconX, shiftIconY, 
            (int)(imgShift->GetWidth() * shiftScale), (int)(imgShift->GetHeight() * shiftScale));
    }

    // 배터리 칸 렌더링
    int startX = 32; 
    int startY = 12;
    int gap = 10;    
    float partScale = 2.0f; 

    for (int i = 0; i < 11; i++) {
        CImage* targetImg = &m_imgHudBatteryPart;
        if (i >= (int)batteryLevel) {
            targetImg = &m_imgHudBatteryUsed;
        }

        if (targetImg && !targetImg->IsNull() && targetImg->GetWidth() > 0 && targetImg->GetHeight() > 0) {
            targetImg->Draw(hDC, 
                (int)(startX + i * gap), 
                startY, 
                (int)(targetImg->GetWidth() * partScale), 
                (int)(targetImg->GetHeight() * partScale));
        }
    }

    if (!m_imgHudTimer.IsNull() && m_imgHudTimer.GetWidth() > 0 && m_imgHudTimer.GetHeight() > 0) {
        float timerScale = 2.0f; // 프레임은 원래대로 (2.0)
        int timerW = (int)(m_imgHudTimer.GetWidth() * timerScale);
        int timerH = (int)(m_imgHudTimer.GetHeight() * timerScale);
        int timerX = (virtualWidth / 2 - timerW / 2); // 중앙 정렬
        int timerY = 0; 
        
        // 프레임을 먼저 그립니다
        if (timerW > 0 && timerH > 0)
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
            int srcH = m_imgHudTimerGauge.GetHeight();
            
            // CImage::Draw의 assertion 오류를 방지하기 위해 모든 수치가 0보다 클 때만 그립니다.
            if (currentGaugeW > 0 && gaugeH > 0 && srcW > 0 && srcH > 0) {
                m_imgHudTimerGauge.Draw(hDC, gaugeX, gaugeY, currentGaugeW, gaugeH,
                    0, 0, srcW, srcH);
            }
        }
    }

    if (!m_imgHudInven.IsNull() && m_imgHudInven.GetWidth() > 0 && m_imgHudInven.GetHeight() > 0)
        m_imgHudInven.Draw(hDC, virtualWidth - m_imgHudInven.GetWidth() - 80, 0, m_imgHudInven.GetWidth() * 2, m_imgHudInven.GetHeight() * 2);

    // [마우스 클릭 아이콘 렌더링]
    int mouseIconScale = 2;
    int mouseIconY = 30; // Inven 아래쪽 위치
    int mouseIconX = virtualWidth - 110; // 우측 끝 기준

    if (!m_imgLeftClick.IsNull()) {
        m_imgLeftClick.Draw(hDC, mouseIconX, mouseIconY, 
            m_imgLeftClick.GetWidth() * mouseIconScale, m_imgLeftClick.GetHeight() * mouseIconScale);
    }
    if (!m_imgRightClick.IsNull()) {
        m_imgRightClick.Draw(hDC, mouseIconX + 70, mouseIconY, 
            m_imgRightClick.GetWidth() * mouseIconScale, m_imgRightClick.GetHeight() * mouseIconScale);
    }

    if (!m_imgCursor.IsNull() && m_imgCursor.GetWidth() > 0 && m_imgCursor.GetHeight() > 0) 
        m_imgCursor.Draw(hDC, mouseX - m_imgCursor.GetWidth(), mouseY - m_imgCursor.GetHeight(), m_imgCursor.GetWidth() * 2, m_imgCursor.GetHeight() * 2);
}


