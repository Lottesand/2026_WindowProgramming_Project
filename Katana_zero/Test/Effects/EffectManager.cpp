#include "EffectManager.h"
#include <math.h>
#include <algorithm>
#include "../Objects/Physics.h"
#include "../SceneAndMap/StageManager.h"

#pragma comment(lib, "msimg32.lib")

std::vector<NeonTrail> EffectManager::m_neonTrails;
std::vector<HitVFX> EffectManager::m_hitVFXs;
std::vector<JumpCloudVFX> EffectManager::m_jumpCloudVFXs;
std::vector<DustCloudVFX> EffectManager::m_dustCloudVFXs;
std::vector<LandCloudVFX> EffectManager::m_landCloudVFXs;
std::vector<PendingHit> EffectManager::m_pendingHits;
std::vector<BloodSplatterVFX> EffectManager::m_bloodSplatters;

HDC EffectManager::m_hBloodLayerDC = NULL;
HBITMAP EffectManager::m_hBloodLayerBmp = NULL;
HBITMAP EffectManager::m_hBloodLayerOldBmp = NULL;
void* EffectManager::m_pBloodLayerBits = nullptr;
int EffectManager::m_bloodLayerWidth = 0;
int EffectManager::m_bloodLayerHeight = 0;

CImage EffectManager::m_imgVfxSlash[5];
CImage EffectManager::m_imgVfxHit[6];
CImage EffectManager::m_imgVfxGunSpark[8];
CImage EffectManager::m_imgVfxJumpCloud[4];
CImage EffectManager::m_imgVfxDustCloud[7];
CImage EffectManager::m_imgVfxLandCloud[7];
CImage EffectManager::m_imgVfxBloodSplatter[7];
CImage EffectManager::m_imgVfxBloodBleed[9];
CImage EffectManager::m_imgVfxMapBloodDir[48];
CImage EffectManager::m_imgVfxMapBloodStatic[7];

void EffectManager::Init() {
    m_neonTrails.clear();
    m_hitVFXs.clear();
    m_jumpCloudVFXs.clear();
    m_dustCloudVFXs.clear();
    m_landCloudVFXs.clear();
    m_pendingHits.clear();
    m_bloodSplatters.clear();
    
    if (m_pBloodLayerBits && m_bloodLayerWidth > 0 && m_bloodLayerHeight > 0) {
        memset(m_pBloodLayerBits, 0, m_bloodLayerWidth * m_bloodLayerHeight * 4);
    }
}

void EffectManager::InitBloodLayer(int w, int h) {
    if (m_hBloodLayerDC) {
        SelectObject(m_hBloodLayerDC, m_hBloodLayerOldBmp);
        DeleteObject(m_hBloodLayerBmp);
        DeleteDC(m_hBloodLayerDC);
    }

    m_bloodLayerWidth = w;
    m_bloodLayerHeight = h;

    m_hBloodLayerDC = CreateCompatibleDC(NULL);
    
    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = -h; // Top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    m_hBloodLayerBmp = CreateDIBSection(m_hBloodLayerDC, &bmi, DIB_RGB_COLORS, &m_pBloodLayerBits, NULL, 0);
    m_hBloodLayerOldBmp = (HBITMAP)SelectObject(m_hBloodLayerDC, m_hBloodLayerBmp);
    
    memset(m_pBloodLayerBits, 0, w * h * 4);
}

