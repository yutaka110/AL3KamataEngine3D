#pragma once
#include "KamataEngine.h"

// 画面フェード（黒）
class ScreenFader {
public:
	// whiteTexId は 1x1 白画像(white1x1.png) を推奨
	void Initialize(int screenW, int screenH, uint32_t whiteTexId);

	// フェード開始：Out=暗転に向かう(0→1)、In=明転に向かう(1→0)
	void StartFadeOut(float durationSec, float r = 0.0f, float g = 0.0f, float b = 0.0f);
	void StartFadeIn(float durationSec, float r = 0.0f, float g = 0.0f, float b = 0.0f);

	// 毎フレーム更新（dt秒）
	void Update(float dt);

	// 描画（常にフレームの一番最後に呼ぶ）
	void Draw();

	// 状態問い合わせ
	bool IsActive() const { return state_ != State::Idle; }
	bool IsFadingOut() const { return state_ == State::Out; }
	bool IsFadingIn() const { return state_ == State::In; }
	bool IsOutJustFinished(); // 直前フレームでOutが完了した瞬間だけ true
	bool IsInJustFinished();  // 直前フレームでInが完了した瞬間だけ true
	float Alpha() const { return alpha_; }

	~ScreenFader();

private:
	enum class State { Idle, Out, In };

	State state_ = State::Idle;
	float t_ = 0.0f;
	float duration_ = 0.0f;
	float from_ = 0.0f;
	float to_ = 0.0f;
	float alpha_ = 0.0f;                // 0..1
	float col_[3] = {0.0f, 0.0f, 0.0f}; // 黒(既定)

	int screenW_ = 1280;
	int screenH_ = 720;

	KamataEngine::Sprite* overlay_ = nullptr;
	uint32_t whiteTex_ = 0;

	bool outJustFinished_ = false;
	bool inJustFinished_ = false;
};
