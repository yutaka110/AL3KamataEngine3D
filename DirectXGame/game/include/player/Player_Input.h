// PlayerInput.h
#pragma once
#include "PlayerTypes.h"

namespace game::player {
class InputReader {
public:
	// KamataEngine::Input を内部で使用して、左右とジャンプトリガを読む
	static void Read(Input& out);
};
} // namespace game::player
