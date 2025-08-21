#include"player.h"
#include "matrix.h"
#include "KamataEngine.h"
#include <algorithm>
#include <cmath>

using KamataEngine::Input;

void Player::Initialize(KamataEngine::Model* model, uint32_t textureHandle, KamataEngine::Camera* camera, const KamataEngine::Vector3& pos) {
	assert(model);

	model_ = model;
	textureHandle_ = textureHandle;
	camera_ = camera;
	

	worldTransform_.Initialize();
	worldTransform_.translation_ = pos; // ★ 引数の座標をセット
	worldTransform_.matWorld_ = MakeAffine(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();

	// 物理初期化
	velocity_ = {0.0f, 0.0f, 0.0f};
	groundY_ = pos.y; // ★ いまは初期位置を地面とみなす（後でタイル衝突へ置換）
	grounded_ = true;
	coyoteCounter_ = 0;
	jumpBufferCounter_ = 0;
}

void Player::Update() {
	auto* in = Input::GetInstance();

	// ======= パラメータ（好みで調整）=======
	// 水平
	const float ACCEL_X = 0.05f;         // 加速
	const float BRAKE_X = 0.08f;         // 逆方向入力時の減速（強め）
	const float MAX_SPEED_X = 0.2f;     // 最高速度
	const float FRICTION_GROUND = 0.88f; // 地上摩擦（指数減衰）
	const float FRICTION_AIR = 0.9f;    // 空中摩擦（弱め）
	// 垂直（＋Yが下向き想定：重力は +、ジャンプは −）
	const float GRAVITY = -0.25f;
	const float JUMP_VELOCITY = 2.2f; // 上向きへ初速（yマイナス）
	const float MAX_FALL_SPEED = -0.1f; // 最大落下速度
	const int COYOTE_FRAMES = 6;       // 地面離れてもこの間はジャンプOK
	const int JUMPBUF_FRAMES = 6;      // 押してから地面着地までの猶予

	// ======= 入力 =======
	// 水平：左右のみ（↑↓はジャンプ専用にして自由移動はしない）
	int ix = 0;
	if (in->PushKey(DIK_LEFT) || in->PushKey(DIK_A))
		ix -= 1;
	if (in->PushKey(DIK_RIGHT) || in->PushKey(DIK_D))
		ix += 1;

	// ジャンプトリガ（スペース / W / ↑）
	const bool jumpPressed = in->TriggerKey(DIK_SPACE) || in->TriggerKey(DIK_W) || in->TriggerKey(DIK_UP);

	if (jumpPressed)
		jumpBufferCounter_ = JUMPBUF_FRAMES;

	// ======= 水平速度更新（慣性＋ブレーキ）=======
	if (ix != 0) {
		// 進行方向と入力が同向？逆向？（1次元なので符号で判定）
		const bool sameDir = (velocity_.x == 0.0f) ? true : ((velocity_.x > 0.0f) == (ix > 0));
		const float a = sameDir ? ACCEL_X : BRAKE_X;
		velocity_.x += a * static_cast<float>(ix);
	} else {
		// 摩擦（地上/空中で係数切替）
		velocity_.x *= grounded_ ? FRICTION_GROUND : FRICTION_AIR;
		if (std::abs(velocity_.x) < 1e-3f)
			velocity_.x = 0.0f;
	}
	// 最高速クランプ
	velocity_.x = (std::max)(-MAX_SPEED_X, (std::min)(MAX_SPEED_X, velocity_.x));

	// ======= 垂直：重力・ジャンプ =======
	// コヨーテタイム管理
	if (grounded_)
		coyoteCounter_ = COYOTE_FRAMES;
	else if (coyoteCounter_ > 0)
		--coyoteCounter_;

	// ジャンプ判定（バッファ × コヨーテ）
	if (jumpBufferCounter_ > 0 && (grounded_ || coyoteCounter_ > 0)) {
		velocity_.y = JUMP_VELOCITY;
		grounded_ = false;
		jumpBufferCounter_ = 0;
	} else if (jumpBufferCounter_ > 0) {
		// まだ空中：カウントダウン
		--jumpBufferCounter_;
	}

	// 重力
	velocity_.y += GRAVITY;
	if (velocity_.y < MAX_FALL_SPEED)
		velocity_.y = MAX_FALL_SPEED;

	// ======= 位置更新 =======
	worldTransform_.translation_.x += velocity_.x;
	worldTransform_.translation_.y += velocity_.y;

	//// ======= 簡易・地面当たり判定（フロアYで止める）=======
	//if (worldTransform_.translation_.y <= groundY_) { // ＋Yが下向き想定
	//	worldTransform_.translation_.y = groundY_;
	//	if (velocity_.y < 0.0f)
	//		velocity_.y = 0.0f;
	//	grounded_ = true;
	//} else {
	//	grounded_ = false;
	//}

	// ======= 行列を組んで転送 =======
	worldTransform_.matWorld_ = MakeAffine(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	if (worldTransform_.parent_) {
		worldTransform_.matWorld_ = Multiply(worldTransform_.matWorld_, worldTransform_.parent_->matWorld_);
	}
	worldTransform_.TransferMatrix();
}


// player.cpp
void Player::Draw(const KamataEngine::Camera& cam) { model_->Draw(worldTransform_, cam, textureHandle_); }