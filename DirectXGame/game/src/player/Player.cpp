// Player.cpp
#include "player/Player.h"
#include "player/Player_Input.h"
#include "player/PlayerPhysics.h"
#include "player/PlayerState.h"
#include "math/MathUtil.h"
#include <algorithm>
#include <cassert>

using KamataEngine::Vector3;
using ge3::math::MakeAffineMatrix;
using ge3::math::Multiply;
void Player::Initialize(KamataEngine::Model* model, uint32_t textureHandle, KamataEngine::Camera* camera, const Vector3& pos) {
	assert(model);
	model_ = model;
	textureHandle_ = textureHandle;
	camera_ = camera;

	worldTransform_.Initialize();
	worldTransform_.translation_ = pos;
	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();

	// Data 初期化（既存の意味合いを継承）
	data_.vx = 0.0f;
	data_.vy = 0.0f;
	data_.groundY = pos.y;
	data_.grounded = true;
	data_.coyoteCounter = 0;
	data_.jumpBufferCounter = 0;

	// 互換用プロキシ
	velocityProxy_ = {data_.vx, data_.vy, 0.0f};
}

void Player::Update() {
	// 外部が EditVelocity() を弄っていたら取り込む（互換）
	if (velocityProxy_.x != data_.vx || velocityProxy_.y != data_.vy) {
		data_.vx = velocityProxy_.x;
		data_.vy = velocityProxy_.y;
	}

	// 1) 入力
	game::player::Input in{};
	game::player::InputReader::Read(in);

	// 2) 状態（ジャンプ猶予 & 水平加減速）
	game::player::StepState(data_, in, params_);

	// 3) 物理（摩擦・重力・積分・床クランプ）
	if (in.axisX == 0) {
		game::player::ApplyFriction(data_, params_);
	}
	game::player::ApplyGravity(data_, params_);
	game::player::IntegratePosition(worldTransform_.translation_.x, worldTransform_.translation_.y, data_);

	// 仮の床（必要なら有効化）
	// game::player::ClampToFloor(worldTransform_.translation_.y, data_);

	// 行列更新
	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	if (worldTransform_.parent_) {
		worldTransform_.matWorld_ = Multiply(worldTransform_.matWorld_, worldTransform_.parent_->matWorld_);
	}
	worldTransform_.TransferMatrix();

	// 互換用プロキシへ反映
	velocityProxy_.x = data_.vx;
	velocityProxy_.y = data_.vy;
}

void Player::Draw(const KamataEngine::Camera& cam) { model_->Draw(worldTransform_, cam, textureHandle_); }
