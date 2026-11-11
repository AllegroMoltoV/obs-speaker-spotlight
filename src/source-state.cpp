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
constexpr const char *STYLE_KEY = "style";
constexpr const char *USER_ID_KEY = "user_id";
constexpr const char *PORTRAIT_KEY = "portrait";
constexpr const char *MIME_KEY = "mime";
constexpr const char *BASE64_KEY = "base64";
constexpr const char *WIDTH_KEY = "width";
constexpr const char *HEIGHT_KEY = "height";

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

std::optional<core::PortraitImage> portrait_from_data(obs_data_t *data)
{
	if (!data)
		return std::nullopt;
	core::PortraitImage portrait;
	portrait.format = core::format_from_mime(obs_data_get_string(data, MIME_KEY));
	portrait.base64 = obs_data_get_string(data, BASE64_KEY);
	portrait.width = static_cast<int>(obs_data_get_int(data, WIDTH_KEY));
	portrait.height = static_cast<int>(obs_data_get_int(data, HEIGHT_KEY));
	if (portrait.format == core::ImageFormat::Unknown || portrait.base64.empty())
		return std::nullopt;
	return portrait;
}

} // namespace

SpeakerSettings load_speaker_settings(obs_source_t *source)
{
	SpeakerSettings settings;
	obs_data_t *priv = obs_source_get_private_settings(source);
	obs_data_t *state = obs_data_get_obj(priv, STATE_KEY);
	obs_data_t *style = state ? obs_data_get_obj(state, STYLE_KEY) : nullptr;
	obs_data_t *portrait = state ? obs_data_get_obj(state, PORTRAIT_KEY) : nullptr;
	settings.style = style_from_data(style, core::voice_sections());
	settings.user_id = state ? obs_data_get_string(state, USER_ID_KEY) : "";
	settings.portrait = portrait_from_data(portrait);
	if (portrait)
		obs_data_release(portrait);
	if (style)
		obs_data_release(style);
	if (state)
		obs_data_release(state);
	obs_data_release(priv);
	return settings;
}

void save_style(obs_source_t *source, const core::StyleValues &values)
{
	edit_state(source, [&](obs_data_t *state) {
		obs_data_t *style = style_to_data(values);
		obs_data_set_obj(state, STYLE_KEY, style);
		obs_data_release(style);
	});
}

void save_user_id(obs_source_t *source, const std::string &user_id)
{
	edit_state(source, [&](obs_data_t *state) { obs_data_set_string(state, USER_ID_KEY, user_id.c_str()); });
}

void save_portrait(obs_source_t *source, const std::optional<core::PortraitImage> &portrait)
{
	edit_state(source, [&](obs_data_t *state) {
		if (!portrait) {
			obs_data_erase(state, PORTRAIT_KEY);
			return;
		}
		obs_data_t *data = obs_data_create();
		obs_data_set_string(data, MIME_KEY, std::string(core::mime_type(portrait->format)).c_str());
		obs_data_set_string(data, BASE64_KEY, portrait->base64.c_str());
		obs_data_set_int(data, WIDTH_KEY, portrait->width);
		obs_data_set_int(data, HEIGHT_KEY, portrait->height);
		obs_data_set_obj(state, PORTRAIT_KEY, data);
		obs_data_release(data);
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
