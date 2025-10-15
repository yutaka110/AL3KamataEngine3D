//#include "StageEditorView.h"
//
//// StageEditorView.cpp（疑似コード：あなたのSpriteAPIに合わせて差し替え）
//// 1) タイル描画：ID→UVを切ってスプライト描画
//void StageEditorView::Draw(const StageEditor& ed, const TileAtlas& A, int ox, int oy, float s) {
//	const auto& buf = ed.Raw();
//	const int W = (int)ed.Width(), H = (int)ed.Height();
//	for (int y = 0; y < H; ++y) {
//		for (int x = 0; x < W; ++x) {
//			int id = buf[(size_t)y * W + x];
//			if (id < 0)
//				continue; // -1などは空マス扱い
//			int u = id % A.atlasCols;
//			int v = id / A.atlasCols;
//			// 画面座標
//			float px = ox + x * A.tileW * s;
//			float py = oy + y * A.tileH * s;
//			// UV（あなたのSpriteにUV指定APIがあるなら設定）
//			// Sprite::Draw(A.texId, px, py, A.tileW*s, A.tileH*s, /*uvRect*/{u,v,1,1});
//			// ↑ライブラリに合わせて：UV＝(u*tileW, v*tileH, tileW, tileH)/テクスチャサイズ
//
//
//		}
//	}
//}
//
//// 2) グリッド描画：編集時の見やすさUp（ラインはImDrawList or 自前ライン描画）
//void StageEditorView::DrawGrid(const StageEditor& ed, int ox, int oy, float s) {
//	const int W = (int)ed.Width(), H = (int)ed.Height();
//	// 例：ImGui の DrawList を使う場合
//	// auto* dl = ImGui::GetForegroundDrawList();
//	// for (int x = 0; x <= W; ++x) {
//	//   float gx = ox + x * tileW * s;
//	//   dl->AddLine(ImVec2(gx, oy), ImVec2(gx, oy + H * tileH * s), 0x40FFFFFF);
//	// }
//	// for (int y = 0; y <= H; ++y) {
//	//   float gy = oy + y * tileH * s;
//	//   dl->AddLine(ImVec2(ox, gy), ImVec2(ox + W * tileW * s, gy), 0x40FFFFFF);
//	// }
//}
