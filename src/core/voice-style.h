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

#include "portrait.h"
#include "style-schema.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace sspot::core {

// 設定の値を、CSS とドックのプレビューで使う形に読んだもの。値の解釈をここだけに置く
struct VoiceLook {
	int avatar_height = 0;     // 立ち絵とアバターの表示の高さ (px)
	int idle_brightness = 100; // 待機中の明るさ (%)。CSS の filter: brightness() の 100 倍
	int speaking_brightness = 100;
	int transition_ms = 0;
	bool speaking_border_visible = true;
	bool name_visible = true;
	std::string name_font;         // 空なら StreamKit の書体のまま
	std::optional<int> name_size;  // px。無ければ StreamKit の大きさのまま
	std::optional<Rgb> name_color; // 無ければ StreamKit の色のまま
	std::optional<Rgb> name_background;
};

VoiceLook resolve_look(const StyleValues &values);

// CSS に埋め込める立ち絵のときに true を返す。形式が分からない立ち絵と、base64 に使わない文字を含む立ち絵は使わない
bool portrait_usable(const PortraitImage &portrait);

// CSS の filter: brightness() と同じく、色の成分に percent / 100 を掛けて 255 で頭打ちにする
std::uint8_t brighten(std::uint8_t channel, int percent);

// 0xAARRGGBB の画素の並びの RGB に brighten() を掛ける。透明度は変えない
void brighten_pixels(std::uint32_t *pixels, std::size_t count, int percent);

// 設定の値と 1 人のユーザー ID から、ブラウザーソースの管理範囲に書く CSS を作る。
// user_id の参加者だけを表示して左上に置き、portrait があればアバターを置き換える。
// user_id は is_valid_user_id() を通った値だけを渡す。
// 形式が分からない立ち絵、base64 に使わない文字を含む立ち絵は CSS に出さず、アバターのまま表示する
std::string generate_css(const StyleValues &values, std::string_view user_id,
			 const std::optional<PortraitImage> &portrait = std::nullopt);

enum class CssUpdateProblem {
	None,
	BrokenMarkers, // 管理範囲の目印が壊れている
	InvalidUserId, // ユーザー ID が is_valid_user_id() を通らない
};

struct CssUpdate {
	CssUpdateProblem problem = CssUpdateProblem::None;
	std::string css; // problem が None のときの、書き込む css の全文
	// problem が None で、css が書き込み済みの css と違うときに true。同じ css を書くとページが作り直されるだけのため、
	// false なら書き込まない
	bool changed = false;
};

// 書き込み済みの css に設定を反映した css を作る。user_id が空なら、プラグインの CSS を書かず管理範囲を外す。
// 管理範囲の外の CSS は残す
CssUpdate update_css(std::string_view current_css, const StyleValues &values, std::string_view user_id,
		     const std::optional<PortraitImage> &portrait);

} // namespace sspot::core
