#include "StartScene.h"
#include "../Core/Input.h"
#include "../SceneAndMap/StageManager.h"
#include "../SceneAndMap/Camera.h"
#include <string>

bool StartScene::m_active = false;
StartScene::State StartScene::m_state = StartScene::State::WAIT_CAMERA;
float StartScene::m_timer = 0.0f;

CImage StartScene::m_imgPlayerPlay[31];
CImage StartScene::m_imgPlaySong;
CImage StartScene::m_imgSongName;
CImage StartScene::m_imgMansionTitle;
CImage StartScene::m_imgAttackStart;
CImage StartScene::m_imgLeftClick;

int StartScene::m_playerFrame = 0;
float StartScene::m_boxWidthProgress = 0.0f;
float StartScene::m_pngAlpha = 0.0f;
float StartScene::m_overlayAlpha = 0.0f;
float StartScene::m_titleProgress = 0.0f;
bool StartScene::m_showFinalPrompts = false;

void StartScene::Init() {
}

void StartScene::LoadAssets() {
    TCHAR path[256];
    for (int i = 0; i < 31; i++) {
        wsprintf(path, TEXT("assets/player/spr_player_playsong/spr_player_playsong_%d.png"), i);
        if (m_imgPlayerPlay[i].IsNull()) m_imgPlayerPlay[i].Load(path);
    }
    if (m_imgPlaySong.IsNull()) m_imgPlaySong.Load(TEXT("assets/startscene/play_song.png"));
    if (m_imgSongName.IsNull()) m_imgSongName.Load(TEXT("assets/startscene/song_name.png"));
    if (m_imgMansionTitle.IsNull()) m_imgMansionTitle.Load(TEXT("assets/startscene/mansion_title.png"));
    if (m_imgAttackStart.IsNull()) m_imgAttackStart.Load(TEXT("assets/startscene/attack_start.png"));
    if (m_imgLeftClick.IsNull()) m_imgLeftClick.Load(TEXT("assets/hud/left_click.png"));
}

void StartScene::ReleaseAssets() {
    for (int i = 0; i < 31; i++) m_imgPlayerPlay[i].Destroy();
    m_imgPlaySong.Destroy();
    m_imgSongName.Destroy();
    m_imgMansionTitle.Destroy();
    m_imgAttackStart.Destroy();
    m_imgLeftClick.Destroy();
}

void StartScene::Reset() {
    m_state = State::WAIT_CAMERA;
    m_timer = 0.0f;
    m_playerFrame = 0;
    m_boxWidthProgress = 0.0f;
    m_pngAlpha = 0.0f;
    m_overlayAlpha = 0.0f;
    m_titleProgress = 0.0f;
    m_showFinalPrompts = false;
}

void StartScene::Update(float dT, bool& bGameStarted) {
    if (!m_active) return;

    m_timer += dT;

    switch (m_state) {
    case State::WAIT_CAMERA:
        if (m_timer >= 0.8f) {
            m_timer = 0.0f;
            m_state = State::PLAYER_ANIM;
        }
        break;
    case State::PLAYER_ANIM:
        if (m_timer >= 0.05f) { // 20 FPS
            m_playerFrame++;
            m_timer = 0.0f;
            if (m_playerFrame >= 31) {
                m_playerFrame = 30;
                m_state = State::BOX_EXPAND;
            }
        }
        break;
    case State::BOX_EXPAND:
        m_boxWidthProgress += dT * 4.0f;
        if (m_boxWidthProgress >= 1.0f) {
            m_boxWidthProgress = 1.0f;
            m_state = State::PNG_FADE_IN;
        }
        break;
    case State::PNG_FADE_IN:
        m_pngAlpha += dT * 3.0f;
        if (m_pngAlpha >= 1.0f) {
            m_pngAlpha = 1.0f;
            m_state = State::OVERLAY_FADE;
        }
        break;
    case State::OVERLAY_FADE:
        m_overlayAlpha += dT * 1.5f;
        if (m_overlayAlpha >= 0.75f) {
            m_overlayAlpha = 0.75f;
            m_state = State::TITLE_MERGE;
        }
        m_titleProgress += dT * 1.5f;
        if (m_titleProgress >= 1.0f) {
            m_titleProgress = 1.0f;
            if (m_overlayAlpha >= 0.75f) {
                m_state = State::WAIT_CLICK;
                m_showFinalPrompts = true;
            }
        }
        break;
    case State::TITLE_MERGE:
        m_titleProgress += dT * 1.5f;
        if (m_titleProgress >= 1.0f) {
            m_titleProgress = 1.0f;
            m_state = State::WAIT_CLICK;
            m_showFinalPrompts = true;
        }
        break;
    case State::WAIT_CLICK:
        if (Input::GetKeyDown(VK_LBUTTON)) {
            m_state = State::ENDING;
            m_timer = 0.0f;
            Camera::StartRewindEffect(1.0f); // 카메라 리와인드 연출 시작
        }
        break;
    case State::ENDING:
        // 1. PNG 이미지들 먼저 페이드 아웃
        if (m_pngAlpha > 0.0f) {
            m_pngAlpha -= dT * 5.0f;
            if (m_pngAlpha < 0.0f) m_pngAlpha = 0.0f;
        }
        // 2. PNG가 다 사라지면 박스랑 구분선을 오른쪽에서 왼쪽으로 지움
        else if (m_boxWidthProgress > 0.0f) {
            m_boxWidthProgress -= dT * 4.0f;
            if (m_boxWidthProgress < 0.0f) m_boxWidthProgress = 0.0f;
        }
        // 3. 종료
        else {
            bGameStarted = true;
            m_active = false;
        }
        break;
    }
}

