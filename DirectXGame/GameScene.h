#pragma once
#include "KamataEngine.h"
#include "Player.h"
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
	private:

		//テクスチャハンドル
	    uint32_t textureHandle_ = 0;

		//サウンドハンドル
	    uint32_t soundDataHandle_ = 0;

		//音声再生ハンドル
	    uint32_t voiceHandle_ = 0;

		// 自動キャラ
	    Player* player_ = nullptr;
	    
	
};