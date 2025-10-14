#pragma once
#include <cassert>
#include <cmath>

// =========================
// Vector3
// =========================
struct Vector3 {
	float x, y, z;

	Vector3() : x(0), y(0), z(0) {}
	Vector3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

	// 加減乗除
	Vector3 operator+(const Vector3& v) const { return {x + v.x, y + v.y, z + v.z}; }
	Vector3 operator-(const Vector3& v) const { return {x - v.x, y - v.y, z - v.z}; }
	Vector3 operator*(float s) const { return {x * s, y * s, z * s}; }
	Vector3 operator/(float s) const {
		assert(s != 0.0f);
		return {x / s, y / s, z / s};
	}

	Vector3& operator+=(const Vector3& v) {
		x += v.x;
		y += v.y;
		z += v.z;
		return *this;
	}
	Vector3& operator-=(const Vector3& v) {
		x -= v.x;
		y -= v.y;
		z -= v.z;
		return *this;
	}
	Vector3& operator*=(float s) {
		x *= s;
		y *= s;
		z *= s;
		return *this;
	}

	float Length() const { return std::sqrt(x * x + y * y + z * z); }
	Vector3 Normalized() const {
		float len = Length();
		if (len == 0.0f)
			return {0, 0, 0};
		return {x / len, y / len, z / len};
	}
};



// =========================
// Matrix4x4（行優先）
// =========================
struct Matrix4x4 {
	float m[4][4];

	Matrix4x4() {
		for (int i = 0; i < 4; i++)
			for (int j = 0; j < 4; j++)
				m[i][j] = (i == j) ? 1.0f : 0.0f;
	}

	static Matrix4x4 Identity() { return Matrix4x4(); }

	// 行列同士の掛け算
	Matrix4x4 operator*(const Matrix4x4& o) const {
		Matrix4x4 r{};
		for (int i = 0; i < 4; ++i)
			for (int j = 0; j < 4; ++j)
				r.m[i][j] = m[i][0] * o.m[0][j] + m[i][1] * o.m[1][j] + m[i][2] * o.m[2][j] + m[i][3] * o.m[3][j];
		return r;
	}
};

