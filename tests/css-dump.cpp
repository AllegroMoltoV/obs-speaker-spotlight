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

// 生成した CSS を、ローカルのページに注入して表示を確かめるためにファイルへ書き出す。
// 使い方: speaker-spotlight-css-dump <出力先のフォルダー> [ユーザー ID]
// default.css (既定値)、max.css (数値を最大、緑の枠なし)、min.css (数値を最小) にはテスト画像の立ち絵を入れ、
// avatar.css (既定値) には入れない。
// ユーザー ID を省くと 100000000000000001 にする

#include "core/portrait.h"
#include "core/voice-style.h"

#include <fstream>
#include <iostream>
#include <optional>
#include <string>

using namespace sspot::core;

namespace {

// 60 x 120 の png。上半分が橙、下半分が緑
const unsigned char IMAGE_A[] = {
	0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, 0x44, 0x52, 0x00, 0x00,
	0x00, 0x3c, 0x00, 0x00, 0x00, 0x78, 0x08, 0x02, 0x00, 0x00, 0x00, 0x21, 0x57, 0x06, 0x5f, 0x00, 0x00, 0x00,
	0x7a, 0x49, 0x44, 0x41, 0x54, 0x78, 0xda, 0xed, 0xce, 0x01, 0x09, 0x00, 0x30, 0x08, 0x00, 0x30, 0xe3, 0x98,
	0xe9, 0x49, 0x8c, 0x63, 0x3a, 0x73, 0xbc, 0x85, 0x20, 0x0c, 0x16, 0x60, 0x31, 0x95, 0xe7, 0x84, 0xb4, 0xb4,
	0xb4, 0xb4, 0xb4, 0xb4, 0xb4, 0xb4, 0xb4, 0xb4, 0xb4, 0xb4, 0xb4, 0xb4, 0xb4, 0xb4, 0xb4, 0xb4, 0xb4, 0xb4,
	0xb4, 0xb4, 0xb4, 0xb4, 0xb4, 0xb4, 0xb4, 0xb4, 0xb4, 0xb4, 0xb4, 0xb4, 0xb4, 0xb4, 0xb4, 0xb4, 0xb4, 0xb4,
	0xb4, 0xb4, 0xf4, 0x4a, 0x3a, 0xfb, 0x9d, 0x23, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d,
	0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d,
	0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0xbd, 0xe2, 0x03, 0xc8, 0x2e, 0x5c,
	0xac, 0xfb, 0x6b, 0x53, 0x56, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82};

std::string bytes_of(const unsigned char *data, size_t size)
{
	return std::string(reinterpret_cast<const char *>(data), size);
}

StyleValues extreme_values(bool use_max)
{
	StyleValues values;
	for (const auto &section : voice_sections())
		for (const auto &item : section.items)
			if (item.kind == ItemKind::Number)
				values.numbers[std::string(item.key)] = use_max ? item.max : item.min;
	if (use_max)
		values.numbers["speaking_border_visible"] = 0;
	return values;
}

bool write_file(const std::string &path, const std::string &text)
{
	std::ofstream out(path, std::ios::binary);
	out << text;
	return static_cast<bool>(out);
}

} // namespace

int main(int argc, char **argv)
{
	if (argc < 2) {
		std::cerr << "usage: speaker-spotlight-css-dump <output-dir> [user-id]\n";
		return 2;
	}
	const std::string dir = argv[1];
	const std::string user_id = argc > 2 ? argv[2] : "100000000000000001";
	if (!is_valid_user_id(user_id)) {
		std::cerr << "the user ID must be 1 to 20 digits\n";
		return 2;
	}

	const std::string image = bytes_of(IMAGE_A, sizeof(IMAGE_A));
	const PortraitImage portrait{detect_image_format(image), base64_encode(image), 60, 120};

	struct Profile {
		const char *name;
		StyleValues values;
		std::optional<PortraitImage> portrait;
	};
	const Profile profiles[] = {
		{"default.css", StyleValues{}, portrait},
		{"max.css", extreme_values(true), portrait},
		{"min.css", extreme_values(false), portrait},
		{"avatar.css", StyleValues{}, std::nullopt},
	};
	for (const auto &profile : profiles) {
		const std::string path = dir + "/" + profile.name;
		if (!write_file(path, generate_css(profile.values, user_id, profile.portrait))) {
			std::cerr << "failed to write " << path << '\n';
			return 1;
		}
		std::cout << path << '\n';
	}
	return 0;
}
