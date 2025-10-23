// PlayerState.cpp
#include "player/PlayerState.h"
#include <algorithm>
#include <cmath>

namespace gp = game::player;

void gp::StepState(gp::Data& d, const gp::Input& in, const gp::Params& p) {
	// ジャンプバッファ（押されたらセット）
	if (in.jumpPressed)
		d.jumpBufferCounter = p.jumpBufFrames;

	// クールダウン・無敵の消費
	if (d.cooldownCounter > 0) {
		--d.cooldownCounter;
	}

	if (d.iFrameCounter > 0) {
		--d.iFrameCounter;
		d.invincible = true;
	} else {
		d.invincible = false;
	}

	// 地上/空中の切替とコヨーテ管理（grounded は Physics 側が更新）
	if (d.grounded) {
		d.state = gp::State::OnGround;
		d.coyoteCounter = p.coyoteFrames;

		// 地上にいる間は追加ジャンプ回数をリセット
		d.jumpsRemain = p.extraJumps;
	} else {
		d.state = gp::State::InAir;
		if (d.coyoteCounter > 0)
			--d.coyoteCounter;
	}

	// すでに回避中の処理（速度は保持、終わったら通常へ）
	if (d.state == gp::State::Dodging) {
		++d.dodgeCounter;
		if (d.dodgeCounter >= p.dodgeFrames) {
			d.dodgeCounter = 0;
			// 回避終了：地上/空中に戻す（grounded は物理側が更新済み）
			d.state = d.grounded ? gp::State::OnGround : gp::State::InAir;
		}
		// 回避中はここでリターンし、通常の加減速/ジャンプ処理をスキップ
		return;
	}

	// 水平：加減速（入力ゼロ時は Physics 側で摩擦）
	if (in.axisX != 0) {
		const bool sameDir = (d.vx == 0.0f) ? true : ((d.vx > 0.0f) == (in.axisX > 0));
		const float a = sameDir ? p.accelX : p.brakeX;
		d.vx += a * static_cast<float>(in.axisX);
		d.vx = std::clamp(d.vx, -p.maxSpeedX, p.maxSpeedX);
	}

	// ジャンプ判定（地上/コヨーテ/空中追加ジャンプ）
	if (d.jumpBufferCounter > 0) {
		const bool canGroundJump = (d.grounded || d.coyoteCounter > 0);
		const bool canAirJump = (!d.grounded && d.coyoteCounter <= 0 && d.jumpsRemain > 0);
		if (canGroundJump || canAirJump) {
			d.vy = p.jumpVelocity;
			d.grounded = false;
			d.jumpBufferCounter = 0;
			if (canAirJump) {
				--d.jumpsRemain; // 空中で消費
				
			}
			
		}
		else {
			--d.jumpBufferCounter;
			
		}
		
	}
	else if (d.jumpBufferCounter > 0) {
		--d.jumpBufferCounter;
	}

	// 回避トリガ（地上優先だが、必要なら空中でも許可できる）
	if (in.dodgePressed && d.cooldownCounter == 0) {
		int dir = (in.axisX != 0) ? in.axisX : ((d.vx >= 0.0f) ? 1 : -1);
		if (dir == 0)
			dir = 1; // 完全停止時の保険
		d.vx = static_cast<float>(dir) * p.dodgeSpeed;
		// 地上では落ちないように縦速度0。空中はそのまま（好みでOK）
		if (d.grounded)
			d.vy = 0.0f;
		d.state = gp::State::Dodging;
		d.dodgeCounter = 0;
		d.cooldownCounter = p.dodgeCooldownFrames;
		d.iFrameCounter = p.dodgeIFrames;
		d.invincible = (d.iFrameCounter > 0);
		return;
	}
}
