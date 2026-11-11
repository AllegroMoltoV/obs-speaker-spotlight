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

#include "source-state.h"

#include "style-data.h"

namespace sspot {

namespace {

constexpr const char *STATE_KEY = "speaker_spotlight";
constexpr const char *HAS_PREVIOUS_KEY = "has_previous";
constexpr const char *PREVIOUS_CSS_KEY = "previous_css";
constexpr const char *APPLIED_FINGERPRINT_KEY = "applied_css_fingerprint";
constexpr const char *STYLE_KEY = "style";
constexpr const char *PORTRAITS_KEY = "portraits";

// private settings の中の状態のオブジェクトを読み書きする。無ければ作る
template<typename Fn> void edit_state(obs_source_t *source, Fn &&fn)
{
	obs_data_t *priv = obs_source_get_private_settings(source);
	obs_data_t *state = obs_data_get_obj(priv, STATE_KEY);
	if (!state)
		state = obs_data_create();
	fn(state);
	obs_data_set_obj(priv, STATE_KEY, state);
	obs_data_release(state);
	obs_data_release(priv);
}

std::string get_string_setting(obs_source_t *source, const char *key)
{
	obs_data_t *settings = obs_source_get_settings(source);
	std::string value = obs_data_get_string(settings, key);
	obs_data_release(settings);
	return value;
}

} // namespace

SourceState load_source_state(obs_source_t *source)
{
	SourceState result;
	obs_data_t *priv = obs_source_get_private_settings(source);
	obs_data_t *state = obs_data_get_obj(priv, STATE_KEY);
	if (state) {
		result.has_previous = obs_data_get_bool(state, HAS_PREVIOUS_KEY);
		result.previous_css = obs_data_get_string(state, PREVIOUS_CSS_KEY);
		result.applied_fingerprint = obs_data_get_string(state, APPLIED_FINGERPRINT_KEY);
		obs_data_release(state);
	}
	obs_data_release(priv);
	return result;
}

void save_apply_record(obs_source_t *source, const std::string &previous_css, const std::string &applied_fingerprint)
{
	edit_state(source, [&](obs_data_t *state) {
		obs_data_set_bool(state, HAS_PREVIOUS_KEY, true);
		obs_data_set_string(state, PREVIOUS_CSS_KEY, previous_css.c_str());
		obs_data_set_string(state, APPLIED_FINGERPRINT_KEY, applied_fingerprint.c_str());
	});
}

void clear_apply_record(obs_source_t *source)
{
	edit_state(source, [](obs_data_t *state) {
		obs_data_set_bool(state, HAS_PREVIOUS_KEY, false);
		obs_data_erase(state, PREVIOUS_CSS_KEY);
		obs_data_erase(state, APPLIED_FINGERPRINT_KEY);
	});
}

core::StyleValues load_style(obs_source_t *source)
{
	obs_data_t *priv = obs_source_get_private_settings(source);
	obs_data_t *state = obs_data_get_obj(priv, STATE_KEY);
	obs_data_t *style = state ? obs_data_get_obj(state, STYLE_KEY) : nullptr;
	const core::StyleValues values = style_from_data(style, core::voice_sections());
	if (style)
		obs_data_release(style);
	if (state)
		obs_data_release(state);
	obs_data_release(priv);
	return values;
}

void save_style(obs_source_t *source, const core::StyleValues &values)
{
	edit_state(source, [&](obs_data_t *state) {
		obs_data_t *style = style_to_data(values);
		obs_data_set_obj(state, STYLE_KEY, style);
		obs_data_release(style);
	});
}

std::vector<core::PortraitEntry> load_portraits(obs_source_t *source)
{
	obs_data_t *priv = obs_source_get_private_settings(source);
	obs_data_t *state = obs_data_get_obj(priv, STATE_KEY);
	obs_data_array_t *array = state ? obs_data_get_array(state, PORTRAITS_KEY) : nullptr;
	std::vector<core::PortraitEntry> entries = portraits_from_array(array);
	if (array)
		obs_data_array_release(array);
	if (state)
		obs_data_release(state);
	obs_data_release(priv);
	return entries;
}

void save_portraits(obs_source_t *source, const std::vector<core::PortraitEntry> &entries)
{
	edit_state(source, [&](obs_data_t *state) {
		obs_data_array_t *array = portraits_to_array(entries);
		obs_data_set_array(state, PORTRAITS_KEY, array);
		obs_data_array_release(array);
	});
}

std::string get_source_url(obs_source_t *source)
{
	return get_string_setting(source, "url");
}

std::string get_source_css(obs_source_t *source)
{
	return get_string_setting(source, "css");
}

void set_source_css(obs_source_t *source, const std::string &css)
{
	obs_data_t *update = obs_data_create();
	obs_data_set_string(update, "css", css.c_str());
	obs_source_update(source, update);
	obs_data_release(update);
}

} // namespace sspot
