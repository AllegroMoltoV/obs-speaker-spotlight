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

#include <optional>
#include <string>
#include <string_view>

namespace sspot::core {

// 立ち絵の表示の高さ (px)
int avatar_height(const StyleValues &values);

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
};

// 書き込み済みの css に設定を反映した css を作る。user_id が空なら、プラグインの CSS を書かず管理範囲を外す。
// 管理範囲の外の CSS は残す
CssUpdate update_css(std::string_view current_css, const StyleValues &values, std::string_view user_id,
		     const std::optional<PortraitImage> &portrait);

} // namespace sspot::core