void EffectManager::LoadAssets() {
    wchar_t path[256];
    for (int i = 0; i < 5; ++i) {
        swprintf_s(path, L"assets/player/spr_slashfx/%d.png", i);
        m_imgVfxSlash[i].Load(path);
    }
    for (int i = 0; i < 6; ++i) {
        swprintf_s(path, L"assets/player/spr_hit_impact/%d.png", i);
        m_imgVfxHit[i].Load(path);
    }
    for (int i = 0; i < 8; ++i) {
        swprintf_s(path, L"assets/gunspark/%d.png", i);
        m_imgVfxGunSpark[i].Load(path);
    }
    for (int i = 0; i < 4; ++i) {
        swprintf_s(path, L"assets/player/spr_jumpcloud/%d.png", i);
        m_imgVfxJumpCloud[i].Load(path);
    }
    for (int i = 0; i < 7; ++i) {
        swprintf_s(path, L"assets/player/spr_dustcloud/%d.png", i);
        m_imgVfxDustCloud[i].Load(path);
    }
    for (int i = 0; i < 7; ++i) {
        swprintf_s(path, L"assets/player/spr_landcloud/spr_landcloud_%d.png", i);
        m_imgVfxLandCloud[i].Load(path);
    }
    for (int i = 0; i < 7; ++i) {
        swprintf_s(path, L"assets/blood/spr_bloodsplatter_nondir/%d.png", i);
        m_imgVfxBloodSplatter[i].Load(path);
    }
    for (int i = 0; i < 9; ++i) {
        swprintf_s(path, L"assets/blood/Blood/%d.png", i);
        m_imgVfxBloodBleed[i].Load(path);
    }
    for (int i = 0; i < 48; ++i) {
        swprintf_s(path, L"assets/blood/spr_bloodsplatter_dir/%d.png", i);
        m_imgVfxMapBloodDir[i].Load(path);
    }
}

void EffectManager::ReleaseAssets() {
    for (int i = 0; i < 5; ++i) m_imgVfxSlash[i].Destroy();
    for (int i = 0; i < 6; ++i) m_imgVfxHit[i].Destroy();
    for (int i = 0; i < 4; ++i) m_imgVfxJumpCloud[i].Destroy();
    for (int i = 0; i < 7; ++i) m_imgVfxDustCloud[i].Destroy();
    for (int i = 0; i < 7; ++i) m_imgVfxLandCloud[i].Destroy();
    for (int i = 0; i < 7; ++i) m_imgVfxBloodSplatter[i].Destroy();
    for (int i = 0; i < 9; ++i) m_imgVfxBloodBleed[i].Destroy();
    for (int i = 0; i < 48; ++i) m_imgVfxMapBloodDir[i].Destroy();
}

void EffectManager::Update(float timeScale, DWORD currentTime) {
    for (auto it = m_neonTrails.begin(); it != m_neonTrails.end(); ) {
        it->life -= (int)(1.0f / timeScale);
        if (it->life <= 0) it = m_neonTrails.erase(it);
        else {
            it->length += (it->maxLength / (it->maxLife / 2.0f)) * timeScale;
            it++;
        }
    }
    for (auto it = m_hitVFXs.begin(); it != m_hitVFXs.end(); ) {
        if (currentTime - it->lastTime >= (DWORD)(10 / timeScale)) {
            it->currentFrame++;
            it->lastTime = currentTime;
        }
        if (it->currentFrame >= it->maxFrame) it = m_hitVFXs.erase(it);
        else it++;
    }
    for (auto it = m_jumpCloudVFXs.begin(); it != m_jumpCloudVFXs.end(); ) {
        if (currentTime - it->lastTime >= (DWORD)(40 / timeScale)) {
            it->currentFrame++;
            it->lastTime = currentTime;
        }
        if (it->currentFrame >= it->maxFrame) it = m_jumpCloudVFXs.erase(it);
        else it++;
    }
    for (auto it = m_dustCloudVFXs.begin(); it != m_dustCloudVFXs.end(); ) {
        if (currentTime < it->startTick) { it++; continue; }
        if (currentTime - it->lastTime >= (DWORD)(50 / timeScale)) {
            it->currentFrame++;
            it->lastTime = currentTime;
        }
        if (it->currentFrame >= it->maxFrame) it = m_dustCloudVFXs.erase(it);
        else it++;
    }
    for (auto it = m_landCloudVFXs.begin(); it != m_landCloudVFXs.end(); ) {
        if (currentTime - it->lastTime >= (DWORD)(40 / timeScale)) {
            it->currentFrame++;
            it->lastTime = currentTime;
        }
        if (it->currentFrame >= it->maxFrame) it = m_landCloudVFXs.erase(it);
        else it++;
    }
    for (auto it = m_pendingHits.begin(); it != m_pendingHits.end(); ) {
        it->remainingFrames--;
        if (it->remainingFrames <= 0) {
            if (it->target && it->target->GetIsAlive()) {
                it->target->OnTakeDamage(it->kvx, it->kvy);
            }
            it = m_pendingHits.erase(it);
        } else it++;
    }
    for (auto it = m_bloodSplatters.begin(); it != m_bloodSplatters.end(); ) {
        if (currentTime - it->startTime > 500) {
            it = m_bloodSplatters.erase(it);
        } else {
            it->x += it->vx * timeScale;
            it->y += it->vy * timeScale;
            it++;
        }
    }
}

