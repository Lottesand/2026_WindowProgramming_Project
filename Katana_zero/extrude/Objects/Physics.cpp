#include "Physics.h"

int GetCollisionType(int targetX, int targetY) {
    CImage& imgColMap = StageManager::GetColMap();
    if (imgColMap.IsNull()) return 1;
    if (targetX < 0 || targetY < 0 || targetX >= imgColMap.GetWidth() || targetY >= imgColMap.GetHeight()) return 1;
    
    COLORREF pixelColor = imgColMap.GetPixel(targetX, targetY) & 0x00FFFFFF;
    int r = GetRValue(pixelColor); 
    int g = GetGValue(pixelColor); 
    int b = GetBValue(pixelColor);
    
    if (g > 200 && r < 50 && b < 50) return 1; // Green
    if (r > 200 && g < 50 && b < 50) return 2; // Red
    if (b > 200 && r < 50 && g < 50) return 3; // Blue
    return 0;
}

bool CheckCollision(int x, int y) {
    int type = GetCollisionType(x, y);
    return type == 1 || type == 3;
}

bool CheckMapCollision(float x, float y, float w, float h) {
    auto isSolid = [](int t) { return t == 1 || t == 3; };
    int x1 = (int)x, x2 = (int)(x + w / 2), x3 = (int)(x + w - 1);
    int y1 = (int)y, y2 = (int)(y + h / 2), y3 = (int)(y + h - 1);

    if (isSolid(GetCollisionType(x1, y1))) return true;
    if (isSolid(GetCollisionType(x2, y1))) return true;
    if (isSolid(GetCollisionType(x3, y1))) return true;
    if (isSolid(GetCollisionType(x1, y2))) return true;
    if (isSolid(GetCollisionType(x3, y2))) return true;
    if (isSolid(GetCollisionType(x1, y3))) return true;
    if (isSolid(GetCollisionType(x2, y3))) return true;
    if (isSolid(GetCollisionType(x3, y3))) return true;
    return false;
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

