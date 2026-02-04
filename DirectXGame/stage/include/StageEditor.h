// stage/StageEditor.h
#pragma once
// 依存: imgui, C/C++標準
#include <algorithm>
#include <cstdio>  // FILE, fopen/fprintf/fscanf
#include <cstring> // std::snprintf
#include <functional>
#include <imgui.h>
#include <vector>

namespace ge3 {
namespace stage {

// マップチップエディター本体（UI+CSV I/O）。描画/当たり判定はゲーム側へ委譲。
// 0=空白, 1..=任意タイル。3D反映は ForEach((x,y,id){...}) で受け取って描く。
struct StageEditor {

	// ===== 基本情報 =====
	int width = 16;        // セル数（横）
	int height = 16;       // セル数（縦）
	float cellSize = 1.0f; // 1セルのワールドスケール（3D側の目安）

	// タイルID配列（row-major: y*width + x）
	std::vector<int> tiles;

	// ===== UI状態 =====
	int selectedId = 1;   // ペイント対象ID（0は消しゴム）
	int paletteMax = 9;   // パレット上限（1..paletteMax）
	bool showGrid = true; // グリッド表示
	bool editMode = true; // ウィンドウON/OFF

	// ドラッグ編集状態
	bool leftDragPainting = false;
	bool rightDragErasing = false;

	// ホバー中セル座標（キャンバス外なら -1）
	int hoverX = -1, hoverY = -1;

	// CSVパス（UIから編集）
	char path[260] = "stage.csv";

	// ===== ライフサイクル =====
	void Initialize(int w, int h, float cell);

	// ===== セル操作 =====
	int Index(int x, int y) const;    // y*width + x
	bool InRange(int x, int y) const; // 範囲チェック
	int Get(int x, int y) const;      // 範囲外は0を返す
	void Set(int x, int y, int id);   // 範囲外は無視
	void ClearAll(int id = 0);        // 全面クリア

	// ===== 列挙（3D反映などに使用）=====
	void ForEach(const std::function<void(int x, int y, int id)>& fn, bool onlyNonZero = true) const;

	// ===== CSV I/O =====
	bool SaveCSV(const char* p) const; // 幅×高さの行列をそのまま保存
	bool LoadCSV(const char* p);       // 読み込み不足は0埋め/余剰は切捨て

	// ===== ImGuiエディタ（2Dキャンバス）=====
	void UpdateEditorUI(const char* windowTitle);
};

} // namespace stage
} // namespace ge3
