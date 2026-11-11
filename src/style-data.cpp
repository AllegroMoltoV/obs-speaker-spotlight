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

#include "style-data.h"

namespace sspot {

namespace {

constexpr const char *USER_ID_KEY = "user_id";
constexpr const char *IMAGE_PATH_KEY = "image_path";
constexpr const char *MEMO_KEY = "memo";

} // namespace

core::StyleValues style_from_data(obs_data_t *data, const std::vector<core::SectionDef> &sections)
{
	core::StyleValues values;
	if (!data)
		return values;
	for (const auto &section : sections) {
		for (const auto &item : section.items) {
			const std::string key(item.key);
			if (!obs_data_has_user_value(data, key.c_str()))
				continue;
			switch (item.kind) {
			case core::ItemKind::Number:
			case core::ItemKind::Opacity:
			case core::ItemKind::Toggle:
				values.numbers[key] = static_cast<int>(obs_data_get_int(data, key.c_str()));
				break;
			case core::ItemKind::Color:
				if (const auto color = core::parse_hex_color(obs_data_get_string(data, key.c_str())))
					values.colors[key] = *color;
				break;
			case core::ItemKind::Choice:
			case core::ItemKind::Font:
				values.texts[key] = obs_data_get_string(data, key.c_str());
				break;
			}
		}
	}
	return core::sanitize(sections, values);
}

obs_data_t *style_to_data(const core::StyleValues &values)
{
	obs_data_t *data = obs_data_create();
	for (const auto &[key, value] : values.numbers)
		obs_data_set_int(data, key.c_str(), value);
	for (const auto &[key, value] : values.colors)
		obs_data_set_string(data, key.c_str(), core::to_hex_color(value).c_str());
	for (const auto &[key, value] : values.texts)
		obs_data_set_string(data, key.c_str(), value.c_str());
	return data;
}

std::vector<core::PortraitEntry> portraits_from_array(obs_data_array_t *array)
{
	std::vector<core::PortraitEntry> entries;
	const size_t count = array ? obs_data_array_count(array) : 0;
	for (size_t i = 0; i < count; ++i) {
		obs_data_t *item = obs_data_array_item(array, i);
		entries.push_back({obs_data_get_string(item, USER_ID_KEY), obs_data_get_string(item, IMAGE_PATH_KEY),
				   obs_data_get_string(item, MEMO_KEY)});
		obs_data_release(item);
	}
	return entries;
}

obs_data_array_t *portraits_to_array(const std::vector<core::PortraitEntry> &entries)
{
	obs_data_array_t *array = obs_data_array_create();
	for (const auto &entry : entries) {
		obs_data_t *item = obs_data_create();
		obs_data_set_string(item, USER_ID_KEY, entry.user_id.c_str());
		obs_data_set_string(item, IMAGE_PATH_KEY, entry.image_path.c_str());
		obs_data_set_string(item, MEMO_KEY, entry.memo.c_str());
		obs_data_array_push_back(array, item);
		obs_data_release(item);
	}
	return array;
}

} // namespace sspot
