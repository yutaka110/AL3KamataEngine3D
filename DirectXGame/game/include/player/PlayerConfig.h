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
	p.gravity = -0.14f;
	p.jumpVelocity = 1.4f;
	p.maxFallSpeed = -1.0f;

	p.coyoteFrames = 6;
	p.jumpBufFrames = 6;
	p.extraJumps = 1;

	// 回避デフォ
	p.dodgeSpeed = 0.60f;
	p.dodgeFrames = 11; 
	p.dodgeCooldownFrames = 20;
	p.dodgeIFrames = 8;

	p.jumpCutFactor = 0.55f; // ボタン離しで上昇を約半分にカット

	// 滑空の初期値（体感を重視した無難セット）
	p.glideGravity = -0.08f;      // 落下をかなり緩める
	p.glideMaxFallSpeed = -0.80f; // 終端も緩める
	p.glideMaxFrames = 120;       // 2秒＠60fps（無制限なら0）
	p.glideStartVy = 0.10f;       // vy <= 0.10f なら開始可（頂点～落下で発動）
	return p;
}
} // namespace game::player