void EffectManager::Render(HDC hDC, float camX, float camY, float mapScale, bool isFullMapView, float cFS, float cFX, float cFY) {
    for (const auto& tr : m_neonTrails) {
        float pFS = isFullMapView ? cFS : mapScale;
        bool isP = (GetTickCount() / 50) % 2 == 0;
        COLORREF dC = isP ? RGB(255, 0, 255) : RGB(0, 255, 255);
        int segs = 20; float sLen = tr.length / segs;
        for (int i = 0; i < segs; ++i) {
            float sD = i * sLen, eD = (i + 1) * sLen;
            float eA = 1.0f, prog = (float)i / segs;
            if (prog < 0.15f) eA = prog / 0.15f; else if (prog > 0.85f) eA = (1.0f - prog) / 0.15f;
            float sX = tr.startX + tr.dirX * sD, sY = tr.startY + tr.dirY * sD;
            float eX = tr.startX + tr.dirX * eD, eY = tr.startY + tr.dirY * eD;
            float vSX, vSY, vEX, vEY;
            if (isFullMapView) { vSX = sX * cFS + cFX; vSY = sY * cFS + cFY; vEX = eX * cFS + cFX; vEY = eY * cFS + cFY; }
            else { vSX = (sX - camX) * mapScale; vSY = (sY - camY) * mapScale; vEX = (eX - camX) * mapScale; vEY = (eY - camY) * mapScale; }
            float bW = m_slashWidth * pFS * (tr.life / (float)tr.maxLife) * 1.5f; if (bW < 1.0f) bW = 1.0f;
            COLORREF fC = RGB((int)(GetRValue(dC) * eA), (int)(GetGValue(dC) * eA), (int)(GetBValue(dC) * eA));
            HPEN hP = CreatePen(PS_SOLID, (int)bW, fC); HPEN hOP = (HPEN)SelectObject(hDC, hP);
            MoveToEx(hDC, (int)vSX, (int)vSY, NULL); LineTo(hDC, (int)vEX, (int)vEY);
            SelectObject(hDC, hOP); DeleteObject(hP);
        }
    }
    for (const auto& v : m_hitVFXs) {
        int safeF = (std::max)(0, v.currentFrame);
        int f = (std::min)(safeF, v.isSlash ? 4 : (v.isGunSpark ? 7 : 5));
        CImage* vI = nullptr;
        if (v.isSlash) vI = &m_imgVfxSlash[f];
        else if (v.isGunSpark) vI = &m_imgVfxGunSpark[f];
        else vI = &m_imgVfxHit[f];

        if (vI && !vI->IsNull()) {
            float customMultiplier = v.isSlash ? 1.5f : 1.1f; // Reduced from 1.8f to 1.1f
            if (v.isGunSpark) customMultiplier = 1.2f; // Adjusted for gunspark as well
            
            float vfxScale = 2.0f * (isFullMapView ? cFS : mapScale) * customMultiplier;
            int vW = (int)(vI->GetWidth() * vfxScale), vH = (int)(vI->GetHeight() * vfxScale);
            float dX, dY;
            if (isFullMapView) { dX = v.x * cFS + cFX; dY = v.y * cFS + cFY; }
            else { dX = (v.x - camX) * mapScale; dY = (v.y - camY) * mapScale; }
            
            int oldMode = SetGraphicsMode(hDC, GM_ADVANCED);
            XFORM xF; xF.eM11 = cos(v.angle); xF.eM12 = sin(v.angle); xF.eM21 = -sin(v.angle); xF.eM22 = cos(v.angle); xF.eDx = dX; xF.eDy = dY;
            SetWorldTransform(hDC, &xF); 
            if (vW > 0 && vH > 0) vI->Draw(hDC, -vW / 2, -vH / 2, vW, vH);
            XFORM xFI = { 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f }; SetWorldTransform(hDC, &xFI);
            SetGraphicsMode(hDC, oldMode);
        }
    }
    for (const auto& v : m_jumpCloudVFXs) {
        int safeF = (std::max)(0, v.currentFrame);
        CImage* vI = &m_imgVfxJumpCloud[(std::min)(safeF, 3)];
        if (vI && !vI->IsNull()) {
            float vfxScale = 2.0f * (isFullMapView ? cFS : mapScale);
            int vW = (int)(vI->GetWidth() * vfxScale), vH = (int)(vI->GetHeight() * vfxScale);
            float dX, dY;
            if (isFullMapView) { dX = v.x * cFS + cFX; dY = v.y * cFS + cFY; }
            else { dX = (v.x - camX) * mapScale; dY = (v.y - camY) * mapScale; }
            
            int oldMode = SetGraphicsMode(hDC, GM_ADVANCED);
            XFORM xF, oldXF; GetWorldTransform(hDC, &oldXF);
            
            float angle = v.angle;
            float cosA = cos(angle), sinA = sin(angle);
            XFORM rot = { cosA, sinA, -sinA, cosA, dX, dY };
            CombineTransform(&xF, &rot, &oldXF);
            SetWorldTransform(hDC, &xF);

            if (vW > 0 && vH > 0) vI->Draw(hDC, -vW / 2, -vH, vW, vH);
            
            SetWorldTransform(hDC, &oldXF);
            SetGraphicsMode(hDC, oldMode);
        }
    }
    for (const auto& v : m_dustCloudVFXs) {
        if (GetTickCount() < v.startTick) continue;
        int safeF = (std::max)(0, v.currentFrame);
        CImage* vI = &m_imgVfxDustCloud[(std::min)(safeF, 6)];
        if (vI && !vI->IsNull()) {
            float progress = (float)safeF / (float)v.maxFrame;
            float vfxScale = 2.0f * (isFullMapView ? cFS : mapScale) * (0.8f + progress * 1.0f);
            int vW = (int)(vI->GetWidth() * vfxScale), vH = (int)(vI->GetHeight() * vfxScale);
            float dX, dY;
            if (isFullMapView) { dX = v.x * cFS + cFX; dY = v.y * cFS + cFY; }
            else { dX = (v.x - camX) * mapScale; dY = (v.y - camY) * mapScale; }
            if (vW > 0 && vH > 0) vI->Draw(hDC, (int)dX - vW / 2, (int)dY - vH, vW, vH);
        }
    }
    for (const auto& v : m_landCloudVFXs) {
        int safeF = (std::max)(0, v.currentFrame);
        CImage* vI = &m_imgVfxLandCloud[(std::min)(safeF, 6)];
        if (vI && !vI->IsNull()) {
            float vfxScale = 2.0f * (isFullMapView ? cFS : mapScale);
            int vW = (int)(vI->GetWidth() * vfxScale), vH = (int)(vI->GetHeight() * vfxScale);
            float dX, dY;
            if (isFullMapView) { dX = v.x * cFS + cFX; dY = v.y * cFS + cFY; }
            else { dX = (v.x - camX) * mapScale; dY = (v.y - camY) * mapScale; }
            if (vW > 0 && vH > 0) vI->Draw(hDC, (int)dX - vW / 2, (int)dY - vH, vW, vH);
        }
    }
    for (const auto& v : m_bloodSplatters) {
        CImage* vI = v.isDirectional ? &m_imgVfxMapBloodDir[v.imgIndex] : &m_imgVfxBloodSplatter[v.imgIndex];
        if (vI && !vI->IsNull()) {
            float baseScale = v.isDirectional ? 2.0f : 2.5f;
            float vfxScale = baseScale * (isFullMapView ? cFS : mapScale);
            int vW = (int)(vI->GetWidth() * vfxScale), vH = (int)(vI->GetHeight() * vfxScale);
            float dX, dY;
            if (isFullMapView) { dX = v.x * cFS + cFX; dY = v.y * cFS + cFY; }
            else { dX = (v.x - camX) * mapScale; dY = (v.y - camY) * mapScale; }
            
            if (vW > 0 && vH > 0) {
                int oldMode = SetGraphicsMode(hDC, GM_ADVANCED);
                XFORM xF, oldXF; GetWorldTransform(hDC, &oldXF);
                // For non-directional, we rotate randomly for variety, or use the provided angle
                float rotAngle = v.angle;
                xF.eM11 = cos(rotAngle); xF.eM12 = sin(rotAngle); xF.eM21 = -sin(rotAngle); xF.eM22 = cos(rotAngle); xF.eDx = dX; xF.eDy = dY;
                SetWorldTransform(hDC, &xF);
                vI->Draw(hDC, -vW / 2, -vH / 2, vW, vH);
                SetWorldTransform(hDC, &oldXF); SetGraphicsMode(hDC, oldMode);
            }
        }
    }
}

