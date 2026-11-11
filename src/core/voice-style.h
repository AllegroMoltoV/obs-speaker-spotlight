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

#include <string>
#include <string_view>
#include <vector>

namespace sspot::core {

// 立ち絵の表示の高さ (px)。埋め込む画像の縮小の基準にも使う
int avatar_height(const StyleValues &values);

// 設定の値と立ち絵から、ブラウザーソースの管理範囲に書く CSS を作る。
// ユーザー ID が不正な立ち絵、形式が分からない立ち絵、base64 に使わない文字を含む立ち絵、
// 先に現れたものと同じユーザー ID の立ち絵は CSS に出さない
std::string generate_css(const StyleValues &values, const std::vector<Portrait> &portraits = {});

// ドックに表示するため、CSS の中の data URI の base64 の本文を省く。
// count_format の %1 を省いた文字数に置き換えて本文の代わりに置く。文字数は 3 桁ごとにコンマで区切る
std::string abbreviate_data_uris(std::string_view css, std::string_view count_format = "\xE2\x80\xA6(%1 chars)");

} // namespace sspot::core
