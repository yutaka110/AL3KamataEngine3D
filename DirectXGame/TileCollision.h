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
	float halfX = 0.5f;                                  // ★ 追加：ブロック見た目の半径（X）
	float halfY = 0.5f;                                  // ★ 追加：ブロック見た目の半径（Y）
	const std::vector<std::vector<int>>* grid = nullptr; // 0:空白, 1:ブロック...
};

// プレイヤーAABB半径（見た目に合わせて微調整）
inline KamataEngine::Vector3 PlayerHalfExtents() { return {0.9f,0.9f, 0.0f}; }
// めり込み・ビリつき防止の隙間
inline float SkinWidth() { return 0.008f; }

// 衝突解決（X→Y）。player の位置/速度/grounded を更新する
void ResolvePlayerVsTilemap(Player& player, const TileField& tf);

// 既存の宣言の下に追記
class Enemy; // 前方宣言

// 敵の当たり（プレイヤーより少し小さめ推奨）
inline KamataEngine::Vector3 EnemyHalfExtents() { return {0.9f, 0.9f, 0.0f}; }

// 敵とタイルの衝突解決（X→Y）。grounded/velocity を更新し、行列も転送する
void ResolveEnemyVsTilemap(Enemy& enemy, const TileField& tf);

// ▼ これを TileCollision.h の末尾あたりに追記
inline bool IsSolidTile(const std::vector<std::vector<int>>& g, int y, int x) { return (y >= 0 && x >= 0 && y < (int)g.size() && x < (int)g[0].size() && g[y][x] != 0); }
