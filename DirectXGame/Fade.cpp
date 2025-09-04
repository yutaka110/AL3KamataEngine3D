#include "Fade.h"

// ここでは Sprite に以下のAPIがある想定：
//   Sprite::Create(texId, {x,y});
//   void SetAnchorPoint(const KamataEngine::Vector2&);
//   void SetPosition(const KamataEngine::Vector2&);
//   void SetSize(const KamataEngine::Vector2&);
//   void SetColor(const KamataEngine::Vector4&);
//   void Draw();
// ※もし SetSize / SetColor が無ければ、Sprite 実装に合わせて置き換えてください。
//   （最低限 1x1白テクスチャを画面サイズへ拡大＆黒＋αで乗算できればOK）

using namespace KamataEngine;

ScreenFader::~ScreenFader() {
	delete overlay_;
	overlay_ = nullptr;
}

void ScreenFader::Initialize(int screenW, int screenH, uint32_t whiteTexId) {
	screenW_ = screenW;
	screenH_ = screenH;
	whiteTex_ = whiteTexId;

	// 1x1 白スプライトを原点から左上起点で全面に拡大
	overlay_ = Sprite::Create(whiteTex_, {0.0f, 0.0f});
	if (overlay_) {
		// 左上原点で全画面サイズ
		if constexpr (true) {
			// SetAnchorPoint(0,0) → 左上基準
			overlay_->SetAnchorPoint({0.0f, 0.0f});
		}
		overlay_->SetPosition({0.0f, 0.0f});
		overlay_->SetSize({static_cast<float>(screenW_), static_cast<float>(screenH_)});
		overlay_->SetColor({0, 0, 0, 0}); // 透明スタート
	}

	// 最初はフェード無し
	state_ = State::Idle;
	t_ = 0.0f;
	duration_ = 0.0f;
	from_ = 0.0f;
	to_ = 0.0f;
	alpha_ = 0.0f;
}

void ScreenFader::StartFadeOut(float durationSec, float r, float g, float b) {
	col_[0] = r;
	col_[1] = g;
	col_[2] = b;
	state_ = State::Out;
	t_ = 0.0f;
	duration_ = (durationSec > 0.0f) ? durationSec : 0.0001f;
	from_ = alpha_;
	to_ = 1.0f; // 暗転
	outJustFinished_ = false;
	inJustFinished_ = false;
}

void ScreenFader::StartFadeIn(float durationSec, float r, float g, float b) {
	col_[0] = r;
	col_[1] = g;
	col_[2] = b;
	state_ = State::In;
	t_ = 0.0f;
	duration_ = (durationSec > 0.0f) ? durationSec : 0.0001f;
	from_ = alpha_;
	to_ = 0.0f; // 明転
	outJustFinished_ = false;
	inJustFinished_ = false;
}

void ScreenFader::Update(float dt) {
	outJustFinished_ = false;
	inJustFinished_ = false;

	if (state_ == State::Idle)
		return;

	t_ += dt;
	float u = t_ / duration_;
	if (u >= 1.0f)
		u = 1.0f;

	alpha_ = from_ + (to_ - from_) * u;

	if (u >= 1.0f) {
		if (state_ == State::Out) {
			outJustFinished_ = true;
		} else if (state_ == State::In) {
			inJustFinished_ = true;
		}
		// 完了後は目標値を保持しつつ Idle
		state_ = State::Idle;
	}
}

bool ScreenFader::IsOutJustFinished() {
	bool f = outJustFinished_;
	outJustFinished_ = false; // 1回だけtrueを返す
	return f;
}
bool ScreenFader::IsInJustFinished() {
	bool f = inJustFinished_;
	inJustFinished_ = false;
	return f;
}

void ScreenFader::Draw() {
	if (!overlay_)
		return;
	if (alpha_ <= 0.0f)
		return;

	// αを反映して全画面スプライトを重ねる
	// SetColor が Vector4(R,G,B,A) で 0..1 の想定
	overlay_->SetColor({col_[0], col_[1], col_[2], alpha_});
	overlay_->Draw();
}
