#include "GameScene.h"
using namespace KamataEngine;

GameScene::~GameScene() {
	delete sprite_;
	delete model_;
	// 自キャラの解放
	delete player_;
	delete debugCamera_;
}

void GameScene::Initialize() {
	// ファイル名を指定してテクスチャを読み込む
	textureHandle_ = TextureManager::Load("mario.jpg");

	// サウンドデータを読み込む
	soundDataHandle_ = Audio::GetInstance()->LoadWave("mokugyo.wav");

	// 音声再生
	voiceHandle_ = Audio::GetInstance()->PlayWave(soundDataHandle_, true);

	Audio::GetInstance()->PlayWave(soundDataHandle_);

	// スプライトインスタンスの生成
	sprite_ = Sprite::Create(textureHandle_, {100, 50});

	// 3Dモデルの生成
	model_ = Model::Create();

	// ワールドトランスファームの初期化
	worldTransform_.Initialize();

	// デバッグカメラの生成
	debugCamera_ = new DebugCamera(1280,720);

	// カメラの初期化
	camera_.Initialize();

	// 自キャラの生成
	player_ = new Player();
	// 自キャラの初期化
	player_->Initialize(model_, textureHandle_, &camera_);
}

void GameScene::Update() {
	// スペースキーを押した瞬間
	if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {

		// 音声停止
		Audio::GetInstance()->StopWave(voiceHandle_);
	}

	// スプライトの今の座標を取得
	Vector2 position = sprite_->GetPosition();

	// 座標を{2,1}移動
	position.x += 2.0f;
	position.y += 1.0f;

	// 移動した座標をスプライトに反映
	sprite_->SetPosition(position);

	// 自キャラの更新
	player_->Update();

	// デバッグカメラの更新
	debugCamera_->Update();
}

void GameScene::Draw() {
	// DirectXCommonインスタンスの取得
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();

	// スプライト描画前処理
	Sprite::PreDraw(dxCommon->GetCommandList());

	sprite_->Draw();

	// スプライト描画処理
	Sprite::PostDraw();

	// 3Dモデル描画前処理
	Model::PreDraw(dxCommon->GetCommandList());

	//model_->Draw(worldTransform_, debugCamera_->GetCamera(), textureHandle_);

	if (player_) {
		// 自キャラの描画
		player_->Draw();
	}

	// 3Dモデル描画後処理
	Model::PostDraw();
}
