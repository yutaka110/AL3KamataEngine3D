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

// ========== ImGui エディタ ==========
void StageEditor::UpdateEditorUI(const char* windowTitle) {
	if (!editMode)
		return;

	ImGui::Begin(windowTitle, &editMode);

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
	// IDボタンを横並びで
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
		ClearAll(0);
	}

	ImGui::Separator();

	// ---- キャンバス ----
	const float cellPx = 24.0f; // 見た目の1セルサイズ（UI上）
	const ImVec2 canvasSize(std::max(180.0f, width * cellPx + 1.0f), std::max(180.0f, height * cellPx + 1.0f));

	ImGui::Text("Canvas:");
	ImGui::BeginChild("##StageCanvas", canvasSize, true, ImGuiWindowFlags_NoScrollWithMouse);
	ImDrawList* draw = ImGui::GetWindowDrawList();
	const ImVec2 origin = ImGui::GetCursorScreenPos();

	// 背景
	draw->AddRectFilled(origin, ImVec2(origin.x + canvasSize.x, origin.y + canvasSize.y), IM_COL32(30, 30, 30, 255));

	// グリッド
	if (showGrid) {
		for (int x = 0; x <= width; ++x) {
			float xi = origin.x + x * cellPx;
			draw->AddLine(ImVec2(xi, origin.y), ImVec2(xi, origin.y + height * cellPx), IM_COL32(60, 60, 60, 255));
		}
		for (int y = 0; y <= height; ++y) {
			float yi = origin.y + y * cellPx;
			draw->AddLine(ImVec2(origin.x, yi), ImVec2(origin.x + width * cellPx, yi), IM_COL32(60, 60, 60, 255));
		}
	}

	// id→色（視認用の適当なテーブル）
	auto colorForId = [](int id) -> ImU32 {
		if (id == 0)
			return IM_COL32(0, 0, 0, 0);
		static ImU32 tbl[] = {
		    IM_COL32(0, 0, 0, 0),        IM_COL32(180, 90, 90, 255),  IM_COL32(90, 180, 90, 255),  IM_COL32(90, 90, 180, 255),  IM_COL32(180, 180, 90, 255),
		    IM_COL32(180, 90, 180, 255), IM_COL32(90, 180, 180, 255), IM_COL32(200, 120, 60, 255), IM_COL32(120, 200, 60, 255), IM_COL32(60, 120, 200, 255),
		};
		return tbl[id % (sizeof(tbl) / sizeof(tbl[0]))];
	};

	// セル塗り
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			int id = tiles[Index(x, y)];
			if (id == 0)
				continue;
			ImVec2 p0(origin.x + x * cellPx, origin.y + y * cellPx);
			ImVec2 p1(origin.x + (x + 1) * cellPx - 1, origin.y + (y + 1) * cellPx - 1);
			draw->AddRectFilled(p0, p1, colorForId(id));
		}
	}

	// ホバー＆ドラッグ編集
	hoverX = hoverY = -1;
	const ImVec2 mouse = ImGui::GetIO().MousePos;
	const bool inside = (mouse.x >= origin.x && mouse.x < origin.x + width * cellPx && mouse.y >= origin.y && mouse.y < origin.y + height * cellPx);

	if (inside) {
		int mx = (int)((mouse.x - origin.x) / cellPx);
		int my = (int)((mouse.y - origin.y) / cellPx);
		if (InRange(mx, my)) {
			hoverX = mx;
			hoverY = my;
			// ハイライト
			ImVec2 p0(origin.x + mx * cellPx, origin.y + my * cellPx);
			ImVec2 p1(origin.x + (mx + 1) * cellPx - 1, origin.y + (my + 1) * cellPx - 1);
			draw->AddRect(p0, p1, IM_COL32(255, 255, 255, 180), 0.0f, 0, 2.0f);

			// 左ボタン：ペイント、右ボタン：消しゴム
			if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
				leftDragPainting = true;
			if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
				rightDragErasing = true;

			if (leftDragPainting && !ImGui::IsMouseDown(ImGuiMouseButton_Left))
				leftDragPainting = false;
			if (rightDragErasing && !ImGui::IsMouseDown(ImGuiMouseButton_Right))
				rightDragErasing = false;

			if (leftDragPainting)
				Set(mx, my, selectedId);
			if (rightDragErasing)
				Set(mx, my, 0);
		}
	} else {
		// キャンバス外でボタンを離したらドラッグ解除
		if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
			leftDragPainting = false;
		if (!ImGui::IsMouseDown(ImGuiMouseButton_Right))
			rightDragErasing = false;
	}

	ImGui::EndChild();

	// 補助表示
	ImGui::Spacing();
	if (hoverX >= 0)
		ImGui::Text("Hover: (%d,%d)  id=%d", hoverX, hoverY, Get(hoverX, hoverY));
	else
		ImGui::TextUnformatted("Hover: (-,-)");

	ImGui::End();
}

} // namespace stage
} // namespace ge3
