#include "Physics.h"

int GetCollisionType(int targetX, int targetY) {
    if (imgColMap.IsNull()) return 1;
    if (targetX < 0 || targetY < 0 || targetX >= imgColMap.GetWidth() || targetY >= imgColMap.GetHeight()) return 1;
    COLORREF pixelColor = imgColMap.GetPixel(targetX, targetY);
    int r = GetRValue(pixelColor); int g = GetGValue(pixelColor); int b = GetBValue(pixelColor);
    if (r == 0 && g == 255 && b == 0) return 1;
    if (r == 255 && g == 0 && b == 0) return 2;
    if (r == 0 && g == 0 && b == 255) return 3;
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
