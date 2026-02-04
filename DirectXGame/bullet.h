#pragma once
// =========================
// Wiiタンク風：弾（生成・移動・反射・ヒット）最小実装
// =========================
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

// ---- 基本設定 ----
static const float kBulletSpeed = 12.0f; // 1ステップ(=同時実行サイクルの小刻みサブステップ)あたりの速度
static const float kBulletRadius = 0.35f;
static const int kBulletMax = 64;
static const int kBulletMaxBounce = 6;   // 反射上限
static const float kTankRadius = 0.9f;   // 戦車の当たり（円近似でOK）
static const float kMuzzleOffset = 1.2f; // 砲口の前に少しオフセットして発生させる
static const float kStepDt = 1.0f;       // サブステップ時間（固定でOK。速度で調整）

// ---- 矩形（AABB） ----
struct AABB {
	float minx, miny, maxx, maxy;
};

// ---- 弾 ----
struct Bullet {
	bool alive;
	float x, y;
	float vx, vy;
	int bounces;
};

static Bullet gBullets[kBulletMax];

// ---- ステージ外枠 & 内壁群 ----
static AABB gArena = {-8.0f, -6.0f, +8.0f, +6.0f}; // 画面内のプレイ領域
static std::vector<AABB> gWalls;                   // 必要なら push_back で追加

// ---- ユーティリティ（最小限） ----
inline float Dot(float ax, float ay, float bx, float by) { return ax * bx + ay * by; }
inline float Len(float x, float y) { return std::sqrt(x * x + y * y); }
inline void Norm(float& x, float& y) {
	float l = Len(x, y);
	if (l > 0) {
		x /= l;
		y /= l;
	}
}

// 円(弾) vs 円(戦車) ヒット
inline bool HitCircleCircle(float ax, float ay, float ar, float bx, float by, float br) {
	float dx = ax - bx, dy = ay - by;
	float rr = (ar + br) * (ar + br);
	return (dx * dx + dy * dy) <= rr;
}

// AABBへ侵入したか（中心と半径でザックリ判定）
inline bool PenetrateCircleAABB(float cx, float cy, float r, const AABB& b) {
	// 最近接点を取って距離で判定
	float nx = std::clamp(cx, b.minx, b.maxx);
	float ny = std::clamp(cy, b.miny, b.maxy);
	float dx = cx - nx, dy = cy - ny;
	return (dx * dx + dy * dy) <= (r * r);
}

// 弾の生成（向きはラジアン角。戦車中心(px,py)と砲塔角dirRad）
static void SpawnBullet(float px, float py, float dirRad) {
	// 空き弾を探す
	int slot = -1;
	for (int i = 0; i < kBulletMax; i++) {
		if (!gBullets[i].alive) {
			slot = i;
			break;
		}
	}
	if (slot < 0)
		return;

	// 進行ベクトル
	float vx = std::cos(dirRad);
	float vy = std::sin(dirRad);

	// 砲口から少し前に出して自機衝突を避ける
	float sx = px + vx * (kMuzzleOffset + kBulletRadius);
	float sy = py + vy * (kMuzzleOffset + kBulletRadius);

	gBullets[slot].alive = true;
	gBullets[slot].x = sx;
	gBullets[slot].y = sy;
	gBullets[slot].vx = vx * kBulletSpeed;
	gBullets[slot].vy = vy * kBulletSpeed;
	gBullets[slot].bounces = 0;
}