void StartScene::Render(HDC hDC, int virtualWidth, int virtualHeight, float playerX, float playerY, float playerColW, float playerColH, float mapScale) {
    if (!m_active) return;

    Gdiplus::Graphics g(hDC);

    // 1. spr_player_playsong
    if (m_state >= State::PLAYER_ANIM) {
        if (!m_imgPlayerPlay[m_playerFrame].IsNull()) {
            float camX = Camera::GetCamX();
            float camY = Camera::GetCamY();
            float playerScale = 2.0f;
            float pFS = mapScale;
            float vx = (playerX - camX) * pFS;
            float vy = (playerY - camY) * pFS;
            float sw = m_imgPlayerPlay[m_playerFrame].GetWidth() * playerScale * pFS;
            float sh = m_imgPlayerPlay[m_playerFrame].GetHeight() * playerScale * pFS;
            float dx = vx + (playerColW * pFS) / 2.0f - (sw / 2.0f);
            float dy = vy + (playerColH * pFS) - sh;
            m_imgPlayerPlay[m_playerFrame].Draw(hDC, (int)dx, (int)dy, (int)sw, (int)sh);
        }
    }

    // 2. Bottom-left Box
    if (m_state >= State::BOX_EXPAND) {
        float boxW = playerColW * 6.0f;
        float boxH = playerColH * 1.3f;
        float currentW = boxW * m_boxWidthProgress;
        int startX = 80;
        int startY = virtualHeight - (int)boxH - 50;

        // Shadow (Pink)
        Gdiplus::SolidBrush pinkBrush(Gdiplus::Color(180, 255, 0, 255));
        g.FillRectangle(&pinkBrush, (Gdiplus::REAL)(startX + 5), (Gdiplus::REAL)(startY + 5), (Gdiplus::REAL)currentW, (Gdiplus::REAL)boxH);

        // Main Box (Indigo)
        Gdiplus::SolidBrush indigoBrush(Gdiplus::Color(180, 50, 60, 240));
        g.FillRectangle(&indigoBrush, (Gdiplus::REAL)startX, (Gdiplus::REAL)startY, (Gdiplus::REAL)currentW, (Gdiplus::REAL)boxH);

        // Content
        if (m_state >= State::PNG_FADE_IN || m_state == State::ENDING) {
            int centerY = startY + (int)(boxH / 2);
            int spacing = -30;

            // 중앙 핑크색 구분선 (박스 너비에 맞춰 축소됨)
            if (m_boxWidthProgress > 0.1f) {
                Gdiplus::SolidBrush separatorBrush(Gdiplus::Color((BYTE)((m_state == State::ENDING ? 1.0f : m_pngAlpha) * 255), 255, 0, 255));
                float maxSepW = boxW - 40;
                float currentSepW = maxSepW * m_boxWidthProgress;
                if (m_state != State::ENDING) currentSepW = maxSepW * m_pngAlpha;
                if (currentSepW < 0) currentSepW = 0;
                g.FillRectangle(&separatorBrush, (Gdiplus::REAL)(startX + 20), (Gdiplus::REAL)(centerY - 1), (Gdiplus::REAL)currentSepW, 2.0f);
            }

            // PNG 이미지 페이드 아웃/인
            if (m_pngAlpha > 0.01f) {
                Gdiplus::ImageAttributes attr;
                Gdiplus::ColorMatrix mat = { 1,0,0,0,0, 0,1,0,0,0, 0,0,1,0,0, 0,0,0, m_pngAlpha, 0, 0,0,0,0,1 };
                attr.SetColorMatrix(&mat);

                if (!m_imgPlaySong.IsNull()) {
                    int iw = (int)(m_imgPlaySong.GetWidth() * 0.8f);
                    int ih = (int)(m_imgPlaySong.GetHeight() * 0.8f);
                    int ix = startX + 10;
                    int iy = centerY - spacing - ih;
                    Gdiplus::Bitmap bmp(m_imgPlaySong.GetWidth(), m_imgPlaySong.GetHeight(), m_imgPlaySong.GetPitch(), PixelFormat32bppARGB, (BYTE*)m_imgPlaySong.GetBits());
                    g.DrawImage(&bmp, Gdiplus::Rect(ix, iy, iw, ih), 0, 0, bmp.GetWidth(), bmp.GetHeight(), Gdiplus::UnitPixel, &attr);
                }
                if (!m_imgSongName.IsNull()) {
                    int iw = (int)(m_imgSongName.GetWidth() * 0.8f);
                    int ih = (int)(m_imgSongName.GetHeight() * 0.8f);
                    int ix = startX + 10;
                    int iy = centerY + spacing;
                    Gdiplus::Bitmap bmp(m_imgSongName.GetWidth(), m_imgSongName.GetHeight(), m_imgSongName.GetPitch(), PixelFormat32bppARGB, (BYTE*)m_imgSongName.GetBits());
                    g.DrawImage(&bmp, Gdiplus::Rect(ix, iy, iw, ih), 0, 0, bmp.GetWidth(), bmp.GetHeight(), Gdiplus::UnitPixel, &attr);
                }
            }
        }
    }

    // 3. Full-width Overlay (ENDING 상태에선 즉시 숨김)
    if (m_state >= State::OVERLAY_FADE && m_state < State::ENDING) {
        Gdiplus::SolidBrush overlayBrush(Gdiplus::Color((BYTE)(m_overlayAlpha * 255), 0, 0, 0));
        int oh = (int)(playerColH * 1.3f);
        int oy = (virtualHeight - oh) / 2;
        g.FillRectangle(&overlayBrush, 0, oy, virtualWidth, oh);

        if (!m_imgMansionTitle.IsNull()) {
            int tw = m_imgMansionTitle.GetWidth();
            int th = m_imgMansionTitle.GetHeight();
            float targetX = (virtualWidth - tw) / 2.0f;
            float targetY = (virtualHeight - th) / 2.0f;

            float pinkX = virtualWidth - (virtualWidth - (targetX + 5)) * m_titleProgress;
            Gdiplus::ImageAttributes pinkAttr;
            Gdiplus::ColorMatrix pinkMat = { 1,0,0,0,0, 0,0.5f,0,0,0, 0,0,1,0,0, 0,0,0, 0.8f, 0, 0,0,0,0,1 };
            pinkAttr.SetColorMatrix(&pinkMat);
            Gdiplus::Bitmap bmpTitle(m_imgMansionTitle, NULL);
            g.DrawImage(&bmpTitle, Gdiplus::Rect((int)pinkX, (int)(targetY + 5), tw, th), 0, 0, tw, th, Gdiplus::UnitPixel, &pinkAttr);

            float cyanX = -tw + (targetX + tw) * m_titleProgress;
            Gdiplus::ImageAttributes cyanAttr;
            Gdiplus::ColorMatrix cyanMat = { 0,0,0,0,0, 0,1.0f,0,0,0, 0,0,1.0f,0,0, 0,0,0, 1, 0, 0,0,0,0,1 };
            cyanAttr.SetColorMatrix(&cyanMat);
            g.DrawImage(&bmpTitle, Gdiplus::Rect((int)cyanX, (int)targetY, tw, th), 0, 0, tw, th, Gdiplus::UnitPixel, &cyanAttr);
        }
    }

    // 5. Final Prompts (ENDING 상태에선 즉시 숨김)
    if (m_showFinalPrompts && m_state < State::ENDING) {
        int oy = (virtualHeight + (int)(playerColH * 1.3f)) / 2 + 10;
        if (!m_imgAttackStart.IsNull() && !m_imgLeftClick.IsNull()) {
            int iw1 = (int)(m_imgAttackStart.GetWidth());
            int ih1 = (int)(m_imgAttackStart.GetHeight());
            int iw2 = (int)(m_imgLeftClick.GetWidth() * 2.0f);
            int ih2 = (int)(m_imgLeftClick.GetHeight() * 2.0f);
            int totalW = iw1 + iw2 + 20;
            int startX = (virtualWidth - totalW) / 2;
            m_imgAttackStart.Draw(hDC, startX, oy, iw1, ih1);
            m_imgLeftClick.Draw(hDC, startX + iw1 + 20, oy, iw2, ih2);
        }
    }
}