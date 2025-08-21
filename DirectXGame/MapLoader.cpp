#include "MapLoader.h"
#include <cctype>
#include <cstdio>

namespace {
inline void rstrip(char* s) {
	int n = (int)std::strlen(s);
	while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r'))
		s[--n] = '\0';
}
inline void trim(char*& p) {
	while (*p && std::isspace((unsigned char)*p))
		++p;
}
} // namespace

bool MapLoader::LoadCsv(const std::string& path, std::vector<std::vector<int>>& out) {
	out.clear();
	std::FILE* fp = nullptr;
	if (fopen_s(&fp, path.c_str(), "rb") != 0 || !fp)
		return false;

	char line[4096];
	while (std::fgets(line, sizeof(line), fp)) {
		rstrip(line);
		std::vector<int> row;
		char* p = line;
		while (*p) {
			trim(p);
			// トークンを切り出し
			char* q = p;
			while (*q && *q != ',')
				++q;

			int v = 0;
			if (q > p)
				v = std::atoi(p);
			row.push_back(v);

			if (*q == ',') {
				p = q + 1;
			} else {
				break;
			}
		}
		if (!row.empty())
			out.emplace_back(std::move(row));
	}
	std::fclose(fp);
	return !out.empty();
}

void MapLoader::NormalizeRect(std::vector<std::vector<int>>& grid, int fill) {
	size_t cols = 0;
	for (auto& r : grid)
		if (r.size() > cols)
			cols = r.size();
	for (auto& r : grid)
		if (r.size() < cols)
			r.resize(cols, fill);
}
