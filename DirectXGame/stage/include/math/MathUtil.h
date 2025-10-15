// stage/include/math/MathUtil.h
#pragma once
#include <cmath>
#include"kamataEngine.h"

namespace ge3::math {

using V3 = KamataEngine::Vector3;
using M44 = KamataEngine::Matrix4x4;

// 行優先(Row-major)なら1、列優先(Column-major)なら0
#ifndef GE3_TRANSLATION_ROW_MAJOR
#define GE3_TRANSLATION_ROW_MAJOR 1
#endif

inline M44 MakeIdentityM44() {
	M44 m{};
	m.m[0][0] = m.m[1][1] = m.m[2][2] = m.m[3][3] = 1.0f;
	return m;
}

inline M44 Multiply(const M44& a, const M44& b) {
	M44 r{};
	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
			r.m[i][j] = a.m[i][0] * b.m[0][j] + a.m[i][1] * b.m[1][j] + a.m[i][2] * b.m[2][j] + a.m[i][3] * b.m[3][j];
	return r;
}

inline M44 MakeTranslationMatrix(const V3& t) {
	M44 m = MakeIdentityM44();
#if GE3_TRANSLATION_ROW_MAJOR
	m.m[3][0] = t.x;
	m.m[3][1] = t.y;
	m.m[3][2] = t.z; // 行優先: 下段にT
#else
	m.m[0][3] = t.x;
	m.m[1][3] = t.y;
	m.m[2][3] = t.z; // 列優先: 右端にT
#endif
	return m;
}

inline M44 MakeScaleMatrix(const V3& s) {
	M44 m = MakeIdentityM44();
	m.m[0][0] = s.x;
	m.m[1][1] = s.y;
	m.m[2][2] = s.z;
	return m;
}

inline M44 MakeRotationXMatrix(float r) {
	M44 m = MakeIdentityM44();
	float c = std::cos(r), s = std::sin(r);
	m.m[1][1] = c;
	m.m[1][2] = s;
	m.m[2][1] = -s;
	m.m[2][2] = c;
	return m;
}
inline M44 MakeRotationYMatrix(float r) {
	M44 m = MakeIdentityM44();
	float c = std::cos(r), s = std::sin(r);
	m.m[0][0] = c;
	m.m[0][2] = -s;
	m.m[2][0] = s;
	m.m[2][2] = c;
	return m;
}
inline M44 MakeRotationZMatrix(float r) {
	M44 m = MakeIdentityM44();
	float c = std::cos(r), s = std::sin(r);
	m.m[0][0] = c;
	m.m[0][1] = s;
	m.m[1][0] = -s;
	m.m[1][1] = c;
	return m;
}

// world = S * R * T
inline M44 MakeAffineMatrix(const V3& s, const V3& r, const V3& t) {
	M44 S = MakeScaleMatrix(s);
	M44 Rx = MakeRotationXMatrix(r.x);
	M44 Ry = MakeRotationYMatrix(r.y);
	M44 Rz = MakeRotationZMatrix(r.z);
	M44 R = Multiply(Multiply(Rz, Rx), Ry); // Z→X→Y
	M44 T = MakeTranslationMatrix(t);
	return Multiply(Multiply(S, R), T);
}

} // namespace ge3::math
