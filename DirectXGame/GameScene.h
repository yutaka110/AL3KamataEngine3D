#pragma once
#include "KamataEngine.h"
#include "Player.h"
#include "Skydome.h"
// 先頭のインクルード付近に追加
#include <vector>
#include "TitleScene.h"
#include "Enemy.h"
enum class ScenePhase { Title, Game, Death };

//ゲームシーン
class GameScene
{
	
	public:
	    ~GameScene();
		// 初期化
		void Initialize();

		// 更新
		void Update();

		// 描画
		void Draw();

		//スプライト
	    KamataEngine::Sprite* sprite_ = nullptr;

		//3Dモデル
	    KamataEngine::Model* model_ = nullptr;

		//ワールドトランスフォーム
	    KamataEngine::WorldTransform worldTransform_;

		// デバッグカメラ
	    KamataEngine::DebugCamera* debugCamera_ = nullptr;


		//カメラ
	    KamataEngine::Camera camera_;

		// 3Dモデル
	    //KamataEngine::Model* modelSkydome_ = nullptr;

		Skydome* skydome_ = nullptr;


	private:

		//テクスチャハンドル
	    uint32_t textureHandle_ = 0;

		//サウンドハンドル
	    uint32_t soundDataHandle_ = 0;

		//音声再生ハンドル
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
};