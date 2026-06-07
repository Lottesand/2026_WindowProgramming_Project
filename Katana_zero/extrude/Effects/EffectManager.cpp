#include "EffectManager.h"
#include <math.h>
#include <algorithm>

std::vector<NeonTrail> EffectManager::m_neonTrails;
std::vector<HitVFX> EffectManager::m_hitVFXs;
std::vector<JumpCloudVFX> EffectManager::m_jumpCloudVFXs;
std::vector<DustCloudVFX> EffectManager::m_dustCloudVFXs;
std::vector<LandCloudVFX> EffectManager::m_landCloudVFXs;
std::vector<PendingHit> EffectManager::m_pendingHits;
CImage EffectManager::m_imgVfxSlash[5];
CImage EffectManager::m_imgVfxHit[6];
CImage EffectManager::m_imgVfxJumpCloud[4];
CImage EffectManager::m_imgVfxDustCloud[7];
CImage EffectManager::m_imgVfxLandCloud[7];

void EffectManager::Init() {
    m_neonTrails.clear();
    m_hitVFXs.clear();
    m_jumpCloudVFXs.clear();
    m_dustCloudVFXs.clear();
    m_landCloudVFXs.clear();
    m_pendingHits.clear();
}

void EffectManager::LoadAssets() {
    wchar_t path[256];
    for (int i = 0; i < 5; ++i) {
        swprintf_s(path, L"assets/spr_slashfx/%d.png", i);
        m_imgVfxSlash[i].Load(path);
    }
    for (int i = 0; i < 6; ++i) {
        swprintf_s(path, L"assets/spr_hit_impact/%d.png", i);
        m_imgVfxHit[i].Load(path);
    }
    for (int i = 0; i < 4; ++i) {
        swprintf_s(path, L"assets/spr_jumpcloud/%d.png", i);
        m_imgVfxJumpCloud[i].Load(path);
    }
    for (int i = 0; i < 7; ++i) {
        swprintf_s(path, L"assets/spr_dustcloud/%d.png", i);
        m_imgVfxDustCloud[i].Load(path);
    }
    for (int i = 0; i < 7; ++i) {
        swprintf_s(path, L"assets/spr_landcloud/spr_landcloud_%d.png", i);
        m_imgVfxLandCloud[i].Load(path);
    }
}

