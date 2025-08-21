#pragma once
#include "KamataEngine.h"

// 内側から見る巨大な球（スカイドーム）
class Skydome {
public:

	~Skydome() {
		if (model_) {
			delete model_; // ← Modelが new なら必ず解放
			model_ = nullptr;
		}
	}

	// objName: "skydome"（半径1想定・内向き面推奨）, scale: 半径スケール
	void Initialize(const char* objName, float scale) {
		model_ = KamataEngine::Model::CreateFromOBJ(objName, true);
		wt_.Initialize();
		wt_.scale_ = {scale, scale, scale};
		wt_.rotation_ = {0.0f, 0.0f, 0.0f};
		wt_.translation_ = {0.0f, 0.0f, 0.0f};
		wt_.TransferMatrix();
	}

	// カメラ位置に追従させたいときに呼ぶ（GetEyeが無い環境でも使える版）
	void UpdateFollowAt(const KamataEngine::Vector3& eye) {
		wt_.translation_ = eye;
		wt_.matWorld_ = MakeAffine(wt_.scale_, wt_.rotation_, wt_.translation_);
		wt_.TransferMatrix();
	}

	// カメラ行列だけ渡して描画
	void Draw(const KamataEngine::Camera& cam) const {
		if (!model_)
			return;
		model_->Draw(wt_, cam);
	}

private:
	// --- 最小限のローカル行列ユーティリティ（行ベクトル系想定） ---
	static KamataEngine::Matrix4x4 Identity() {
		KamataEngine::Matrix4x4 a{};
		for (int i = 0; i < 4; ++i)
			a.m[i][i] = 1.0f;
		return a;
	}
	static KamataEngine::Matrix4x4 Multiply(const KamataEngine::Matrix4x4& A, const KamataEngine::Matrix4x4& B) {
		KamataEngine::Matrix4x4 C{};
		for (int r = 0; r < 4; ++r) {
			for (int c = 0; c < 4; ++c) {
				C.m[r][c] = A.m[r][0] * B.m[0][c] + A.m[r][1] * B.m[1][c] + A.m[r][2] * B.m[2][c] + A.m[r][3] * B.m[3][c];
			}
		}
		return C;
	}
	static KamataEngine::Matrix4x4 MakeScale(const KamataEngine::Vector3& s) {
		auto a = Identity();
		a.m[0][0] = s.x;
		a.m[1][1] = s.y;
		a.m[2][2] = s.z;
		return a;
	}
	static KamataEngine::Matrix4x4 MakeTranslate(const KamataEngine::Vector3& t) {
		auto a = Identity();
		a.m[3][0] = t.x;
		a.m[3][1] = t.y;
		a.m[3][2] = t.z;
		return a;
	}
	static KamataEngine::Matrix4x4 MakeRotateX(float rx) {
		auto a = Identity();
		float c = std::cos(rx), s = std::sin(rx);
		a.m[1][1] = c;
		a.m[1][2] = s;
		a.m[2][1] = -s;
		a.m[2][2] = c;
		return a;
	}
	static KamataEngine::Matrix4x4 MakeRotateY(float ry) {
		auto a = Identity();
		float c = std::cos(ry), s = std::sin(ry);
		a.m[0][0] = c;
		a.m[0][2] = -s;
		a.m[2][0] = s;
		a.m[2][2] = c;
		return a;
	}
	static KamataEngine::Matrix4x4 MakeRotateZ(float rz) {
		auto a = Identity();
		float c = std::cos(rz), s = std::sin(rz);
		a.m[0][0] = c;
		a.m[0][1] = s;
		a.m[1][0] = -s;
		a.m[1][1] = c;
		return a;
	}
	static KamataEngine::Matrix4x4 MakeAffine(const KamataEngine::Vector3& s, const KamataEngine::Vector3& r, const KamataEngine::Vector3& t) {
		auto S = MakeScale(s);
		auto Rx = MakeRotateX(r.x);
		auto Ry = MakeRotateY(r.y);
		auto Rz = MakeRotateZ(r.z);
		auto R = Multiply(Multiply(Rx, Ry), Rz);
		auto T = MakeTranslate(t);
		return Multiply(Multiply(S, R), T);
	}

private:
	KamataEngine::Model* model_ = nullptr;
	KamataEngine::WorldTransform wt_{};
};
