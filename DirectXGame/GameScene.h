// =============================================
// File: GameScene.h （置き換え）
// =============================================
#pragma once
#include "KamataEngine.h"
#include <algorithm>
#include <cmath>
#include <deque>
#include <vector>
#include "stageEditor.h"   // 既存のStageEditor


// 行動の種類
enum class Act { MOVE_FWD, ROT_L, ROT_R, SHOOT };

// 新:
struct BoxXZ {
	float minx, minz, maxx, maxz; // 壁のAABB（XZのみ）
};

// 弾（XZ 平面でだけ動く）
struct BulletXZ {
	float x{}, z{};   // 位置（XZ）
	float vx{}, vz{}; // 速度（XZ）
	bool alive{true};
	std::unique_ptr<KamataEngine::WorldTransform> wt; // ← 追加
};

// タンク（XZ平面のみで管理）
struct TankXZ {
	KamataEngine::WorldTransform wt; // 描画用（Box）
	float x{}, z{};                  // 位置（XZ）
	int dir4{0};                     // 0:+X, 1:+Z, 2:-X, 3:-Z（90°単位）
	bool alive{true};
	std::deque<Act> queue; // 行動予約（最大3）
};



class GameScene {
public:
	~GameScene();
	void Initialize();
	void Update();
	void Draw();

private:
	// --- 定数（必要なら好みで調整） ---
	const float CELL = 1.0f;         // 1マスの幅
	const int MAP_W = 10;            // マップ横マス
	const int MAP_H = 10;            // マップ縦マス
	const int STEP_N = 8;            // 1手の移動を分割するミニフレーム数
	const float BULLET_SPEED = 6.0f; // 弾速（セル/手）
	const float BULLET_R = 0.12f;    // 弾半径
	const float TANK_HALF_W = 0.4f;  // タンク当たり（半幅）
	const float TANK_HALF_D = 0.4f;  // タンク当たり（半奥行）
	const float Y_LIFT = 0.5f;       // モデルの持ち上げ量（見映え）
	                                 // --- モデル／描画 ---
	                                 // GameScene.h の private 定数群に追加
	const float CAM_H = 12.0f;       // カメラの高さ（好みで調整）

	KamataEngine::Model* cubeModel_ = nullptr; // 壁／タンク／弾 全部 Cube でOK
	KamataEngine::Camera camera_{};            // 真上固定（正射影が無い場合は高所からの見下ろし）

	// --- ゲーム要素 ---
	std::vector<BoxXZ> walls_{};
	std::vector<std::unique_ptr<KamataEngine::WorldTransform>> wallWts_; // ← 追加
	TankXZ player_{};
	TankXZ enemy_{};
	std::vector<BulletXZ> bullets_{};

	// --- ターン管理 ---
	enum class Phase { Reserve, Execute, Result };
	Phase phase_ = Phase::Reserve;

	int execStep_ = 0; // 0..2（各ターンで最大3手）
	int execMini_ = 0; // 0..STEP_N-1（ミニフレーム進行）

	// --- 入力（予約UI） ---
	int selSlot_ = 0; // 予約スロット現在位置 0..2

	 ge3::stage::StageEditor editor_; // マップチップ情報（ID配列）
	// ★ 所有権を持たせる
	std::vector<std::unique_ptr<KamataEngine::Model>> tileModels_;
	float cellSize_ = 1.0f;          // 1マスの幅（X/Z方向）

private:
	// ユーティリティ
	static void DirVec(int dir4, float& dx, float& dz) {
		switch (dir4 & 3) {
		case 0:
			dx = +1;
			dz = 0;
			break; // +X
		case 1:
			dx = 0;
			dz = +1;
			break; // +Z
		case 2:
			dx = -1;
			dz = 0;
			break; // -X
		default:
			dx = 0;
			dz = -1;
			break; // -Z
		}
	}
	static float Clamp(float v, float a, float b) { return (v < a) ? a : (v > b) ? b : v; }

	// 予約UI（キーボード簡易版 + ImGui）
	void HandleReserveInput();

	// 敵AI：シンプルに3手埋める
	void BuildEnemyOrder();

	// 実行フェーズの1ミニフレームを進める（戻り値：決着したらtrue）
	bool TickExecuteMiniFrame();

	// MOVE の1ミニフレーム
	void MoveOneMini(TankXZ& t);

	// 弾の更新＋壁反射＋命中（戻り値：誰か死んだらtrue）
	bool UpdateBulletsMini();

	// 反射（壁は軸揃えAABBなので符号反転のみ）
	void ReflectBullet(BulletXZ& b, float prevx, float prevz);

	// 衝突ヘルパ
	bool HitBoxExpanded(float x, float z, float rx, float rz); // 円中心 vs 膨張AABB（壁）
	bool HitTank(const BulletXZ& b, const TankXZ& t);

	// 描画WTを座標に反映
	void SyncWT_Tank(TankXZ& t);
	// ↓ 既存宣言の置き換え/追加
	// 新:
	void MakeWallWT(const BoxXZ& w, KamataEngine::WorldTransform& out);
	// 参照渡しでWTを更新する関数（値返し禁止）
	void BuildWallWT(const BoxXZ& w, KamataEngine::WorldTransform& out);
	void DrawCube(const KamataEngine::WorldTransform& wt, const KamataEngine::Camera& cam);
};