#pragma once
#include <string>
#include <vector>

namespace MapLoader {

// CSVを 0/1/… の2次元配列に読み込む（成功=true）
bool LoadCsv(const std::string& path, std::vector<std::vector<int>>& out);

// 行数・列数が揃っていないCSVを整形（短い行は右側を0で埋める）
void NormalizeRect(std::vector<std::vector<int>>& grid, int fill = 0);

} // namespace MapLoader