void EffectManager::AddNeonTrail(float x, float y, float ux, float uy, float angle) {
    NeonTrail trail; trail.x = x; trail.y = y; trail.startX = x - ux * 1000.0f; trail.startY = y - uy * 1000.0f;
    trail.endX = x + ux * 3000.0f; trail.endY = y + uy * 3000.0f; trail.dirX = ux; trail.dirY = uy; trail.angle = angle;
    trail.length = 0.0f; trail.maxLength = 4000.0f; trail.life = m_neonTrailLife; trail.maxLife = m_neonTrailLife;
    m_neonTrails.push_back(trail);
}

void EffectManager::AddHitVFX(float x, float y, float angle, DWORD currentTime) {
    HitVFX s; s.x = x; s.y = y; s.angle = angle; s.currentFrame = 0; s.maxFrame = 5; s.lastTime = currentTime; s.isSlash = true; s.isGunSpark = false; m_hitVFXs.push_back(s);
    HitVFX i; i.x = x; i.y = y; i.angle = angle; i.currentFrame = 0; i.maxFrame = 6; i.lastTime = currentTime; i.isSlash = false; i.isGunSpark = false; m_hitVFXs.push_back(i);
    
    // Add extra bloody particles in the direction of the hit
    float ux = cos(angle), uy = sin(angle);
    for (int j = 0; j < 8; ++j) {
        float speed = 8.0f + (rand() % 80) / 10.0f;
        float spread = (rand() % 40 - 20) / 10.0f;
        float vx = ux * speed - uy * spread;
        float vy = uy * speed + ux * spread;
        AddBloodSplatter(x, y, vx, vy, angle, currentTime);
    }
}

