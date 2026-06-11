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
    
    // 적 스폰 영역 색상들 (노랑, 주황, 보라, 회색) 충돌체에서 제외
    if (r >= 240 && g >= 240 && b <= 50) return 0; // 노랑
    if (r >= 240 && g >= 140 && g <= 170 && b <= 50) return 0; // 주황
    if (r >= 190 && r <= 210 && g <= 50 && b >= 190 && b <= 210) return 0; // 보라
    if (r >= 140 && r <= 160 && g >= 140 && g <= 160 && b >= 140 && b <= 160) return 0; // 회색

    if (g > 200 && r < 50 && b < 50) return 1; // Green
    if (r > 200 && g < 50 && b < 50) return 2; // Red
    if (b > 200 && r < 50 && g < 50) return 3; // Blue
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

