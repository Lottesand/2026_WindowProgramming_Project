#pragma once
#include <windows.h>
#include <atlimage.h>

// 전역 맵 관련 변수 (FileName.cpp에서 관리)
extern CImage imgMap;
extern CImage imgColMap;

// 물리 관련 상수
extern const int VIRTUAL_WIDTH;
extern const int VIRTUAL_HEIGHT;

int GetCollisionType(int x, int y);
bool CheckCollision(int x, int y);
bool CheckMapCollision(float x, float y, float w, float h);
bool CheckSpecificCollision(float x, float y, float w, float h, int targetType);
