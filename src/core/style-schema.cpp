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

#include "style-schema.h"

#include <algorithm>

namespace sspot::core {

const std::vector<SectionDef> &voice_sections()
{
	// キーは保存とプリセットに使うため、変えるときは古いキーの読み込みも考える
	static const std::vector<SectionDef> sections = {
		{"Section.Brightness",
		 {
			 {"idle_brightness", ItemKind::Number, "Item.IdleBrightness", 0, 200, 50, "%", {}, {}},
			 {"speaking_brightness", ItemKind::Number, "Item.SpeakingBrightness", 0, 200, 100, "%", {}, {}},
			 // 雑音で 0.1 秒ごとに発話と待機が入れ替わることがあるため、既定でも緩やかに切り替える
			 {"transition_ms", ItemKind::Number, "Item.TransitionTime", 0, 2000, 200, "ms", {}, {}},
		 }},
		{"Section.Speaking",
		 {
			 {"speaking_border_visible", ItemKind::Toggle, "Item.SpeakingBorderVisible", 0, 1, 1, {}, {}, {}},
		 }},
		{"Section.Avatar",
		 {
			 {"avatar_height", ItemKind::Number, "Item.AvatarHeight", 20, 600, 150, "px", {}, {}},
		 }},
	};
	return sections;
}

bool item_effective(const ItemDef &item, const std::vector<SectionDef> &sections, const StyleValues &values)
{
	if (item.enabled_when.empty())
		return true;
	for (const auto &condition : item.enabled_when) {
		const ItemDef *target = find_item(sections, condition.key);
		if (!target)
			continue;
		switch (target->kind) {
		case ItemKind::Toggle: {
			const auto it = values.numbers.find(condition.key);
			const int value = it != values.numbers.end() ? it->second : target->default_value.value_or(0);
			if ((value != 0) == (condition.value == "on"))
				return true;
			break;
		}
		case ItemKind::Choice: {
			const auto it = values.texts.find(condition.key);
			const std::string current = it != values.texts.end() ? it->second
									     : std::string(target->choices.front());
			const bool negate = !condition.value.empty() && condition.value.front() == '!';
			const std::string_view expected = negate ? condition.value.substr(1) : condition.value;
			if ((current == expected) != negate)
				return true;
			break;
		}
		case ItemKind::Number:
		case ItemKind::Opacity:
			break;
		}
	}
	return false;
}

const ItemDef *find_item(const std::vector<SectionDef> &sections, std::string_view key)
{
	for (const auto &section : sections)
		for (const auto &item : section.items)
			if (item.key == key)
				return &item;
	return nullptr;
}

StyleValues sanitize(const std::vector<SectionDef> &sections, const StyleValues &values)
{
	StyleValues result;
	for (const auto &[key, value] : values.numbers) {
		const ItemDef *item = find_item(sections, key);
		if (!item || (item->kind != ItemKind::Number && item->kind != ItemKind::Opacity &&
			      item->kind != ItemKind::Toggle))
			continue;
		result.numbers[key] = std::clamp(value, item->min, item->max);
	}
	for (const auto &[key, value] : values.texts) {
		const ItemDef *item = find_item(sections, key);
		if (item && item->kind == ItemKind::Choice &&
		    std::find(item->choices.begin(), item->choices.end(), value) != item->choices.end())
			result.texts[key] = value;
	}
	return result;
}

} // namespace sspot::core
