#include"player.h"

void Player::Initialize(KamataEngine::Model* model, uint32_t textureHandle, KamataEngine::Camera* camera) {
	//プレイヤーの初期化処理

	//NULLポインタチェック
	assert(model);

	 model_ = model;
	textureHandle_ = textureHandle;
	worldTransform_.Initialize();

	//引数の内容をメンバ変数に記録
	camera_ = camera;
}

void Player::Update() {
	//プレイヤーの更新処理
	//  行列を定数バッファに転送
	worldTransform_.TransferMatrix();
}	

void Player::Draw() {
	//プレイヤーの描画処理
	//  3Dモデルを描画
	model_->Draw(worldTransform_,*camera_, textureHandle_);
}	