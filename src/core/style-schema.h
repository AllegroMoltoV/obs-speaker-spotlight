/*
Speaker Spotlight
Copyright (C) 2026 AllegroMoltoV <contact@allegromoltov.jp>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace sspot::core {

struct Rgb {
	std::uint8_t r = 0;
	std::uint8_t g = 0;
	std::uint8_t b = 0;

	bool operator==(const Rgb &other) const { return r == other.r && g == other.g && b == other.b; }
};

enum class ItemKind {
	Number,  // 整数。default_value が無ければ、最小値の 1 つ下を「既定」として扱う
	Opacity, // 0 から 100 の百分率
	Toggle,  // 0 か 1
	Choice,  // choices のいずれか
};

// 項目が効くための前提。値の書き方は、Toggle は "on"、Choice は値そのもの、Choice の否定は "!値"。
// いずれか 1 つが満たされれば効く
struct Condition {
	std::string_view key;
	std::string_view value;
};

struct ItemDef {
	std::string_view key; // 保存とプリセットの名前
	ItemKind kind;
	std::string_view label_key; // ロケールのキー
	int min = 0;
	int max = 0;
	std::optional<int> default_value;      // Number・Opacity・Toggle の既定値
	std::string_view unit;                 // Number の単位の表示
	std::vector<std::string_view> choices; // Choice の値。先頭が既定。表示は "Choice.<値>"
	std::vector<Condition> enabled_when;   // 空なら常に効く
};

struct SectionDef {
	std::string_view title_key;
	std::vector<ItemDef> items;
};

// 見た目の設定項目の表
const std::vector<SectionDef> &voice_sections();
const ItemDef *find_item(const std::vector<SectionDef> &sections, std::string_view key);

// 設定の値。項目が無いときは既定の見た目のままにする
struct StyleValues {
	std::map<std::string, int, std::less<>> numbers;       // Number・Opacity・Toggle
	std::map<std::string, std::string, std::less<>> texts; // Choice

	bool operator==(const StyleValues &other) const { return numbers == other.numbers && texts == other.texts; }
};

// 項目が今の値で効くときに true を返す。ドックで効かない欄を押せない状態にするために使う
bool item_effective(const ItemDef &item, const std::vector<SectionDef> &sections, const StyleValues &values);

// 範囲の外の値と、表に無い項目と、選択肢に無い値を捨てる
StyleValues sanitize(const std::vector<SectionDef> &sections, const StyleValues &values);

} // namespace sspot::core
