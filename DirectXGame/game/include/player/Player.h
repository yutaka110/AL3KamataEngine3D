// Player.h
#pragma once
#include "KamataEngine.h"
#include "PlayerConfig.h"
#include "PlayerTypes.h"

class Player {
public:

	// 既存APIを温存（pos あり版）
	void Initialize(KamataEngine::Model* model, uint32_t textureHandle, KamataEngine::Camera* camera, const KamataEngine::Vector3& pos);
	void Update(); // 既存呼び出し箇所をそのまま使える
	void Draw(const KamataEngine::Camera& cam);

	// アクセッサ（既存と同等）
	const KamataEngine::WorldTransform& GetWorldTransform() const { return worldTransform_; }
	KamataEngine::WorldTransform& EditWorldTransform() { return worldTransform_; }
	KamataEngine::Vector3& EditVelocity() { return velocityProxy_; } // 互換用
	bool IsGrounded() const { return data_.grounded; }
	void SetGrounded(bool g) { data_.grounded = g; }
	bool IsInvincible() const { return data_.invincible; }
    bool IsDodging()   const { return data_.state == game::player::State::Dodging; }

private:
	// 既存フィールド
	KamataEngine::WorldTransform worldTransform_;
	KamataEngine::Model* model_ = nullptr;
	uint32_t textureHandle_ = 0u;
	KamataEngine::Camera* camera_ = nullptr;

	// 新：内部状態（元の velocity_/ground 系は Data に集約）
	game::player::Data data_{};
	game::player::Params params_ = game::player::DefaultParams();

	// 互換性のための一時アクセサ（外部が EditVelocity() を使っている場合の“窓”）
	// 実体は data_.vx, data_.vy に同期します。
	KamataEngine::Vector3 velocityProxy_{0.0f, 0.0f, 0.0f};

};
