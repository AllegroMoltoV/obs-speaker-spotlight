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

#include "core/style-schema.h"

#include <string>
#include <vector>

namespace sspot {

// 見た目の設定だけを持つ。ユーザー ID と立ち絵はソースごとの設定のため含めない
struct Preset {
	std::string name;
	core::StyleValues values;
};

// プラグインの設定のディレクトリの presets.json を読み書きする。ファイルが無ければ空の一覧を返す
std::vector<Preset> load_presets();
bool save_presets(const std::vector<Preset> &presets);

} // namespace sspot
