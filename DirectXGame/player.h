#pragma once
#include "kamataEngine.h"
class Player {
	public:
	// ★ pos を追加
	void Initialize(KamataEngine::Model* model, uint32_t textureHandle, KamataEngine::Camera* camera, const KamataEngine::Vector3& pos);
	void Update();
	void Draw(const KamataEngine::Camera& cam);
	// ▼ Player クラスの public: に1行追加
	const KamataEngine::WorldTransform& GetWorldTransform() const { return worldTransform_; }

	
	private:
	// ワールド変換データ
    KamataEngine::WorldTransform worldTransform_;
	// モデル
	KamataEngine::Model* model_ = nullptr;
	// テクスチャハンドル
	uint32_t textureHandle_ = 0u;

	KamataEngine::Camera* camera_ = nullptr;

	// ---- 物理（慣性） ----
	// 速度ベクトル（x,yのみ使用。zは固定）
	KamataEngine::Vector3 velocity_{0.0f, 0.0f, 0.0f};

	// 地面のY（暫定フロア。タイル衝突導入までの仮実装）
	float groundY_ = 0.0f;
	bool grounded_ = true;

	// コヨーテタイム / ジャンプバッファ（フレーム数カウンタ）
	int coyoteCounter_ = 0;
	int jumpBufferCounter_ = 0;
};