// PlayerPhysics.cpp
#include "player/PlayerPhysics.h"
#include <cmath>

namespace gp = game::player;

void gp::ApplyFriction(gp::Data& d, const gp::Params& p) {
	const float k = d.grounded ? p.frictionGround : p.frictionAir;
	d.vx *= k;
	if (std::fabs(d.vx) < 1e-3f)
		d.vx = 0.0f;
}

void gp::ApplyGravity(gp::Data& d, const gp::Params& p) {
	const bool gliding = (d.state == State::Gliding) || d.gliding;
	const float g = gliding ? p.glideGravity : p.gravity;
	const float vmax = gliding ? p.glideMaxFallSpeed : p.maxFallSpeed;
	d.vy += g;
	if (d.vy < vmax) d.vy = vmax;
}

void gp::IntegratePosition(float& x, float& y, gp::Data& d) {
	x += d.vx;
	y += d.vy;
}

void gp::ClampToFloor(float& y, gp::Data& d) {
	// 既存実装の「仮地面」ロジック。必要なら呼び出し側で有効化。
	if (y <= d.groundY) {
		y = d.groundY;
		if (d.vy < 0.0f)
			d.vy = 0.0f;
		d.grounded = true;
	} else {
		d.grounded = false;
	}
}
