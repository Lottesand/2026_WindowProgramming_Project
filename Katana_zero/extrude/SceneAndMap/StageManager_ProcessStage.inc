void StageManager::ProcessStage(int stage) {
    StageData& data = m_stageDataMap[stage];
    CImage& colMap = m_colMapImages[stage];
    CImage& objMap = m_objMapImages[stage];

    data.doors.clear();
    data.clearZones.clear();
    data.enemySpawns.clear();
    data.playerStart = { 100, 100 };
    data.stageLimitTime = (stage == 1) ? 30.0f : 60.0f;

    // --- 1. Process colMap for Player Start and Clear Zones ---
    if (!colMap.IsNull() && colMap.IsDIBSection()) {
        int w = colMap.GetWidth();
        int h = colMap.GetHeight();
        int pitch = colMap.GetPitch();
        int bpp = colMap.GetBPP() / 8;
        BYTE* pBits = (BYTE*)colMap.GetBits();

        auto IsColorMatch = [](BYTE r, BYTE g, BYTE b, int tr, int tg, int tb) {
            return abs((int)r - tr) < 30 && abs((int)g - tg) < 30 && abs((int)b - tb) < 30;
        };

        std::vector<bool> visited(w * h, false);

        for (int y = 0; y < h; y++) {
            BYTE* pRow = pBits + (y * pitch);
            for (int x = 0; x < w; x++) {
                if (visited[y * w + x]) continue;

                BYTE* pPixel = pRow + (x * bpp);
                BYTE b = pPixel[0], g = pPixel[1], r = pPixel[2];

                // [플레이어 스폰: 흰색 (255, 255, 255)]
                if (IsColorMatch(r, g, b, 255, 255, 255)) {
                    data.playerStart = { x, y };
                    visited[y * w + x] = true;
                }
                // [클리어 존: 하늘색 (0, 255, 255)]
                else if (IsColorMatch(r, g, b, 0, 255, 255)) {
                    int rectW = 0;
                    while (x + rectW < w) {
                        BYTE* pN = pRow + ((x + rectW) * bpp);
                        if (IsColorMatch(pN[2], pN[1], pN[0], 0, 255, 255)) rectW++;
                        else break;
                    }
                    int rectH = 0;
                    while (y + rectH < h) {
                        BYTE* pNRow = pBits + ((y + rectH) * pitch) + (x * bpp);
                        if (IsColorMatch(pNRow[2], pNRow[1], pNRow[0], 0, 255, 255)) rectH++;
                        else break;
                    }

                    if (rectW > 0 && rectH > 0) {
                        data.clearZones.push_back({ x, y, x + rectW, y + rectH });
                        for (int ry = y; ry < y + rectH; ry++) {
                            for (int rx = x; rx < x + rectW; rx++) {
                                visited[ry * w + rx] = true;
                            }
                        }
                    }
                }
            }
        }
    }

    // --- 2. Process objMap for Entities (Enemies, Doors) ---
    if (!objMap.IsNull() && objMap.IsDIBSection()) {
        int w = objMap.GetWidth();
        int h = objMap.GetHeight();
        int pitch = objMap.GetPitch();
        int bpp = objMap.GetBPP() / 8;
        BYTE* pBits = (BYTE*)objMap.GetBits();

        auto IsColorMatch = [](BYTE r, BYTE g, BYTE b, int tr, int tg, int tb) {
            return abs((int)r - tr) < 30 && abs((int)g - tg) < 30 && abs((int)b - tb) < 30;
        };

        std::vector<bool> visited(w * h, false);

        for (int y = 0; y < h; y++) {
            BYTE* pRow = pBits + (y * pitch);
            for (int x = 0; x < w; x++) {
                if (visited[y * w + x]) continue;

                BYTE* pPixel = pRow + (x * bpp);
                BYTE b = pPixel[0], g = pPixel[1], r = pPixel[2];

                // [적 카테고리: 노란색(255,255,0) 또는 흰색(255,255,255)] 
                bool isEnemyCategory = IsColorMatch(r, g, b, 255, 255, 0) || IsColorMatch(r, g, b, 255, 255, 255);
                
                if (isEnemyCategory) {
                    if (y + 1 < h) {
                        BYTE* pSubRow = pBits + ((y + 1) * pitch) + (x * bpp);
                        BYTE sb = pSubRow[0], sg = pSubRow[1], sr = pSubRow[2];

                        int enemyType = -1;
                        if (IsColorMatch(sr, sg, sb, 255, 0, 0)) enemyType = 1;      // 빨강: Grunt
                        else if (IsColorMatch(sr, sg, sb, 0, 255, 0)) enemyType = 0; // 초록: Gangster
                        else if (IsColorMatch(sr, sg, sb, 0, 0, 255)) enemyType = 2; // 파랑: Pomp

                        if (enemyType != -1) {
                            int rectW = 0;
                            while (x + rectW < w) {
                                BYTE* pN = pRow + ((x + rectW) * bpp);
                                if (IsColorMatch(pN[2], pN[1], pN[0], r, g, b)) rectW++;
                                else break;
                            }

                            int rectH = 1;
                            while (y + rectH < h) {
                                BYTE* pNRow = pBits + ((y + rectH) * pitch) + (x * bpp);
                                if (IsColorMatch(pNRow[2], pNRow[1], pNRow[0], sr, sg, sb)) rectH++;
                                else break;
                            }

                            data.enemySpawns.push_back({ (float)x, (float)y, (float)rectW, enemyType });

                            for (int ry = y; ry < y + rectH; ry++) {
                                for (int rx = x; rx < x + rectW; rx++) {
                                    visited[ry * w + rx] = true;
                                }
                            }
                        }
                    }
                }
                // [오브젝트 카테고리: 분홍색 (255, 0, 255)]
                else if (IsColorMatch(r, g, b, 255, 0, 255)) {
                    int rectW = 0;
                    while (x + rectW < w) {
                        BYTE* pN = pRow + ((x + rectW) * bpp);
                        if (IsColorMatch(pN[2], pN[1], pN[0], 255, 0, 255)) rectW++;
                        else break;
                    }

                    if (y + 1 < h) {
                        BYTE* pSubRow = pBits + ((y + 1) * pitch) + (x * bpp);
                        BYTE sb = pSubRow[0], sg = pSubRow[1], sr = pSubRow[2];

                        if (IsColorMatch(sr, sg, sb, 255, 0, 0)) { // 빨강: 문(Door)
                            int rectH = 1;
                            while (y + rectH < h) {
                                BYTE* pNRow = pBits + ((y + rectH) * pitch) + (x * bpp);
                                if (IsColorMatch(pNRow[2], pNRow[1], pNRow[0], sr, sg, sb)) rectH++;
                                else break;
                            }

                            data.doors.emplace_back((float)x, (float)y, (float)rectW, (float)rectH);

                            for (int ry = y; ry < y + rectH; ry++) {
                                for (int rx = x; rx < x + rectW; rx++) {
                                    visited[ry * w + rx] = true;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}