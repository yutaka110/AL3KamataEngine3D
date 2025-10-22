// PlayerState.cpp
#include "player/PlayerState.h"
#include <algorithm>
#include <cmath>

namespace gp = game::player;

void gp::StepState(gp::Data& d, const gp::Input& in, const gp::Params& p) {
	// ジャンプバッファ（押されたらセット）
	if (in.jumpPressed)
		d.jumpBufferCounter = p.jumpBufFrames;

	// 地上/空中の切替とコヨーテ管理（grounded は Physics 側が更新）
	if (d.grounded) {
		d.state = gp::State::OnGround;
		d.coyoteCounter = p.coyoteFrames;
	} else {
		d.state = gp::State::InAir;
		if (d.coyoteCounter > 0)
			--d.coyoteCounter;
	}

	// 水平：加減速（入力ゼロ時は Physics 側で摩擦）
	if (in.axisX != 0) {
		const bool sameDir = (d.vx == 0.0f) ? true : ((d.vx > 0.0f) == (in.axisX > 0));
		const float a = sameDir ? p.accelX : p.brakeX;
		d.vx += a * static_cast<float>(in.axisX);
		d.vx = std::clamp(d.vx, -p.maxSpeedX, p.maxSpeedX);
	}

	// ジャンプ判定（バッファ × コヨーテ）
	if (d.jumpBufferCounter > 0 && (d.grounded || d.coyoteCounter > 0)) {
		d.vy = p.jumpVelocity;
		d.grounded = false;
		d.jumpBufferCounter = 0;
	} else if (d.jumpBufferCounter > 0) {
		--d.jumpBufferCounter;
	}
}
