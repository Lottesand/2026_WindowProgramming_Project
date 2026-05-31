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
    if (isSolid(GetCollisionType((int)x, (int)y))) return true;
    if (isSolid(GetCollisionType((int)(x + w / 2), (int)y))) return true;
    if (isSolid(GetCollisionType((int)(x + w), (int)y))) return true;
    if (isSolid(GetCollisionType((int)x, (int)(y + h / 2)))) return true;
    if (isSolid(GetCollisionType((int)(x + w), (int)(y + h / 2)))) return true;
    if (isSolid(GetCollisionType((int)x, (int)(y + h)))) return true;
    if (isSolid(GetCollisionType((int)(x + w / 2), (int)(y + h)))) return true;
    if (isSolid(GetCollisionType((int)(x + w), (int)(y + h)))) return true;
    return false;
}

bool CheckSpecificCollision(float x, float y, float w, float h, int targetType) {
    if (GetCollisionType((int)x, (int)y) == targetType) return true;
    if (GetCollisionType((int)(x + w / 2), (int)y) == targetType) return true;
    if (GetCollisionType((int)(x + w), (int)y) == targetType) return true;
    if (GetCollisionType((int)x, (int)(y + h / 2)) == targetType) return true;
    if (GetCollisionType((int)(x + w), (int)(y + h / 2)) == targetType) return true;
    if (GetCollisionType((int)x, (int)(y + h)) == targetType) return true;
    if (GetCollisionType((int)(x + w / 2), (int)(y + h)) == targetType) return true;
    if (GetCollisionType((int)(x + w), (int)(y + h)) == targetType) return true;
    return false;
}

