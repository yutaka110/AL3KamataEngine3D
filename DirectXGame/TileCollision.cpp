#include "TileCollision.h"
#include "matrix.h"
#include <algorithm>
#include <cmath>

using KamataEngine::Vector3;
using KamataEngine::WorldTransform;

namespace {
inline bool IsSolid(const std::vector<std::vector<int>>& g, int y, int x) { return (y >= 0 && x >= 0 && y < (int)g.size() && x < (int)g[0].size() && g[y][x] != 0); }
inline void TileCenter(int tx, int ty, const TileField& tf, float& cx, float& cy) {
	cx = tf.originX + tx * tf.pitchX;
	cy = tf.originY + ty * tf.pitchY;
}
} // namespace

void ResolvePlayerVsTilemap(Player& player, const TileField& tf) {
	if (!tf.grid || tf.grid->empty())
		return;

	WorldTransform& wt = player.EditWorldTransform();
	Vector3& vel = player.EditVelocity();
	Vector3 pos = wt.translation_;

	const Vector3 ph = PlayerHalfExtents();
	const float BHX = tf.pitchX * 0.5f; // ★ 見た目の並びと一致させる
	const float BHY = tf.pitchY * 0.5f;

	// 一旦非接地。Y解決時に立て直す
	player.SetGrounded(false);

	auto collideAxis = [&](int axis) {
		// 近傍だけチェック（±3タイル）
		const int cx = (int)std::floor((pos.x - tf.originX) / tf.pitchX + 0.5f);
		const int cy = (int)std::floor((pos.y - tf.originY) / tf.pitchY + 0.5f);
		for (int ty = cy - 3; ty <= cy + 3; ++ty) {
			for (int tx = cx - 3; tx <= cx + 3; ++tx) {
				if (!IsSolid(*tf.grid, ty, tx))
					continue;

				float cxw, cyw;
				TileCenter(tx, ty, tf, cxw, cyw);
				const float dx = pos.x - cxw;
				const float dy = pos.y - cyw;
				const float px = (BHX + ph.x) - std::fabs(dx);
				const float py = (BHY + ph.y) - std::fabs(dy);
				if (px <= 0.0f || py <= 0.0f)
					continue;

				const float skin = SkinWidth();
				if (axis == 0) {
					if (px < py) {
						// Xをスナップ
						if (dx > 0.0f)
							pos.x = cxw + (BHX + ph.x) + skin;
						else
							pos.x = cxw - (BHX + ph.x) - skin;
						vel.x = 0.0f;
					}
				} else {
					if (py <= px) {
						// Yをスナップ
						if (dy > 0.0f) { // 上側（プレイヤーが上にいる＝床に着地）
							pos.y = cyw + (BHY + ph.y) + skin;
							if (vel.y < 0.0f)
								vel.y = 0.0f;
							player.SetGrounded(true);
						} else { // 下側（天井ヒット）
							pos.y = cyw - (BHY + ph.y) - skin;
							if (vel.y > 0.0f)
								vel.y = 0.0f;
						}
					}
				}
			}
		}
	};

	// Player::Update() で pos += vel 済み想定。X→Y の順に解決
	collideAxis(0);
	collideAxis(1);

	// 反映＆GPUへ
	wt.translation_ = pos;
	wt.matWorld_ = MakeAffine(wt.scale_, wt.rotation_, wt.translation_);
	if (wt.parent_)
		wt.matWorld_ = Multiply(wt.matWorld_, wt.parent_->matWorld_);
	wt.TransferMatrix();
}
