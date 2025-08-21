#pragma once
#include "kamataEngine.h"
#include "player.h"
#include <vector>

// タイル座標系（見た目の並びに合わせる：中心=origin + pitch*index）
struct TileField {
	float originX = 0.0f;
	float originY = 0.0f;
	float pitchX = 1.0f; // ★ AABBは pitch/2 で作る
	float pitchY = 1.0f;
	const std::vector<std::vector<int>>* grid = nullptr; // 0:空白, 1:ブロック...
};

// プレイヤーAABB半径（見た目に合わせて微調整）
inline KamataEngine::Vector3 PlayerHalfExtents() { return {0.45f, 0.90f, 0.0f}; }
// めり込み・ビリつき防止の隙間
inline float SkinWidth() { return 0.008f; }

// 衝突解決（X→Y）。player の位置/速度/grounded を更新する
void ResolvePlayerVsTilemap(Player& player, const TileField& tf);
