#pragma once
#include "matrix.h"
#include <cmath>
#include"kamataEngine.h"

// ========== 行列生成系 ==========
inline Matrix4x4 MakeTranslationMatrix(const Vector3& t) {
	Matrix4x4 m = Matrix4x4::Identity();
	m.m[3][0] = t.x;
	m.m[3][1] = t.y;
	m.m[3][2] = t.z;
	return m;
}

inline Matrix4x4 MakeScaleMatrix(const Vector3& s) {
	Matrix4x4 m = Matrix4x4::Identity();
	m.m[0][0] = s.x;
	m.m[1][1] = s.y;
	m.m[2][2] = s.z;
	return m;
}

inline Matrix4x4 MakeRotationXMatrix(float rad) {
	Matrix4x4 m = Matrix4x4::Identity();
	float c = std::cos(rad), s = std::sin(rad);
	m.m[1][1] = c;
	m.m[1][2] = s;
	m.m[2][1] = -s;
	m.m[2][2] = c;
	return m;
}
inline Matrix4x4 MakeRotationYMatrix(float rad) {
	Matrix4x4 m = Matrix4x4::Identity();
	float c = std::cos(rad), s = std::sin(rad);
	m.m[0][0] = c;
	m.m[0][2] = -s;
	m.m[2][0] = s;
	m.m[2][2] = c;
	return m;
}
inline Matrix4x4 MakeRotationZMatrix(float rad) {
	Matrix4x4 m = Matrix4x4::Identity();
	float c = std::cos(rad), s = std::sin(rad);
	m.m[0][0] = c;
	m.m[0][1] = s;
	m.m[1][0] = -s;
	m.m[1][1] = c;
	return m;
}

inline Matrix4x4 Multiply(const Matrix4x4& a, const Matrix4x4& b) {
	Matrix4x4 r{};
	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
			r.m[i][j] = a.m[i][0] * b.m[0][j] + a.m[i][1] * b.m[1][j] + a.m[i][2] * b.m[2][j] + a.m[i][3] * b.m[3][j];
	return r;
}

inline Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotation, const Vector3& translation) {
	using std::cos;
	using std::sin;

	Matrix4x4 matScale = MakeScaleMatrix(scale);
	Matrix4x4 matRotX = MakeRotationXMatrix(rotation.x);
	Matrix4x4 matRotY = MakeRotationYMatrix(rotation.y);
	Matrix4x4 matRotZ = MakeRotationZMatrix(rotation.z);
	Matrix4x4 matRot = matRotZ * matRotX * matRotY;
	Matrix4x4 matTrans = MakeTranslationMatrix(translation);
	return matScale * matRot * matTrans;
}
