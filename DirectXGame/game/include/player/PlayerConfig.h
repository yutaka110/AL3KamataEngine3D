#pragma once
#include "PlayerTypes.h"

namespace game::player {
inline Params DefaultParams() {
	Params p{};
	// 既存値に合わせています（あなたのコードの係数と同等）
	p.accelX = 0.05f;
	p.brakeX = 0.08f;
	p.maxSpeedX = 0.20f;
	p.frictionGround = 0.88f;
	p.frictionAir = 0.90f;

	// ＋Yが下 or 上のどちら想定でも動くよう、
	// 既存ロジックに合わせ「ジャンプで +vy」「重力で -vy」になる符号系で統一
	p.gravity = -0.25f;
	p.jumpVelocity = 2.20f;
	p.maxFallSpeed = -0.10f;

	p.coyoteFrames = 6;
	p.jumpBufFrames = 6;
	return p;
}
} // namespace game::player
