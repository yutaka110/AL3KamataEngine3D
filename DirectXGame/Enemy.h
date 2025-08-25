#pragma once
#include "kamataEngine.h"

class Enemy {
public:
	void Initialize(KamataEngine::Model* model, uint32_t textureHandle, const KamataEngine::Vector3& spawn);
	void Update(float dt);
	void Draw(const KamataEngine::Camera& cam);

	// 衝突器から触る用
	const KamataEngine::WorldTransform& GetWorldTransform() const { return worldTransform_; }
	KamataEngine::WorldTransform& EditWorldTransform() { return worldTransform_; }
	KamataEngine::Vector3& EditVelocity() { return velocity_; }
	bool IsGrounded() const { return grounded_; }
	void SetGrounded(bool g) { grounded_ = g; }

	// 振る舞いパラメータ
	void SetSpeed(float s) { walkSpeed_ = s; }

private:
	KamataEngine::WorldTransform worldTransform_;
	KamataEngine::Model* model_ = nullptr;
	uint32_t textureHandle_ = 0u;

	// 物理
	KamataEngine::Vector3 velocity_{0, 0, 0};
	bool grounded_ = false;

	// AI
	int dir_ = -1;           // -1: 左 / +1: 右
	float walkSpeed_ = 2.0f; // ユニット/秒
};
