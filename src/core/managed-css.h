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

#include <optional>
#include <string>
#include <string_view>

namespace sspot::core {

// ブラウザーソースの css のうち、プラグインが書き換えてよい範囲の目印
inline constexpr std::string_view BLOCK_BEGIN = "/* speaker-spotlight begin */";
inline constexpr std::string_view BLOCK_END = "/* speaker-spotlight end */";

enum class BlockState {
	Absent, // 目印が無い
	Intact, // 開始と終了の目印が 1 組だけ、この順にある
	Broken, // 片方だけ、複数組、逆順のいずれか
};

BlockState inspect_block(std::string_view css);

// 管理範囲を generated で置き換えた css を返す。範囲が無ければ末尾に足す。
// 目印が壊れているときは、利用者の CSS を消さないよう std::nullopt を返す。
std::optional<std::string> replace_block(std::string_view css, std::string_view generated);

} // namespace sspot::core