void EffectManager::ReleaseAssets() {
    for (int i = 0; i < 5; ++i) m_imgVfxSlash[i].Destroy();
    for (int i = 0; i < 6; ++i) m_imgVfxHit[i].Destroy();
    for (int i = 0; i < 4; ++i) m_imgVfxJumpCloud[i].Destroy();
    for (int i = 0; i < 7; ++i) m_imgVfxDustCloud[i].Destroy();
    for (int i = 0; i < 7; ++i) m_imgVfxLandCloud[i].Destroy();
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
        if (currentTime - it->lastTime >= (DWORD)(20 / timeScale)) {
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
                it->target->OnTakeDamage(1.0f);
                it->target->ApplyKnockback(it->kbForce);
            }
            it = m_pendingHits.erase(it);
        } else it++;
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
        int f = (std::min)(safeF, v.isSlash ? 4 : 5);
        CImage* vI = v.isSlash ? &m_imgVfxSlash[f] : &m_imgVfxHit[f];
        if (vI && !vI->IsNull()) {
            float customMultiplier = v.isSlash ? 1.5f : 1.8f;
            float vfxScale = 2.0f * (isFullMapView ? cFS : mapScale) * customMultiplier;
            int vW = (int)(vI->GetWidth() * vfxScale), vH = (int)(vI->GetHeight() * vfxScale);
            float dX, dY;
            if (isFullMapView) { dX = v.x * cFS + cFX; dY = v.y * cFS + cFY; }
            else { dX = (v.x - camX) * mapScale; dY = (v.y - camY) * mapScale; }
            
            int oldMode = SetGraphicsMode(hDC, GM_ADVANCED);
            XFORM xF; xF.eM11 = cos(v.angle); xF.eM12 = sin(v.angle); xF.eM21 = -sin(v.angle); xF.eM22 = cos(v.angle); xF.eDx = dX; xF.eDy = dY;
            SetWorldTransform(hDC, &xF); 
            
            if (vW > 0 && vH > 0 && vI->GetWidth() > 0 && vI->GetHeight() > 0) {
                vI->Draw(hDC, -vW / 2, -vH / 2, vW, vH);
            }

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
            if (v.angle != 0.0f) {
                XFORM xF, oldXF; int oldMode = GetGraphicsMode(hDC); SetGraphicsMode(hDC, GM_ADVANCED); GetWorldTransform(hDC, &oldXF);
                xF.eM11 = cos(v.angle); xF.eM12 = sin(v.angle); xF.eM21 = -sin(v.angle); xF.eM22 = cos(v.angle); xF.eDx = dX; xF.eDy = dY;
                SetWorldTransform(hDC, &xF); 
                if (vW > 0 && vH > 0 && vI->GetWidth() > 0 && vI->GetHeight() > 0) vI->Draw(hDC, -vW / 2, -vH, vW, vH);
                SetWorldTransform(hDC, &oldXF); SetGraphicsMode(hDC, oldMode);
            } else {
                if (vW > 0 && vH > 0 && vI->GetWidth() > 0 && vI->GetHeight() > 0) vI->Draw(hDC, (int)dX - vW / 2, (int)dY - vH, vW, vH);
            }
        }
    }
    for (const auto& v : m_dustCloudVFXs) {
        if (GetTickCount() < v.startTick) continue;
        int safeF = (std::max)(0, v.currentFrame);
        CImage* vI = &m_imgVfxDustCloud[(std::min)(safeF, 6)];
        if (vI && !vI->IsNull()) {
            float progress = (float)safeF / (float)v.maxFrame;
            float vfxScale = 2.0f * (isFullMapView ? cFS : mapScale) * (0.8f + progress * 1.0f);
            float driftOffset = progress * 35.0f * (isFullMapView ? cFS : mapScale);
            int vW = (int)(vI->GetWidth() * vfxScale), vH = (int)(vI->GetHeight() * vfxScale);
            float dX, dY;
            if (isFullMapView) { dX = v.x * cFS + cFX; dY = v.y * cFS + cFY; }
            else { dX = (v.x - camX) * mapScale; dY = (v.y - camY) * mapScale; }
            if (v.isFacingRight) {
                if (vW > 0 && vH > 0 && vI->GetWidth() > 0 && vI->GetHeight() > 0) vI->Draw(hDC, (int)(dX - vW - driftOffset), (int)dY - vH, vW, vH);
            }
            else {
                XFORM xF, oldXF; int oldMode = GetGraphicsMode(hDC); SetGraphicsMode(hDC, GM_ADVANCED); GetWorldTransform(hDC, &oldXF);
                xF.eM11 = -1.0f; xF.eM12 = 0.0f; xF.eM21 = 0.0f; xF.eM22 = 1.0f; xF.eDx = dX + driftOffset; xF.eDy = 0.0f;
                SetWorldTransform(hDC, &xF); 
                if (vW > 0 && vH > 0 && vI->GetWidth() > 0 && vI->GetHeight() > 0) vI->Draw(hDC, 0, (int)dY - vH, vW, vH);
                SetWorldTransform(hDC, &oldXF); SetGraphicsMode(hDC, oldMode);
            }
        }
    }
    for (const auto& v : m_landCloudVFXs) {
        int safeF = (std::max)(0, v.currentFrame);
        CImage* vI = &m_imgVfxLandCloud[(std::min)(safeF, 6)];
        if (vI && !vI->IsNull()) {
            float vfxScale = 2.0f * (isFullMapView ? cFS : mapScale) * 0.8f;
            int vW = (int)(vI->GetWidth() * vfxScale), vH = (int)(vI->GetHeight() * vfxScale);
            float dX, dY;
            if (isFullMapView) { dX = v.x * cFS + cFX; dY = v.y * cFS + cFY; }
            else { dX = (v.x - camX) * mapScale; dY = (v.y - camY) * mapScale; }
            if (vW > 0 && vH > 0 && vI->GetWidth() > 0 && vI->GetHeight() > 0) vI->Draw(hDC, (int)dX - vW / 2, (int)dY - vH, vW, vH);
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
    HitVFX s; s.x = x; s.y = y; s.angle = angle; s.currentFrame = 0; s.maxFrame = 5; s.lastTime = currentTime; s.isSlash = true; m_hitVFXs.push_back(s);
    HitVFX i; i.x = x; i.y = y; i.angle = angle; i.currentFrame = 0; i.maxFrame = 6; i.lastTime = currentTime; i.isSlash = false; m_hitVFXs.push_back(i);
}

void EffectManager::AddJumpCloudVFX(float x, float y, DWORD currentTime, float angle) {
    JumpCloudVFX c; c.x = x; c.y = y; c.angle = angle; c.currentFrame = 0; c.maxFrame = 4; c.lastTime = currentTime; m_jumpCloudVFXs.push_back(c);
}

void EffectManager::AddDustCloudVFX(float x, float y, bool isFacingRight, DWORD currentTime) {
    static int bc = 0; DustCloudVFX c; c.x = x; c.y = y; c.isFacingRight = isFacingRight; c.currentFrame = 0; c.maxFrame = 7;
    c.startTick = currentTime + (bc * 60); c.lastTime = c.startTick; m_dustCloudVFXs.push_back(c); bc = (bc + 1) % 3;
}

void EffectManager::AddLandCloudVFX(float x, float y, DWORD currentTime) {
    LandCloudVFX c; c.x = x; c.y = y; c.currentFrame = 0; c.maxFrame = 7; c.lastTime = currentTime; m_landCloudVFXs.push_back(c);
}

void EffectManager::AddPendingHit(Enemy* target, float kbForce) {
    PendingHit p; p.target = target; p.kbForce = kbForce; p.remainingFrames = 6; m_pendingHits.push_back(p);
}

bool EffectManager::HasActiveHitVFX() { return !m_hitVFXs.empty(); }
bool EffectManager::HasActiveVFX() { return !m_neonTrails.empty() || !m_hitVFXs.empty() || !m_jumpCloudVFXs.empty() || !m_dustCloudVFXs.empty() || !m_landCloudVFXs.empty(); }

