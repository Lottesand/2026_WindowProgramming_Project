#include "Physics.h"

int GetCollisionType(int targetX, int targetY) {
    CImage& imgColMap = StageManager::GetColMap();
    if (imgColMap.IsNull()) return 1;
    // 맵 범위를 벗어난 경우 처리
    if (targetX < 0 || targetX >= imgColMap.GetWidth()) return 1;
    if (targetY < 0) return 1;
    if (targetY >= imgColMap.GetHeight()) return 0; // 맵 아래쪽은 뚫려있음 (낙사 구간)
    
    COLORREF pixelColor = imgColMap.GetPixel(targetX, targetY) & 0x00FFFFFF;
    int r = GetRValue(pixelColor); 
    int g = GetGValue(pixelColor); 
    int b = GetBValue(pixelColor);
    
    if (g > 200 && r < 50 && b < 50) return 1; // Green (Wall/Ground)
    if (r > 200 && g < 50 && b < 50) return 2; // Red (Hazard/Spikes)
    if (b > 200 && r < 50 && g < 50) return 3; // Blue (Platform)
    return 0;
}

bool CheckCollision(int x, int y) {
    int type = GetCollisionType(x, y);
    return type == 1 || type == 3;
}

bool IsMapTransparent(int x, int y) {
    CImage& imgMap = StageManager::GetMap();
    if (imgMap.IsNull()) return true;
    if (x < 0 || x >= imgMap.GetWidth() || y < 0 || y >= imgMap.GetHeight()) return true;
    
    if (imgMap.GetBPP() == 32) {
        BYTE* pBits = (BYTE*)imgMap.GetBits();
        int pitch = imgMap.GetPitch();
        int bpp = imgMap.GetBPP() / 8;
        BYTE* pPixel = pBits + (y * pitch) + (x * bpp);
        if (pPixel[3] == 0) return true; // Alpha is 0
    }
    return false;
}

bool CheckMapCollision(float x, float y, float w, float h) {
    auto isSolid = [](int t) { return t == 1 || t == 3; };
    
    // Check points along the edges every 16 pixels for thoroughness
    for (float px = x; px <= x + w - 1.0f; px += 16.0f) {
        if (isSolid(GetCollisionType((int)px, (int)y))) return true;           // Top
        if (isSolid(GetCollisionType((int)px, (int)(y + h - 1.0f)))) return true; // Bottom
    }
    // Ensure far right edge is checked
    if (isSolid(GetCollisionType((int)(x + w - 1.0f), (int)y))) return true;
    if (isSolid(GetCollisionType((int)(x + w - 1.0f), (int)(y + h - 1.0f)))) return true;

    for (float py = y; py <= y + h - 1.0f; py += 16.0f) {
        if (isSolid(GetCollisionType((int)x, (int)py))) return true;           // Left
        if (isSolid(GetCollisionType((int)(x + w - 1.0f), (int)py))) return true; // Right
    }
    // Ensure midpoint is checked
    if (isSolid(GetCollisionType((int)(x + w / 2.0f), (int)(y + h / 2.0f)))) return true;

    return false;
}

bool CheckDoorCollision(float x, float y, float w, float h) {
    auto pDoors = StageManager::GetCurrentDoors();
    if (!pDoors) return false;

    RECT rect1 = { (int)x, (int)y, (int)(x + w), (int)(y + h) };
    for (const auto& door : *pDoors) {
        if (door.IsClosed()) {
            RECT rect2 = { (int)door.GetX(), (int)door.GetY(), (int)(door.GetX() + door.GetW()), (int)(door.GetY() + door.GetH()) };
            RECT overlap;
            if (IntersectRect(&overlap, &rect1, &rect2)) {
                return true;
            }
        }
    }
    return false;
}

void ResolveMapCollision(float& x, float& y, float w, float h, float oldX, float oldY) {
    if (!CheckMapCollision(x, y, w, h)) return;

    bool collidesX = CheckMapCollision(x, oldY, w, h);
    bool collidesY = CheckMapCollision(oldX, y, w, h);

    if (collidesX && !collidesY) {
        x = oldX;
    }
    else if (!collidesX && collidesY) {
        y = oldY;
    }
    else {
        // Both or neither (corner collision). Revert both to be safe.
        x = oldX;
        y = oldY;
    }
    
    // Final emergency push-out if still stuck
    if (CheckMapCollision(x, y, w, h)) {
        // Try to find a free spot nearby
        for (int i = 1; i <= 5; i++) {
            if (!CheckMapCollision(x - i, y, w, h)) { x -= i; return; }
            if (!CheckMapCollision(x + i, y, w, h)) { x += i; return; }
            if (!CheckMapCollision(x, y - i, w, h)) { y -= i; return; }
            if (!CheckMapCollision(x, y + i, w, h)) { y += i; return; }
        }
    }
}

bool CheckSpecificCollision(float x, float y, float w, float h, int targetType) {
    int x1 = (int)x, x2 = (int)(x + w / 2), x3 = (int)(x + w - 1);
    int y1 = (int)y, y2 = (int)(y + h / 2), y3 = (int)(y + h - 1);

    if (GetCollisionType(x1, y1) == targetType) return true;
    if (GetCollisionType(x2, y1) == targetType) return true;
    if (GetCollisionType(x3, y1) == targetType) return true;
    if (GetCollisionType(x1, y2) == targetType) return true;
    if (GetCollisionType(x3, y2) == targetType) return true;
    if (GetCollisionType(x1, y3) == targetType) return true;
    if (GetCollisionType(x2, y3) == targetType) return true;
    if (GetCollisionType(x3, y3) == targetType) return true;
    return false;
}

