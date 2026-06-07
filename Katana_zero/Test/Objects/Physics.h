#pragma once
#include <windows.h>
#include <atlimage.h>
#include "../SceneAndMap/StageManager.h"


extern const int VIRTUAL_WIDTH;
extern const int VIRTUAL_HEIGHT;

int GetCollisionType(int x, int y);
bool CheckCollision(int x, int y);
bool IsMapTransparent(int x, int y);
bool CheckMapCollision(float x, float y, float w, float h);
bool CheckDoorCollision(float x, float y, float w, float h);
void ResolveMapCollision(float& x, float& y, float w, float h, float oldX, float oldY);
bool CheckSpecificCollision(float x, float y, float w, float h, int targetType);

