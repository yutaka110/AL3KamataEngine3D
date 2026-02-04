#pragma once
#include <cstdint>

namespace game::player {

enum class State : uint8_t { OnGround, InAir, Dodging, Gliding };

struct Params {
	// 水平
	float accelX;         // 加速
	float brakeX;         // 逆向き時の減速(強め)
	float maxSpeedX;      // 最高速
	float frictionGround; // 地上摩擦(指数減衰)
	float frictionAir;    // 空中摩擦
	// 垂直
	float gravity;      // 重力(下向きを負とするなら符号に注意)
	float jumpVelocity; // ジャンプ初速
	float maxFallSpeed; // 最大落下速度
	// 猶予
	int coyoteFrames;  // コヨーテタイム
	int jumpBufFrames; // ジャンプ入力バッファ

	// 追加: 空中で許可する追加ジャンプ回数（1なら二段ジャンプ）
    int extraJumps;

   // 回避
   float dodgeSpeed;          // 回避の水平速度（毎フレーム加算ではなく即時セット）
   int   dodgeFrames;         // 回避の持続フレーム数
   int   dodgeCooldownFrames; // クールダウン
   int   dodgeIFrames;        // 無敵フレーム（回避開始から）

   // ジャンプカット（ボタン離しで上昇を短く）
   float jumpCutFactor;

   // 滑空
   float glideGravity;       // 滑空中の重力（負値の小さい絶対値 = ゆっくり落下）
   float glideMaxFallSpeed;  // 滑空時の終端落下速度（負値の大きさ）
   int   glideMaxFrames;     // 滑空できる最大フレーム（0なら無制限）
   float glideStartVy;       // これ以下の上昇/落下速度なら開始可（例：+0.1f以下＝ほぼ頂点～落下）
};

struct Input {
	int axisX{};        // -1,0,1
	bool jumpPressed{}; // フレーム内トリガ
	bool dodgePressed{};
	bool jumpHeld{};
};

struct Data {
	// 運動状態
	float vx{}, vy{};
	float groundY{}; // 仮の地面Y（後でタイル衝突へ置換）
	bool grounded{true};
	int coyoteCounter{};
	int jumpBufferCounter{};
	State state{State::OnGround};

	// 追加: 残りの空中ジャンプ回数
	int jumpsRemain{0};

	// 回避用
    int   dodgeCounter{};      // 残り/経過フレーム
    int   cooldownCounter{};   // クールダウン
    int   iFrameCounter{};     // 無敵残り
    bool  invincible{};        // 外部公開向けフラグ

	
	bool  gliding{false};
    int   glideCounter{0};
};

} // namespace game::player