void EffectManager::AddGunSparkVFX(float x, float y, float angle, DWORD currentTime) {
    // A gun spark is a brighter, slightly larger hit impact without blood
    HitVFX i; i.x = x; i.y = y; i.angle = angle; i.currentFrame = 0; i.maxFrame = 6; i.lastTime = currentTime; i.isSlash = false; i.isGunSpark = true; 
    m_hitVFXs.push_back(i);
}

void EffectManager::AddJumpCloudVFX(float x, float y, DWORD currentTime, float angle) {
    JumpCloudVFX c; c.x = x; c.y = y; c.angle = angle; c.currentFrame = 0; c.maxFrame = 4; c.lastTime = currentTime; m_jumpCloudVFXs.push_back(c);
}

void EffectManager::AddDustCloudVFX(float x, float y, bool isFacingRight, DWORD currentTime, float angle) {
    static int bc = 0; DustCloudVFX c; c.x = x; c.y = y; c.isFacingRight = isFacingRight; c.angle = angle; c.currentFrame = 0; c.maxFrame = 7;
    c.startTick = currentTime + (bc * 60); c.lastTime = c.startTick; m_dustCloudVFXs.push_back(c); bc = (bc + 1) % 3;
}

void EffectManager::AddLandCloudVFX(float x, float y, DWORD currentTime) {
    LandCloudVFX c; c.x = x; c.y = y; c.currentFrame = 0; c.maxFrame = 7; c.lastTime = currentTime; m_landCloudVFXs.push_back(c);
}

void EffectManager::AddBloodSplatter(float x, float y, float vx, float vy, float angle, DWORD currentTime) {
    BloodSplatterVFX b;
    b.x = x; b.y = y; b.vx = vx; b.vy = vy; b.angle = angle;
    
    // Switch to non-directional assets as requested
    b.isDirectional = false;
    b.imgIndex = rand() % 7; 
    
    b.startTime = currentTime; b.isBleed = false;
    m_bloodSplatters.push_back(b);
}

void EffectManager::AddPendingHit(Enemy* target, float kvx, float kvy) {
    PendingHit p; p.target = target; p.kvx = kvx; p.remainingFrames = 3; m_pendingHits.push_back(p);
}

