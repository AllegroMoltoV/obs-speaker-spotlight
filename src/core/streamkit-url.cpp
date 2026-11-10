/*
Voice Spotlight
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

#include "streamkit-url.h"

#include <algorithm>
#include <cctype>
#include <optional>
#include <string>
#include <vector>

namespace vspot::core {

namespace {

struct UrlParts {
	std::string host; // 小文字
	std::string path; // 先頭の / を含む。大文字と小文字を保つ
};

std::string to_lower(std::string_view text)
{
	std::string result(text);
	std::transform(result.begin(), result.end(), result.begin(),
		       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	return result;
}

std::string_view trim(std::string_view text)
{
	while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front())))
		text.remove_prefix(1);
	while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back())))
		text.remove_suffix(1);
	return text;
}

bool starts_with(std::string_view text, std::string_view prefix)
{
	return text.substr(0, prefix.size()) == prefix;
}

// スキームの無い "streamkit.discord.com/..." も URL として扱う
std::optional<UrlParts> split_url(std::string_view url)
{
	url = trim(url);
	const std::string lower = to_lower(url);
	size_t start = 0;
	if (starts_with(lower, "https://"))
		start = 8;
	else if (starts_with(lower, "http://"))
		start = 7;

	const std::string_view rest = url.substr(start);
	const size_t host_end = rest.find_first_of("/?#");
	UrlParts parts;
	parts.host = to_lower(rest.substr(0, host_end));
	if (parts.host.empty() || parts.host.find('.') == std::string::npos)
		return std::nullopt;
	if (host_end == std::string_view::npos)
		return parts;

	const std::string_view tail = rest.substr(host_end);
	parts.path = std::string(tail.substr(0, tail.find_first_of("?#")));
	return parts;
}

// パスを / で区切った要素。空の要素は除く
std::vector<std::string> path_segments(std::string_view path)
{
	std::vector<std::string> segments;
	size_t pos = 0;
	while (pos < path.size()) {
		const size_t end = path.find('/', pos);
		const std::string_view segment =
			path.substr(pos, end == std::string_view::npos ? std::string_view::npos : end - pos);
		if (!segment.empty())
			segments.emplace_back(segment);
		if (end == std::string_view::npos)
			break;
		pos = end + 1;
	}
	return segments;
}

bool is_digits(std::string_view text)
{
	return !text.empty() &&
	       std::all_of(text.begin(), text.end(), [](unsigned char c) { return std::isdigit(c) != 0; });
}

} // namespace

bool is_streamkit_voice_url(std::string_view url)
{
	const auto parts = split_url(url);
	if (!parts)
		return false;
	if (parts->host != "streamkit.discord.com" && parts->host != "streamkit.discordapp.com")
		return false;

	const auto segments = path_segments(to_lower(parts->path));
	return segments.size() == 4 && segments[0] == "overlay" && segments[1] == "voice" && is_digits(segments[2]) &&
	       is_digits(segments[3]);
}

} // namespace vspot::core
