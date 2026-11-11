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

#include "portrait.h"

#include <set>

namespace sspot::core {

namespace {

bool starts_with(std::string_view text, std::string_view prefix)
{
	return text.size() >= prefix.size() && text.compare(0, prefix.size(), prefix) == 0;
}

bool ends_with_ignoring_case(std::string_view text, std::string_view suffix)
{
	if (text.size() < suffix.size())
		return false;
	const std::string_view tail = text.substr(text.size() - suffix.size());
	for (size_t i = 0; i < suffix.size(); ++i) {
		char c = tail[i];
		if (c >= 'A' && c <= 'Z')
			c = static_cast<char>(c - 'A' + 'a');
		if (c != suffix[i])
			return false;
	}
	return true;
}

} // namespace

bool is_valid_user_id(std::string_view user_id)
{
	if (user_id.empty() || user_id.size() > MAX_USER_ID_DIGITS)
		return false;
	for (const char c : user_id)
		if (c < '0' || c > '9')
			return false;
	return true;
}

ImageFormat detect_image_format(std::string_view bytes)
{
	static constexpr std::string_view PNG_SIGNATURE("\x89PNG\r\n\x1a\n", 8);
	static constexpr std::string_view JPEG_SIGNATURE("\xff\xd8\xff", 3);
	if (starts_with(bytes, PNG_SIGNATURE))
		return ImageFormat::Png;
	if (starts_with(bytes, JPEG_SIGNATURE))
		return ImageFormat::Jpeg;
	return ImageFormat::Unknown;
}

ImageFormat format_from_file_name(std::string_view file_name)
{
	if (ends_with_ignoring_case(file_name, ".png"))
		return ImageFormat::Png;
	if (ends_with_ignoring_case(file_name, ".jpg") || ends_with_ignoring_case(file_name, ".jpeg"))
		return ImageFormat::Jpeg;
	return ImageFormat::Unknown;
}

std::string_view mime_type(ImageFormat format)
{
	switch (format) {
	case ImageFormat::Png:
		return "image/png";
	case ImageFormat::Jpeg:
		return "image/jpeg";
	case ImageFormat::Unknown:
		break;
	}
	return {};
}

ImageProblem check_image_size(std::size_t size)
{
	return size > MAX_IMAGE_BYTES ? ImageProblem::TooLarge : ImageProblem::None;
}

ImageProblem check_total_image_size(const std::vector<std::size_t> &registered_sizes, std::size_t added_size)
{
	if (added_size > MAX_TOTAL_IMAGE_BYTES)
		return ImageProblem::TotalTooLarge;
	std::size_t total = added_size;
	for (const std::size_t size : registered_sizes) {
		// 足し算の桁あふれを避けるため、残りの枠と比べる
		if (size > MAX_TOTAL_IMAGE_BYTES - total)
			return ImageProblem::TotalTooLarge;
		total += size;
	}
	return ImageProblem::None;
}

ImageProblem check_image(std::string_view file_name, std::string_view bytes)
{
	if (check_image_size(bytes.size()) != ImageProblem::None)
		return ImageProblem::TooLarge;
	const ImageFormat by_name = format_from_file_name(file_name);
	if (by_name == ImageFormat::Unknown)
		return ImageProblem::UnsupportedExtension;
	const ImageFormat by_content = detect_image_format(bytes);
	if (by_content == ImageFormat::Unknown)
		return ImageProblem::UnknownContent;
	return by_name == by_content ? ImageProblem::None : ImageProblem::ExtensionMismatch;
}

std::string base64_encode(std::string_view bytes)
{
	static constexpr char ALPHABET[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	const auto byte_at = [&](size_t index) {
		return static_cast<unsigned>(static_cast<unsigned char>(bytes[index]));
	};
	const auto sextet = [](unsigned group, unsigned shift) {
		return ALPHABET[(group >> shift) & 0x3fu];
	};
	std::string out;
	out.reserve((bytes.size() + 2) / 3 * 4);
	size_t i = 0;
	for (; i + 3 <= bytes.size(); i += 3) {
		const unsigned group = (byte_at(i) << 16) | (byte_at(i + 1) << 8) | byte_at(i + 2);
		out += sextet(group, 18);
		out += sextet(group, 12);
		out += sextet(group, 6);
		out += sextet(group, 0);
	}
	const size_t rest = bytes.size() - i;
	if (rest > 0) {
		const unsigned group = (byte_at(i) << 16) | (rest == 2 ? byte_at(i + 1) << 8 : 0u);
		out += sextet(group, 18);
		out += sextet(group, 12);
		out += rest == 2 ? sextet(group, 6) : '=';
		out += '=';
	}
	return out;
}

EntryCheck check_entries(const std::vector<PortraitEntry> &entries)
{
	std::set<std::string_view> seen;
	for (std::size_t i = 0; i < entries.size(); ++i) {
		const PortraitEntry &entry = entries[i];
		if (entry.user_id.empty() && entry.image_path.empty())
			continue;
		if (entry.user_id.empty())
			return {EntryProblem::MissingUserId, i};
		if (!is_valid_user_id(entry.user_id))
			return {EntryProblem::InvalidUserId, i};
		if (!seen.insert(entry.user_id).second)
			return {EntryProblem::DuplicateUserId, i};
		if (entry.image_path.empty())
			return {EntryProblem::MissingImage, i};
	}
	return {};
}

} // namespace sspot::core
