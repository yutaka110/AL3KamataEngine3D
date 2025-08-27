#include "goal.h"
#include <cmath>

using namespace KamataEngine;

// ---- 内部ユーティリティ ----
static inline Matrix4x4 Identity() {
	Matrix4x4 a{};
	for (int i = 0; i < 4; ++i)
		a.m[i][i] = 1.0f;
	return a;
}
static inline Matrix4x4 Multiply(const Matrix4x4& A, const Matrix4x4& B) {
	Matrix4x4 C{};
	for (int r = 0; r < 4; ++r) {
		for (int c = 0; c < 4; ++c) {
			C.m[r][c] = A.m[r][0] * B.m[0][c] + A.m[r][1] * B.m[1][c] + A.m[r][2] * B.m[2][c] + A.m[r][3] * B.m[3][c];
		}
	}
	return C;
}
static inline Matrix4x4 MakeScale(const Vector3& s) {
	Matrix4x4 a = Identity();
	a.m[0][0] = s.x;
	a.m[1][1] = s.y;
	a.m[2][2] = s.z;
	return a;
}
static inline Matrix4x4 MakeTranslate(const Vector3& t) {
	Matrix4x4 a = Identity();
	// 行ベクトル系
	a.m[3][0] = t.x;
	a.m[3][1] = t.y;
	a.m[3][2] = t.z;
	return a;
}
static inline Matrix4x4 MakeRotateX(float rx) {
	Matrix4x4 a = Identity();
	float c = std::cos(rx), s = std::sin(rx);
	a.m[1][1] = c;
	a.m[1][2] = s;
	a.m[2][1] = -s;
	a.m[2][2] = c;
	return a;
}
static inline Matrix4x4 MakeRotateY(float ry) {
	Matrix4x4 a = Identity();
	float c = std::cos(ry), s = std::sin(ry);
	a.m[0][0] = c;
	a.m[0][2] = -s;
	a.m[2][0] = s;
	a.m[2][2] = c;
	return a;
}
static inline Matrix4x4 MakeRotateZ(float rz) {
	Matrix4x4 a = Identity();
	float c = std::cos(rz), s = std::sin(rz);
	a.m[0][0] = c;
	a.m[0][1] = s;
	a.m[1][0] = -s;
	a.m[1][1] = c;
	return a;
}

Matrix4x4 Goal::MakeAffine(const Vector3& s, const Vector3& r, const Vector3& t) {
	Matrix4x4 S = MakeScale(s);
	Matrix4x4 Rx = MakeRotateX(r.x);
	Matrix4x4 Ry = MakeRotateY(r.y);
	Matrix4x4 Rz = MakeRotateZ(r.z);
	Matrix4x4 R = Multiply(Multiply(Rx, Ry), Rz);
	Matrix4x4 T = MakeTranslate(t);
	return Multiply(Multiply(S, R), T); // S * R * T  （行ベクトル）
}

// ---- Goal
void Goal::Initialize(Model* model, uint32_t textureHandle, Camera* camera, const Vector3& pos) {
	model_ = model;
	textureHandle_ = textureHandle;
	camera_ = camera;

	wt_.Initialize();
	wt_.translation_ = pos;
	wt_.scale_ = {1.0f, 1.0f, 1.0f};
	wt_.rotation_ = {0.0f, 0.0f, 0.0f};

	wt_.matWorld_ = MakeAffine(wt_.scale_, wt_.rotation_, wt_.translation_);
	wt_.TransferMatrix();
}

void Goal::Update() {
	// 必要ならアニメーション
	// wt_.rotation_.z += 0.02f;

	wt_.matWorld_ = MakeAffine(wt_.scale_, wt_.rotation_, wt_.translation_);
	wt_.TransferMatrix();
}

void Goal::Draw(const Camera& cam) {
	if (!model_)
		return;
	model_->Draw(wt_, cam, textureHandle_);
}
