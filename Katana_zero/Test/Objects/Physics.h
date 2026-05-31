#pragma once
#include <windows.h>
#include <atlimage.h>
#include "../SceneAndMap/StageManager.h"

// 臾쇰━ 愿???곸닔
extern const int VIRTUAL_WIDTH;
extern const int VIRTUAL_HEIGHT;

int GetCollisionType(int x, int y);
bool CheckCollision(int x, int y);
bool CheckMapCollision(float x, float y, float w, float h);
bool CheckSpecificCollision(float x, float y, float w, float h, int targetType);

