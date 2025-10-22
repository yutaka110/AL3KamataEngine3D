// PlayerPhysics.h
#pragma once
#include "PlayerTypes.h"

namespace game::player {
void ApplyFriction(Data& d, const Params& p); // 入力ゼロ時の減衰
void ApplyGravity(Data& d, const Params& p);
void IntegratePosition(float& x, float& y, Data& d); // x,y を外から受け取って更新
void ClampToFloor(float& y, Data& d);                // 仮地面での接地（後でタイル衝突へ置換）
} // namespace game::player
