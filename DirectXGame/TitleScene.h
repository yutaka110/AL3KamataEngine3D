#pragma once
#include "IScene.h"

class TitleScene final : public IScene {
public:
	~TitleScene() override;
	void Initialize() override;
	void Update() override;
	void Draw() override;

	bool IsFinished() const override { return finished_; }
	SceneId NextScene() const override { return SceneId::Game; }

private:
	// 表示物（お好みで）
	uint32_t tex_ = 0;
	KamataEngine::Sprite* sprite_ = nullptr;

	bool finished_ = false;
};
