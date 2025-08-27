#pragma once
#include "KamataEngine.h"

class ClearScene {
public:
	void Initialize() {
		// でかい文字用フォント or テクスチャが無ければ白スプライトで簡易表示でもOK
		timer_ = 0.0f;
		finished_ = false;
	}
	void Update() {
		timer_ += 1.0f / 60.0f;
		auto* in = KamataEngine::Input::GetInstance();
		if (in->TriggerKey(DIK_SPACE) || in->TriggerKey(DIK_RETURN)) {
			finished_ = true; // 次へ進む（タイトルに戻す or ゲーム再開）
		}
	}
	void Draw() {
		// 超簡易表示：SpriteFont等が無い前提で、中央に白四角＋テキスト風
		// もしあなたの環境にテキスト描画があれば差し替えてOK
		// ここは仮: 何も無ければ背景的に何も描かずゲーム下地をそのまま見せてもOK
	}

	bool IsFinished() const { return finished_; }

private:
	float timer_ = 0.0f;
	bool finished_ = false;
};
