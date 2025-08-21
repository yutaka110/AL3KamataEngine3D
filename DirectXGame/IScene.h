#pragma once
#include "KamataEngine.h"

enum class SceneId { Title, Game };

struct IScene {
	virtual ~IScene() = default;
	virtual void Initialize() = 0;
	virtual void Update() = 0;
	virtual void Draw() = 0;

	// 遷移管理
	virtual bool IsFinished() const = 0;
	virtual SceneId NextScene() const = 0;
};
