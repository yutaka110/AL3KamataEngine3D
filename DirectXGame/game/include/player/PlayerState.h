// PlayerState.h
#pragma once
#include "PlayerTypes.h"

namespace game::player {
// 入力と内部カウンタをもとに「速度の方針」を決める層（ジャンプ猶予をここで消費）
void StepState(Data& d, const Input& in, const Params& p);
} // namespace game::player