// 弾の移動＆反射＆ヒット処理（1サブステップ）
static void TickBullets_OneSubstep(
    float tankX, float tankY, // プレイヤー戦車位置（敵にも流用可）
    bool checkHitVsPlayer,    // この呼び出しで当たり判定対象にするか
    bool& outPlayerHit        // ここでtrueになったら撃破
) {
	for (int i = 0; i < kBulletMax; i++) {
		Bullet& b = gBullets[i];
		if (!b.alive)
			continue;

		// 次位置
		float nx = b.x + b.vx * kStepDt;
		float ny = b.y + b.vy * kStepDt;

		// --- 外枠での反射（簡易：軸反転） ---
		bool reflected = false;
		if (nx - kBulletRadius < gArena.minx) {
			nx = gArena.minx + kBulletRadius;
			b.vx = -b.vx;
			reflected = true;
		}
		if (nx + kBulletRadius > gArena.maxx) {
			nx = gArena.maxx - kBulletRadius;
			b.vx = -b.vx;
			reflected = true;
		}
		if (ny - kBulletRadius < gArena.miny) {
			ny = gArena.miny + kBulletRadius;
			b.vy = -b.vy;
			reflected = true;
		}
		if (ny + kBulletRadius > gArena.maxy) {
			ny = gArena.maxy - kBulletRadius;
			b.vy = -b.vy;
			reflected = true;
		}

		// --- 内壁での反射（侵入していたら浅い方の軸で反転） ---
		for (const AABB& w : gWalls) {
			// まずは「次位置」で侵入チェック
			if (!PenetrateCircleAABB(nx, ny, kBulletRadius, w))
				continue;

			// 直前位置→次位置の移動でどちらの面に近いかを見て片軸反転
			float px = b.x, py = b.y;
			float dx = nx - px, dy = ny - py;

			// x方向に近い面へめり込みが強ければx反転、y側ならy反転
			float depL = (w.minx - (nx - kBulletRadius)); // 左面超え(正なら超え)
			float depR = ((nx + kBulletRadius) - w.maxx); // 右面超え
			float depD = (w.miny - (ny - kBulletRadius)); // 下
			float depU = ((ny + kBulletRadius) - w.maxy); // 上

			// どれが最大侵入か大まかに判断
			float ax = std::max(depL, depR);
			float ay = std::max(depD, depU);

			if (ax > ay) {
				b.vx = -b.vx;
				// はみ出しを押し戻す（左右どちらから来たかで位置補正）
				if (depL > depR)
					nx = w.minx - (kBulletRadius);
				else
					nx = w.maxx + (kBulletRadius);
			} else {
				b.vy = -b.vy;
				if (depD > depU)
					ny = w.miny - (kBulletRadius);
				else
					ny = w.maxy + (kBulletRadius);
			}
			reflected = true;
			break;
		}

		if (reflected) {
			b.bounces++;
			if (b.bounces > kBulletMaxBounce) {
				b.alive = false;
				continue;
			}
		}

		// 位置確定
		b.x = nx;
		b.y = ny;

		// プレイヤー当たり（敵ターン用にフラグで切替）
		if (checkHitVsPlayer && HitCircleCircle(b.x, b.y, kBulletRadius, tankX, tankY, kTankRadius)) {
			outPlayerHit = true;
			b.alive = false;
		}
	}
}

// =========================
// 行動実行フェーズでの使い方（例）
// =========================
//
// 1サイクルの中で：
//   (A) 「射撃」アクションがあれば SpawnBullet(px,py,dirRad);
//   (B) 弾のサブステップを数回まわす（跳弾の安定化用に3〜6回推奨）
//
// ※ プレイヤーと敵で別々に Hit 判定を回す（フレンドリーファイア仕様に合わせて調整）
static void ResolvePhase_DoBullets(
    float playerX, float playerY, float playerDir, bool playerChoseShoot, float enemyX, float enemyY, float enemyDir, bool enemyChoseShoot, bool& outPlayerDown, bool& outEnemyDown) {
	// (A) 同時にスポーン（同時実行の“同時”感を担保）
	if (playerChoseShoot)
		SpawnBullet(playerX, playerY, playerDir);
	if (enemyChoseShoot)
		SpawnBullet(enemyX, enemyY, enemyDir);

	// (B) サブステップで物理更新（反射の抜けを減らす）
	outPlayerDown = false;
	outEnemyDown = false;

	const int kSubSteps = 4; // 必要なら調整
	for (int s = 0; s < kSubSteps; s++) {
		// まずプレイヤー被弾チェック（敵弾が主対象だが、共通データなのでフラグで処理）
		bool pHit = false, eHit = false;
		TickBullets_OneSubstep(playerX, playerY, /*checkHitVsPlayer=*/true, pHit);
		TickBullets_OneSubstep(enemyX, enemyY, /*checkHitVsPlayer=*/true, eHit);
		outPlayerDown |= pHit;
		outEnemyDown |= eHit;
	}
}

// =========================
// 例：行動実行ステップで呼ぶ
// =========================
// （どこかの“同時実行ステップ”ループ内）
//
// bool playerShoot = (playerAction == SHOOT);
// bool enemyShoot  = (enemyAction  == SHOOT);
// bool playerDown=false, enemyDown=false;
// ResolvePhase_DoBullets(player.pos.x, player.pos.y, player.dir, playerShoot,
//                        enemy.pos.x,  enemy.pos.y,  enemy.dir,  enemyShoot,
//                        playerDown, enemyDown);
//
// if (playerDown || enemyDown) { /* ラウンド終了→即リスタート */ }
//
// 描画：gBullets[i].alive の球体を (x,y, z=0) に置けばOK（正射影）
