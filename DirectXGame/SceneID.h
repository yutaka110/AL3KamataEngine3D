#pragma once

enum class SceneID { None, Title, Game };

class IScene {
public:
	virtual ~IScene() = default;
	virtual void Initialize() = 0;
	virtual void Update() = 0;
	virtual void Draw() = 0;

	virtual SceneID NextScene() const { return SceneID::None; }
	virtual void ConsumeNext() {} // 遷移したらフラグをクリア
};