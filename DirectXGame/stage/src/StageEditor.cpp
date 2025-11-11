// stage/StageEditor.cpp
#include "StageEditor.h"

namespace ge3 {
namespace stage {

// ========== ライフサイクル ==========
void StageEditor::Initialize(int w, int h, float cell) {
	width = (w > 0 ? w : 1);
	height = (h > 0 ? h : 1);
	cellSize = cell;
	tiles.assign(width * height, 0);
	selectedId = 1;
	paletteMax = std::max(9, selectedId);
	showGrid = true;
	editMode = true;
	leftDragPainting = rightDragErasing = false;
	hoverX = hoverY = -1;
	std::snprintf(path, sizeof(path), "%s", "stage.csv");
}

// ========== セル操作 ==========
int StageEditor::Index(int x, int y) const { return y * width + x; }
bool StageEditor::InRange(int x, int y) const { return (0 <= x && x < width && 0 <= y && y < height); }

int StageEditor::Get(int x, int y) const {
	if (!InRange(x, y))
		return 0;
	return tiles[Index(x, y)];
}

void StageEditor::Set(int x, int y, int id) {
	if (!InRange(x, y))
		return;
	tiles[Index(x, y)] = id;
}

void StageEditor::ClearAll(int id) { std::fill(tiles.begin(), tiles.end(), id); }

// ========== 列挙 ==========
void StageEditor::ForEach(const std::function<void(int, int, int)>& fn, bool onlyNonZero) const {
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			int id = tiles[Index(x, y)];
			if (!onlyNonZero || id != 0)
				fn(x, y, id);
		}
	}
}

// ========== CSV I/O ==========
bool StageEditor::SaveCSV(const char* p) const {
	if (!p || !*p)
		return false;
	FILE* fp = nullptr;
#if defined(_MSC_VER)
	if (fopen_s(&fp, p, "w") != 0 || !fp)
		return false;
#else
	fp = std::fopen(p, "w");
	if (!fp)
		return false;
#endif
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			std::fprintf(fp, "%d", tiles[Index(x, y)]);
			if (x + 1 < width)
				std::fprintf(fp, ",");
		}
		std::fprintf(fp, "\n");
	}
	std::fclose(fp);
	return true;
}

bool StageEditor::LoadCSV(const char* p) {
	if (!p || !*p)
		return false;
	FILE* fp = nullptr;
#if defined(_MSC_VER)
	if (fopen_s(&fp, p, "r") != 0 || !fp)
		return false;
#else
	fp = std::fopen(p, "r");
	if (!fp)
		return false;
#endif
	std::vector<int> temp(width * height, 0);
	char line[8192];
	int y = 0;
	// --- LoadCSV(...) 内の行パース部を置き換え ---
	while (std::fgets(line, sizeof(line), fp) && y < height) {
		int x = 0;
		const char* s = line;
		while (*s && x < width) {
			// 前後の空白をスキップ
			while (*s == ' ' || *s == '\t') {
				++s;
			}

			char* endptr = nullptr;
			long v = std::strtol(s, &endptr, 10);

			if (endptr != s) {
				// 数値が読めた
				temp[Index(x, y)] = static_cast<int>(v);
				s = endptr;
			} else {
				// 数値じゃないトークンを読み飛ばす（次の区切りまで）
				while (*s && *s != ',' && *s != '\n' && *s != '\r') {
					++s;
				}
			}

			// 区切りカンマをスキップ
			if (*s == ',') {
				++s;
			}
			++x;
		}
		++y;
	}
	std::fclose(fp);
	tiles.swap(temp);
	return true;
}

// ===== Undo/Redo 実装 =====
void ge3::stage::StageEditor::ApplySet(int x, int y, int id) {
	if (!InRange(x, y))
		return;
	const int cur = Get(x, y);
	if (cur == id)
		return; // 変化なしは履歴を積まない

	// 履歴を積む（新規編集で Redo は破棄）
	undoStack_.push_back({x, y, cur, id});
	redoStack_.clear();

	// 反映
	tiles[Index(x, y)] = id;
}

void ge3::stage::StageEditor::Undo() {
	if (undoStack_.empty())
		return;
	EditDiff d = undoStack_.back();
	undoStack_.pop_back();

	// 現在値を new として Redo 側に退避（往復に強い）
	const int cur = Get(d.x, d.y);
	redoStack_.push_back({d.x, d.y, cur, d.oldId});

	// 旧値を反映
	tiles[Index(d.x, d.y)] = d.oldId;
}

void ge3::stage::StageEditor::Redo() {
	if (redoStack_.empty())
		return;
	EditDiff d = redoStack_.back();
	redoStack_.pop_back();

	// 現在値を old として Undo 側に退避
	const int cur = Get(d.x, d.y);
	undoStack_.push_back({d.x, d.y, cur, d.newId});

	// 新値を反映
	tiles[Index(d.x, d.y)] = d.newId;
}

