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

} // namespace sspot
