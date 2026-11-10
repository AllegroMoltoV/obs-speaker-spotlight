/*
Voice Spotlight
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

#include <obs.h>

namespace vspot {

// obs_data の値を設定の値にする。表に無いキーと範囲の外の値は捨てる
core::StyleValues style_from_data(obs_data_t *data, const std::vector<core::SectionDef> &sections);

// 設定の値を新しい obs_data にする。呼び出し側が obs_data_release() で返す
obs_data_t *style_to_data(const core::StyleValues &values);

} // namespace vspot
