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

#include "preset-store.h"

#include "style-data.h"

#include <obs-module.h>
#include <plugin-support.h>
#include <util/platform.h>

namespace sspot {

namespace {

constexpr const char *FILE_NAME = "presets.json";
constexpr const char *PRESETS_KEY = "presets";
constexpr const char *NAME_KEY = "name";
constexpr const char *VALUES_KEY = "values";
constexpr const char *PORTRAITS_KEY = "portraits";

} // namespace

std::vector<Preset> load_presets()
{
	std::vector<Preset> presets;
	char *path = obs_module_config_path(FILE_NAME);
	if (!path)
		return presets;
	obs_data_t *root = os_file_exists(path) ? obs_data_create_from_json_file_safe(path, "bak") : nullptr;
	bfree(path);
	if (!root)
		return presets;

	obs_data_array_t *array = obs_data_get_array(root, PRESETS_KEY);
	const size_t count = array ? obs_data_array_count(array) : 0;
	for (size_t i = 0; i < count; ++i) {
		obs_data_t *item = obs_data_array_item(array, i);
		Preset preset;
		preset.name = obs_data_get_string(item, NAME_KEY);
		obs_data_t *values = obs_data_get_obj(item, VALUES_KEY);
		preset.values = style_from_data(values, core::voice_sections());
		if (values)
			obs_data_release(values);
		obs_data_array_t *portraits = obs_data_get_array(item, PORTRAITS_KEY);
		preset.portraits = portraits_from_array(portraits);
		if (portraits)
			obs_data_array_release(portraits);
		obs_data_release(item);
		// 名前の無いものは読み飛ばす
		if (!preset.name.empty())
			presets.push_back(std::move(preset));
	}
	if (array)
		obs_data_array_release(array);
	obs_data_release(root);
	return presets;
}

bool save_presets(const std::vector<Preset> &presets)
{
	char *dir = obs_module_config_path("");
	char *path = obs_module_config_path(FILE_NAME);
	if (!dir || !path) {
		bfree(dir);
		bfree(path);
		return false;
	}
	os_mkdirs(dir);
	bfree(dir);

	obs_data_t *root = obs_data_create();
	obs_data_array_t *array = obs_data_array_create();
	for (const auto &preset : presets) {
		obs_data_t *item = obs_data_create();
		obs_data_set_string(item, NAME_KEY, preset.name.c_str());
		obs_data_t *values = style_to_data(preset.values);
		obs_data_set_obj(item, VALUES_KEY, values);
		obs_data_release(values);
		obs_data_array_t *portraits = portraits_to_array(preset.portraits);
		obs_data_set_array(item, PORTRAITS_KEY, portraits);
		obs_data_array_release(portraits);
		obs_data_array_push_back(array, item);
		obs_data_release(item);
	}
	obs_data_set_array(root, PRESETS_KEY, array);
	obs_data_array_release(array);

	const bool saved = obs_data_save_json_safe(root, path, "tmp", "bak");
	if (!saved)
		obs_log(LOG_WARNING, "failed to save presets to %s", path);
	obs_data_release(root);
	bfree(path);
	return saved;
}

} // namespace sspot
