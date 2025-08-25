#include "Enemy.h"
#include "matrix.h"
#include <algorithm>

using namespace KamataEngine;

void Enemy::Initialize(Model* model, uint32_t textureHandle, const Vector3& spawn) {
	model_ = model;
	textureHandle_ = textureHandle;

	worldTransform_.Initialize();
	worldTransform_.translation_ = spawn;
	worldTransform_.scale_ = {1, 1, 1};

	worldTransform_.matWorld_ = MakeAffine(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();
}

void Enemy::Update(float dt) {
	// パラメータ
	const float GRAVITY = -0.05f; // +Yが上想定：重力は下向き
	const float MAX_FALL = -0.1f;

	// 地上なら歩行、空中は慣性のまま
	if (grounded_)
		velocity_.x = dir_ * walkSpeed_;
	// 重力
	velocity_.y += GRAVITY * dt;
	if (velocity_.y < MAX_FALL)
		velocity_.y = MAX_FALL;

	// 位置更新（衝突で補正される想定）
	worldTransform_.translation_.x += velocity_.x * dt;
	worldTransform_.translation_.y += velocity_.y * dt;

	// 行列
	worldTransform_.matWorld_ = MakeAffine(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();
}

void Enemy::Draw(const Camera& cam) { model_->Draw(worldTransform_, cam, textureHandle_); }
