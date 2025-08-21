#pragma once
#include "GameScene.h"
using KamataEngine::Matrix4x4;
using KamataEngine::Vector3;

inline Matrix4x4 Identity() {
	Matrix4x4 a{};
	for (int i = 0; i < 4; ++i)
		a.m[i][i] = 1.0f;
	return a;
}

inline Matrix4x4 Multiply(const Matrix4x4& A, const Matrix4x4& B) {
	Matrix4x4 C{};
	for (int r = 0; r < 4; ++r) {
		for (int c = 0; c < 4; ++c) {
			C.m[r][c] = A.m[r][0] * B.m[0][c] + A.m[r][1] * B.m[1][c] + A.m[r][2] * B.m[2][c] + A.m[r][3] * B.m[3][c];
		}
	}
	return C;
}

inline Matrix4x4 MakeScale(const Vector3& s) {
	Matrix4x4 a = Identity();
	a.m[0][0] = s.x;
	a.m[1][1] = s.y;
	a.m[2][2] = s.z;
	return a;
}
inline Matrix4x4 MakeTranslate(const Vector3& t) {
	Matrix4x4 a = Identity();
	a.m[3][0] = t.x;
	a.m[3][1] = t.y;
	a.m[3][2] = t.z; // ←行ベクトル系（多くの教材エンジンがこれ）
	return a;
}
inline Matrix4x4 MakeRotateX(float rx) {
	Matrix4x4 a = Identity();
	float c = std::cos(rx), s = std::sin(rx);
	a.m[1][1] = c;
	a.m[1][2] = s;
	a.m[2][1] = -s;
	a.m[2][2] = c;
	return a;
}
inline Matrix4x4 MakeRotateY(float ry) {
	Matrix4x4 a = Identity();
	float c = std::cos(ry), s = std::sin(ry);
	a.m[0][0] = c;
	a.m[0][2] = -s;
	a.m[2][0] = s;
	a.m[2][2] = c;
	return a;
}
inline Matrix4x4 MakeRotateZ(float rz) {
	Matrix4x4 a = Identity();
	float c = std::cos(rz), s = std::sin(rz);
	a.m[0][0] = c;
	a.m[0][1] = s;
	a.m[1][0] = -s;
	a.m[1][1] = c;
	return a;
}

// 角度はラジアン。行列の掛け順は S * (Rx*Ry*Rz) * T（教材の行ベクトル想定）
inline Matrix4x4 MakeAffine(const Vector3& s, const Vector3& r, const Vector3& t) {
	Matrix4x4 S = MakeScale(s);
	Matrix4x4 Rx = MakeRotateX(r.x);
	Matrix4x4 Ry = MakeRotateY(r.y);
	Matrix4x4 Rz = MakeRotateZ(r.z);
	Matrix4x4 R = Multiply(Multiply(Rx, Ry), Rz);
	Matrix4x4 T = MakeTranslate(t);
	return Multiply(Multiply(S, R), T);
}

inline Vector3 Lerp3(const Vector3& a, const Vector3& b, float t) { return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t}; }

// ▼ これで置き換え
inline Matrix4x4 MakeLookAtRow(const Vector3& eye, const Vector3& target, const Vector3& up) {
	// Left-Hand (+Z 前) / row-vector
	Vector3 f{target.x - eye.x, target.y - eye.y, target.z - eye.z}; // forward
	float fl = std::sqrt(f.x * f.x + f.y * f.y + f.z * f.z);
	if (fl > 1e-6f) {
		f.x /= fl;
		f.y /= fl;
		f.z /= fl;
	} else {
		f = {0, 0, 1};
	}

	// ★修正: r = up × f  （以前は f × up だった）
	Vector3 r{up.y * f.z - up.z * f.y, up.z * f.x - up.x * f.z, up.x * f.y - up.y * f.x};
	float rl = std::sqrt(r.x * r.x + r.y * r.y + r.z * r.z);
	if (rl > 1e-6f) {
		r.x /= rl;
		r.y /= rl;
		r.z /= rl;
	} else {
		r = {1, 0, 0};
	}

	// y軸 = f × r
	Vector3 u{f.y * r.z - f.z * r.y, f.z * r.x - f.x * r.z, f.x * r.y - f.y * r.x};

	Matrix4x4 V = Identity();
	// 上左3x3 は R^T（各行に right, up, forward）
	V.m[0][0] = r.x;
	V.m[0][1] = r.y;
	V.m[0][2] = r.z;
	V.m[1][0] = u.x;
	V.m[1][1] = u.y;
	V.m[1][2] = u.z;
	V.m[2][0] = f.x;
	V.m[2][1] = f.y;
	V.m[2][2] = f.z;
	// 最下段は -eye * R^T
	V.m[3][0] = -(eye.x * r.x + eye.y * r.y + eye.z * r.z);
	V.m[3][1] = -(eye.x * u.x + eye.y * u.y + eye.z * u.z);
	V.m[3][2] = -(eye.x * f.x + eye.y * f.y + eye.z * f.z);
	return V;
}
