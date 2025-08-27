#pragma once
#include "KamataEngine.h"

// プレイヤーと同じ流儀：Initialize(model, texture, camera, pos) / Update() / Draw()
// 位置やスケールは WorldTransform に保持します。
class Goal {
public:
	// 初期化（モデル・テクスチャ・カメラ・初期位置）
	void Initialize(KamataEngine::Model* model, uint32_t textureHandle, KamataEngine::Camera* camera, const KamataEngine::Vector3& pos);

	// 毎フレーム更新（必要なら回転などをここで）
	void Update();

	// 描画
	void Draw(const KamataEngine::Camera& cam);

	// WT 参照（プレイヤーと同様）
	const KamataEngine::WorldTransform& GetWorldTransform() const { return wt_; }
	KamataEngine::WorldTransform& EditWorldTransform() { return wt_; }

private:
	// 行列ユーティリティ（GameSceneのMakeAffine相当 / 行ベクトル前提）
	static KamataEngine::Matrix4x4 MakeAffine(const KamataEngine::Vector3& s, const KamataEngine::Vector3& r, const KamataEngine::Vector3& t);

private:
	KamataEngine::WorldTransform wt_{};
	KamataEngine::Model* model_ = nullptr;
	uint32_t textureHandle_ = 0;
	KamataEngine::Camera* camera_ = nullptr;
};
