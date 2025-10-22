// pch を使っているなら最初に
// #include "pch.h"

#include "player/player_input.h" // 小文字＆スネークケースに統一
#include "KamataEngine.h"

namespace gp = game::player;

void gp::InputReader::Read(gp::Input& out) {
	// 初期化
	out.axisX = 0;
	out.jumpPressed = false;

	// ※ここがポイント：game::player::Input（構造体）と名前が衝突していたので
	//   KamataEngine 側の Input を「完全修飾名」で呼びます。
	auto* in = KamataEngine::Input::GetInstance();

	int ax = 0;
	if (in->PushKey(DIK_LEFT) || in->PushKey(DIK_A))
		ax -= 1;
	if (in->PushKey(DIK_RIGHT) || in->PushKey(DIK_D))
		ax += 1;
	out.axisX = (ax < 0) ? -1 : (ax > 0 ? 1 : 0);

	const bool tr = in->TriggerKey(DIK_SPACE) || in->TriggerKey(DIK_W) || in->TriggerKey(DIK_UP);

	out.jumpPressed = tr;
}
