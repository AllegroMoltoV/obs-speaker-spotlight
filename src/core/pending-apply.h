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

// 最後の操作からソースへ反映するまでの待ち (ms)。css を書き換えるとページが作り直されて表示が途切れるため、
// 調整中の操作をまとめて 1 回の書き込みにする
inline constexpr int APPLY_DELAY_MS = 2000;

// 反映待ちのソース。待ちは操作のたびに始め直す。
// 選択が別のソースへ移った後にも元のソースへ書き込めるよう、反映待ちは選択中のソースではなくソースの ID に結び付ける
class PendingApply {
public:
	// source_id を反映待ちにする。別のソースの反映待ちが残っていれば、先に書き込めるよう、その ID を返す
	std::optional<std::string> mark(std::string_view source_id);
	// 反映待ちのソースの ID を取り出し、反映待ちを無くす
	std::optional<std::string> take();
	bool pending() const { return source_id_.has_value(); }

private:
	std::optional<std::string> source_id_;
};

} // namespace sspot::core
