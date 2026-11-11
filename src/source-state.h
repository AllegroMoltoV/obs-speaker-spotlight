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

#include "core/portrait.h"
#include "core/style-schema.h"

#include <obs.h>

#include <string>
#include <vector>

namespace sspot {

// ブラウザーソースごとの状態。private settings の "speaker_spotlight" に置き、シーンコレクションと一緒に保存される
struct SourceState {
	bool has_previous = false;
	std::string previous_css; // 最後の適用の前の css
	std::string applied_css;  // 最後の適用の直後の css
};

SourceState load_source_state(obs_source_t *source);
void save_apply_record(obs_source_t *source, const std::string &previous_css, const std::string &applied_css);
void clear_apply_record(obs_source_t *source);

// 見た目の設定。private settings の speaker_spotlight の "style" に置く
core::StyleValues load_style(obs_source_t *source);
void save_style(obs_source_t *source, const core::StyleValues &values);

// 立ち絵の一覧。private settings の speaker_spotlight の "portraits" に置く。画像の中身は置かない
std::vector<core::PortraitEntry> load_portraits(obs_source_t *source);
void save_portraits(obs_source_t *source, const std::vector<core::PortraitEntry> &entries);

std::string get_source_url(obs_source_t *source);
std::string get_source_css(obs_source_t *source);
// css を書き換える。ブラウザーソースはページを読み込み直す
void set_source_css(obs_source_t *source, const std::string &css);

} // namespace sspot
