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
#include "core/voice-style.h"

#include <obs.h>

#include <optional>
#include <string>

namespace sspot {

// ブラウザーソースごとの設定。private settings の "speaker_spotlight" に置き、シーンコレクションと一緒に保存される。
// "style" (見た目の設定)、"user_id" (文字列)、"portrait" (立ち絵。無いときはキーを置かない) を持つ
struct SpeakerSettings {
	core::StyleValues style;
	std::string user_id;
	std::optional<core::PortraitImage> portrait;
};

SpeakerSettings load_speaker_settings(obs_source_t *source);
void save_style(obs_source_t *source, const core::StyleValues &values);
void save_user_id(obs_source_t *source, const std::string &user_id);
// 縮小して書き出した画像の base64 と形式と寸法を保存する。nullopt では立ち絵を外す
void save_portrait(obs_source_t *source, const std::optional<core::PortraitImage> &portrait);

std::string get_source_url(obs_source_t *source);
std::string get_source_css(obs_source_t *source);
core::SourceSize get_source_size(obs_source_t *source);
// css と大きさのうち値のあるものを、obs_source_update() の 1 回で書き換える。css を変えるとブラウザーソースはページを
// 読み込み直す。大きさだけを変えるときは読み込み直さない
void update_source(obs_source_t *source, const std::optional<std::string> &css,
		   const std::optional<core::SourceSize> &size);

} // namespace sspot
