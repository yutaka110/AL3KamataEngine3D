#pragma once
#include "KamataEngine.h"
#include "Player.h"
#include "Skydome.h"
// 先頭のインクルード付近に追加
#include "Enemy.h"
#include "TitleScene.h"
#include "goal.h"
#include <vector>
enum class ScenePhase { Title, Game, Death, Clear };
class ClearScene; // 前方宣言
// ゲームシーン
class GameScene {

public:
	~GameScene();
	// 初期化
	void Initialize();

	// 更新
	void Update();

	// 描画
	void Draw();

	// スプライト
	KamataEngine::Sprite* sprite_ = nullptr;

	// 3Dモデル
	KamataEngine::Model* model_ = nullptr;

	// ワールドトランスフォーム
	KamataEngine::WorldTransform worldTransform_;

	// デバッグカメラ
	KamataEngine::DebugCamera* debugCamera_ = nullptr;

	// カメラ
	KamataEngine::Camera camera_;

	// 3Dモデル
	// KamataEngine::Model* modelSkydome_ = nullptr;

	Skydome* skydome_ = nullptr;

private:
	// テクスチャハンドル
	uint32_t textureHandle_ = 0;

	// サウンドハンドル
	uint32_t soundDataHandle_ = 0;

	// 音声再生ハンドル
	uint32_t voiceHandle_ = 0;

	// 自動キャラ
	Player* player_ = nullptr;

	// 2次元配列形式（行×列）
	std::vector<std::vector<KamataEngine::WorldTransform*>> worldTransformBlocks_;

	KamataEngine::Model* modelBlock_ = nullptr;

	std::vector<std::vector<int>> mapData_;

	// ★ デバッグカメラ有効フラグ（デフォルトOFF）
	bool isDebugCameraActive_ = false;

	// ★ 現在のフェーズ（最初はタイトル）
	ScenePhase phase_ = ScenePhase::Title;

	// ★ タイトルシーンを中で所有（委譲用）
	TitleScene* title_ = nullptr;

    // クリアシーンを中で所有（委譲用）
	ClearScene* clear_ = nullptr;

	// ▼ private: に追加
	float tileOriginX_ = 0.0f, tileOriginY_ = 0.0f;
	float tilePitchX_ = 1.0f, tilePitchY_ = 1.0f;
	float tileHalfX_ = 0.8f, tileHalfY_ = 0.8f; // cube の scale から決める

	//-----敵-----
	std::vector<std::unique_ptr<Enemy>> enemies_;

	struct DeathParticle {
		KamataEngine::WorldTransform* wt = nullptr; // ★ ポインタ化
		KamataEngine::Vector3 vel{};
		float life = 0.0f;    // 残り寿命（秒）
		float maxLife = 0.0f; // 初期寿命（秒）
	};
	std::vector<DeathParticle> deathParticles_;
	KamataEngine::Model* modelSphere_ = nullptr; // 球モデル

	// GameScene.h の private に追記
	float deathElapsed_ = 0.0f;  // 死亡演出の経過秒
	float deathMinTime_ = 0.80f; // 最低でもこの時間は Death を続ける（秒）

	// bullets
	// GameScene.h の private: 末尾あたりに追加
	struct Bullet {
		KamataEngine::Vector3 pos;
		KamataEngine::Vector3 vel; // XY で進む（Zは固定）
		float radius = 0.18f;      // 見た目小さめが気持ちいい
		int bounces = 0;           // 反射回数
		bool alive = true;
	};

	std::vector<Bullet> bullets_;

	// 弾の基本パラメータ
	float bulletSpeed_ = 6.0f; // タイル/秒
	int bulletMaxBounce_ = 3;
	float bulletSkin_ = 0.02f;   // めり込み防止
	float fireCooldown_ = 0.25f; // 連射間隔（秒）
	float fireTimer_ = 0.0f;

	// 簡易エイム：最後に押した移動キーの向きを記憶（初期は+X）
	KamataEngine::Vector3 lastAimDir_{1.0f, 0.0f, 0.0f};

	// ★ リスポーン用スポーン位置
	KamataEngine::Vector3 playerSpawn_{};
	std::vector<KamataEngine::Vector3> enemySpawns_;

	// ★ プレイヤー死亡後のリセット関数
	void ResetAfterPlayerDeath(bool forceNow = false);

	// ★ 追加：クリア後（や任意のタイミング）に、プレイヤー/敵を初期スポーンへ戻すだけの共通関数
	void ResetActorsToSpawn();

	// --------------- CLEAR パーティクル ---------------
	struct ClearParticle {
		KamataEngine::WorldTransform* wt = nullptr;
		KamataEngine::Vector3 vel{}; // x,y を使用（Zは固定）
		float life = 0.0f;
		float maxLife = 0.0f;
	};

	std::vector<ClearParticle> clearParticles_;

	// 生成＆更新（実装は .cpp）
	void SpawnClearBurst(int count, const KamataEngine::Vector3& center);
	void UpdateClearParticles(float dt);

	 // 既存: Clear用パーティクル構造体や clearParticles_ がある前提

// ---- Clear用エミッタ（常時発生制御）----
struct ClearEmitter {
    bool active = false;
    float rate = 80.0f;     // 1秒あたりの生成数（お好みで）
    float accum = 0.0f;     // 積算（小数を貯めて整数分だけ生成）
    KamataEngine::Vector3 origin{0.0f, 0.0f, 10.0f}; // 画面中央付近（正射影ならz=10等）
};
ClearEmitter clearEmitter_;

// 画面を塗る用の 1x1 白テクスチャ（無ければ0のままでもOK）
uint32_t clearTexWhite_ = 0;
int screenW_ = 1280, screenH_ = 720; // 必要なら実取得に置換



//// エミッタ更新（dtごとに発生数を決めて Spawn）
//void UpdateClearEmitter(float dt);
//// 画面切替：Clear開始/終了
//void StartClearScene();
//void FinishClearToTitle();

// ゴール（タイル3）の位置
int goalTx_ = -1, goalTy_ = -1;
Goal* goal_ = nullptr;
uint32_t goalTex_ = 0; 
//KamataEngine::Model* modelBlockGoals_ = nullptr;
KamataEngine::Sprite* titleSprite_ = nullptr;
KamataEngine::Sprite* clearSprite_ = nullptr;

// 背景
KamataEngine::Sprite* bgSprite_ = nullptr;
uint32_t bgTex_ = 0;
};