void EffectManager::AddMapBlood(float x, float y, float angle, int imgIndex) {
    if (!m_hBloodLayerDC || !m_pBloodLayerBits) return;

    CImage* vI = &m_imgVfxBloodBleed[imgIndex % 9];
    if (vI && !vI->IsNull()) {
        float vfxScale = 1.5f; 
        int vW = (int)(vI->GetWidth() * vfxScale), vH = (int)(vI->GetHeight() * vfxScale);
        
        // Use a temporary DC to handle rotation and scaling before drawing to the layer
        HDC hTempDC = CreateCompatibleDC(m_hBloodLayerDC);
        HBITMAP hTempBmp = CreateCompatibleBitmap(m_hBloodLayerDC, vW, vH);
        HBITMAP hOldTempBmp = (HBITMAP)SelectObject(hTempDC, hTempBmp);
        
        // Clear temp DC with transparent black
        // Since we want to use AlphaBlend, we need to be careful with transparency.
        // For simplicity, we can just draw the CImage with its own AlphaBlend to the layer DC.
        // But first, let's handle the positioning.
        
        int destX = (int)x - vW / 2;
        int destY = (int)y - vH / 2;

        // Draw blood splatter to the blood layer using global AlphaBlend for reliability
        HDC hSrcDC = vI->GetDC();
        if (hSrcDC) {
            BLENDFUNCTION bf = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
            ::AlphaBlend(m_hBloodLayerDC, destX, destY, vW, vH, hSrcDC, 0, 0, vI->GetWidth(), vI->GetHeight(), bf);
            vI->ReleaseDC();
        }

        // Now, perform masking: iterate over the area we just drew and clear bits if map is transparent
        int startX = (std::max)(0, destX);
        int startY = (std::max)(0, destY);
        int endX = (std::min)(m_bloodLayerWidth, destX + vW);
        int endY = (std::min)(m_bloodLayerHeight, destY + vH);

        DWORD* pPixels = (DWORD*)m_pBloodLayerBits;
        for (int py = startY; py < endY; ++py) {
            for (int px = startX; px < endX; ++px) {
                if (IsMapTransparent(px, py)) {
                    pPixels[py * m_bloodLayerWidth + px] = 0; // Clear pixel (Alpha = 0)
                }
            }
        }

        DeleteObject(hTempBmp);
        DeleteDC(hTempDC);
    }
}

void EffectManager::RenderMapBlood(HDC hDC, Gdiplus::Graphics* g, float camX, float camY, float mapScale) {
    if (!hDC || !m_hBloodLayerDC || !m_hBloodLayerBmp) return;
    
    // Draw the entire blood layer with 70% transparency
    int destW = (int)(m_bloodLayerWidth * mapScale);
    int destH = (int)(m_bloodLayerHeight * mapScale);
    int srcX = (int)camX;
    int srcY = (int)camY;
    int srcW = (int)(m_bloodLayerWidth); // We'll let AlphaBlend handle the scaling from src to dest
    int srcH = (int)(m_bloodLayerHeight);

    // If mapScale is 1.0, src and dest size are same
    // We only want to draw what's visible in the camera
    // For simplicity, we draw the whole thing scaled and offset
    
    // Calculate source rect from camera
    // In Katana Zero, camX/camY are usually top-left of view.
    // mapScale is typically 1.0f or different for zoom.
    
    // Source width/height in world pixels that fit the screen
    // screenWidth = 1280, screenHeight = 720.
    // if mapScale = 1, srcW = 1280.
    
    float viewW = 1280.0f / mapScale;
    float viewH = 720.0f / mapScale;

    BLENDFUNCTION bf = { AC_SRC_OVER, 0, (BYTE)(255 * 0.7f), AC_SRC_ALPHA };
    ::AlphaBlend(hDC, 0, 0, 1280, 720, 
                 m_hBloodLayerDC, (int)camX, (int)camY, (int)viewW, (int)viewH, bf);
}

bool EffectManager::HasActiveHitVFX() { return !m_hitVFXs.empty(); }
bool EffectManager::HasActiveVFX() { return !m_neonTrails.empty() || !m_hitVFXs.empty() || !m_jumpCloudVFXs.empty() || !m_dustCloudVFXs.empty() || !m_landCloudVFXs.empty(); }