void ge3::stage::StageEditor::ClearHistory() {
	undoStack_.clear();
	redoStack_.clear();
}


// ========== ImGui エディタ ==========
void StageEditor::UpdateEditorUI(const char* windowTitle) {
	windowTitle; // 未使用回避
#ifdef _DEBUG
	if (!editMode)
		return;

	// タイトルバーでのみ移動（中身ドラッグでウィンドウが動かない）
	ImGuiIO& io = ImGui::GetIO();
	// io.ConfigWindowsMoveFromTitleBarOnly = true;
	io.KeyRepeatDelay = 0.35f; // 押し始めからの待ち時間（初回→2回目）
	io.KeyRepeatRate = 0.05f;  // 以降の間隔（小さいほど速い）
if	ImGui::Begin(windowTitle, &editMode);

	// ---- ツールバー ----
	ImGui::Text("Map: %dx%d  cell=%.2f", width, height, cellSize);
	ImGui::SameLine();
	ImGui::Checkbox("Grid", &showGrid);

	ImGui::Separator();

	// パレット（0は消しゴム）
	ImGui::Text("Palette  (Selected: %d)", selectedId);
	if (ImGui::Button("Eraser (0)"))
		selectedId = 0;
	ImGui::SameLine();
	if (ImGui::InputInt("MaxID", &paletteMax)) {
		if (paletteMax < 1)
			paletteMax = 1;
	}
	for (int i = 1; i <= paletteMax; ++i) {
		if (i % 10 != 1)
			ImGui::SameLine();
		char label[16];
		std::snprintf(label, sizeof(label), "%d", i);
		if (ImGui::Button(label))
			selectedId = i;
	}

	ImGui::Separator();

	// 保存/読込
	ImGui::InputText("Path", path, IM_ARRAYSIZE(path));
	if (ImGui::Button("Save CSV")) {
		SaveCSV(path);
	}
	ImGui::SameLine();
	if (ImGui::Button("Load CSV")) {
		LoadCSV(path);
	}
	ImGui::SameLine();
	if (ImGui::Button("Clear")) {
		ClearAll(0); /* ClearHistory(); 任意 */
	}

	// Undo/Redo ボタン（長押しで連続実行）
	ImGui::SameLine();
	ImGui::PushButtonRepeat(true); // ★ 長押しで Button() が連続 true になる
	if (ImGui::Button("Undo (Ctrl+Z)"))
		Undo();
	ImGui::SameLine();
	if (ImGui::Button("Redo (Ctrl+Y)"))
		Redo();
	ImGui::PopButtonRepeat(); // ★ 忘れずに

	ImGui::Separator();

	// ---- キャンバス ----
	const float cellPx = 24.0f; // UI上の1セルサイズ
	const float needW = width * cellPx + 1.0f;
	const float needH = height * cellPx + 1.0f;
	const ImVec2 minCanvas((float)std::max(180.0f, needW), (float)std::max(180.0f, needH));

	ImGui::Text("Canvas:");
	ImGuiWindowFlags childFlags = ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar;
	ImGui::BeginChild("##StageCanvas", minCanvas, true, childFlags);

	ImDrawList* draw = ImGui::GetWindowDrawList();
	ImVec2 canvasPos = ImGui::GetCursorScreenPos(); // 左上（スクリーン座標）
	ImVec2 avail = ImGui::GetContentRegionAvail();
	ImVec2 canvasSize = ImVec2((avail.x < minCanvas.x) ? minCanvas.x : avail.x, (avail.y < minCanvas.y) ? minCanvas.y : avail.y);

	// 背景（+枠線）※ ImVec2 の加算は自前で
	ImVec2 bg_p0 = canvasPos;
	ImVec2 bg_p1 = ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y);
	draw->AddRectFilled(bg_p0, bg_p1, IM_COL32(30, 30, 30, 255));
	draw->AddRect(bg_p0, bg_p1, IM_COL32(80, 80, 80, 255)); // ← 色引数が必須

	// グリッド
	if (showGrid) {
		for (int x = 0; x <= width; ++x) {
			float xi = canvasPos.x + x * cellPx;
			draw->AddLine(ImVec2(xi, canvasPos.y), ImVec2(xi, canvasPos.y + height * cellPx), IM_COL32(60, 60, 60, 255));
		}
		for (int y = 0; y <= height; ++y) {
			float yi = canvasPos.y + y * cellPx;
			draw->AddLine(ImVec2(canvasPos.x, yi), ImVec2(canvasPos.x + width * cellPx, yi), IM_COL32(60, 60, 60, 255));
		}
	}

	// id→色（視認用）
	auto colorForId = [](int id) -> ImU32 {
		if (id == 0)
			return IM_COL32(0, 0, 0, 0);
		static ImU32 tbl[] = {
		    IM_COL32(0, 0, 0, 0),        IM_COL32(180, 90, 90, 255),  IM_COL32(90, 180, 90, 255),  IM_COL32(90, 90, 180, 255),  IM_COL32(180, 180, 90, 255),
		    IM_COL32(180, 90, 180, 255), IM_COL32(90, 180, 180, 255), IM_COL32(200, 120, 60, 255), IM_COL32(120, 200, 60, 255), IM_COL32(60, 120, 200, 255),
		};
		return tbl[id % (sizeof(tbl) / sizeof(tbl[0]))];
	};

	// タイル塗り
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			int id = tiles[Index(x, y)];
			if (id == 0)
				continue;
			ImVec2 p0(canvasPos.x + x * cellPx, canvasPos.y + y * cellPx);
			ImVec2 p1(canvasPos.x + (x + 1) * cellPx - 1.0f, canvasPos.y + (y + 1) * cellPx - 1.0f);
			draw->AddRectFilled(p0, p1, colorForId(id));
		}
	}

	// InvisibleButton で入力をこの領域に束縛（※ SetItemUsingMouseWheel は古い版には無いので未使用）
	ImGui::SetCursorScreenPos(canvasPos);
	ImGui::InvisibleButton("##StageCanvasBtn", canvasSize, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
	bool hovered = ImGui::IsItemHovered();
	bool activeL = ImGui::IsItemActive() && ImGui::IsMouseDown(ImGuiMouseButton_Left);
	bool activeR = ImGui::IsItemActive() && ImGui::IsMouseDown(ImGuiMouseButton_Right);

	// ホバー／ドラッグ編集
	hoverX = hoverY = -1;
	if (hovered || activeL || activeR) {
		ImVec2 mouse = io.MousePos;
		bool inside = (mouse.x >= canvasPos.x && mouse.x < canvasPos.x + width * cellPx && mouse.y >= canvasPos.y && mouse.y < canvasPos.y + height * cellPx);
		if (inside) {
			int mx = (int)((mouse.x - canvasPos.x) / cellPx);
			int my = (int)((mouse.y - canvasPos.y) / cellPx);
			if (InRange(mx, my)) {
				hoverX = mx;
				hoverY = my;

				// ハイライト
				ImVec2 hp0(canvasPos.x + mx * cellPx, canvasPos.y + my * cellPx);
				ImVec2 hp1(canvasPos.x + (mx + 1) * cellPx - 1.0f, canvasPos.y + (my + 1) * cellPx - 1.0f);
				draw->AddRect(hp0, hp1, IM_COL32(255, 255, 255, 180), 0.0f, 0, 2.0f);

				// スポイト（中クリック / Ctrl+右 / Alt+右 / 'E'）
				bool wantPick =
				    ImGui::IsMouseClicked(ImGuiMouseButton_Middle) || (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && (io.KeyCtrl || io.KeyAlt)) || ImGui::IsKeyPressed(ImGuiKey_E, false);
				if (wantPick) {
					int id = Get(mx, my);
					selectedId = (id >= 0) ? id : 0;
				}

				// ドラッグ開始/終了（スポイト修飾除く）
				if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
					leftDragPainting = true;
				if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
					rightDragErasing = true;
				if (leftDragPainting && !ImGui::IsMouseDown(ImGuiMouseButton_Left))
					leftDragPainting = false;
				if (rightDragErasing && !ImGui::IsMouseDown(ImGuiMouseButton_Right))
					rightDragErasing = false;

				// 編集（履歴付き）
				if (leftDragPainting)
					ApplySet(mx, my, selectedId);
				if (rightDragErasing && !io.KeyCtrl && !io.KeyAlt)
					ApplySet(mx, my, 0);
			}
		} else {
			if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
				leftDragPainting = false;
			if (!ImGui::IsMouseDown(ImGuiMouseButton_Right))
				rightDragErasing = false;
		}
	}

	ImGui::EndChild();

	// 補助表示 & ショートカット
	ImGui::Spacing();
	if (hoverX >= 0)
		ImGui::Text("Hover: (%d,%d)  id=%d", hoverX, hoverY, Get(hoverX, hoverY));
	else
		ImGui::TextUnformatted("Hover: (-,-)");

	bool ctrl = io.KeyCtrl;
	bool shift = io.KeyShift;
	// ★ 第2引数を true にするとオートリピート（長押し）で複数回 true が返る
	if (ctrl && ImGui::IsKeyPressed(ImGuiKey_Z, /*repeat=*/true) && !shift) {
		Undo();
	}
	if ((ctrl && ImGui::IsKeyPressed(ImGuiKey_Y, /*repeat=*/true)) || (ctrl && shift && ImGui::IsKeyPressed(ImGuiKey_Z, /*repeat=*/true))) {
		Redo();
	}

	ImGui::End();
#endif
}

} // namespace stage
} // namespace ge3
