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

#include "voice-style.h"

#include "css-builder.h"
#include "streamkit-dom.h"

#include <algorithm>
#include <set>

namespace vspot::core {

namespace {

namespace sk = streamkit;

bool is_base64_char(char c)
{
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '+' || c == '/' ||
	       c == '=';
}

bool is_base64_text(std::string_view text)
{
	return !text.empty() && std::all_of(text.begin(), text.end(), is_base64_char);
}

// CSS に出してよい立ち絵だけを、表示の順に並べて返す
std::vector<const Portrait *> usable_portraits(const std::vector<Portrait> &portraits)
{
	std::vector<const Portrait *> result;
	std::set<std::string, std::less<>> seen;
	for (const auto &portrait : portraits) {
		if (!is_valid_user_id(portrait.user_id) || mime_type(portrait.format).empty() ||
		    !is_base64_text(portrait.base64) || !seen.insert(portrait.user_id).second)
			continue;
		result.push_back(&portrait);
	}
	std::stable_sort(result.begin(), result.end(),
			 [](const Portrait *a, const Portrait *b) { return a->order < b->order; });
	return result;
}

std::string group_digits(size_t value)
{
	std::string digits = std::to_string(value);
	for (size_t pos = digits.size(); pos > 3; pos -= 3)
		digits.insert(pos - 3, ",");
	return digits;
}

} // namespace

std::string generate_css(const StyleValues &values, const std::vector<Portrait> &portraits)
{
	const css::Reader reader(voice_sections(), values);
	css::CssWriter css;

	const std::string item(sk::ITEM);
	const std::string avatar = css::under(item, sk::AVATAR);
	const std::string speaking_avatar = css::under(item + std::string(sk::SPEAKING), sk::AVATAR);
	const std::string user = css::under(item, sk::USER);
	const std::string name = css::under(user, sk::NAME);

	const int height = reader.number("avatar_height").value_or(150);
	const bool horizontal = reader.text("layout_direction") == "horizontal";
	const std::vector<const Portrait *> registered = usable_portraits(portraits);

	css.add(std::string(sk::BODY), "background-color", "rgba(0, 0, 0, 0)");

	// 表示の順を order で指定できるよう、一覧を flex にする。StreamKit の li は高さが 50px に固定され、
	// img は float で並ぶため、立ち絵の高さに合わせて li も flex にする
	const std::string list(sk::LIST);
	css.add(list, "display", "flex");
	css.add(list, "flex-direction", horizontal ? "row" : "column");
	css.add(list, "align-items", horizontal ? "flex-end" : "flex-start");
	css.add(list, "gap", css::px(reader.number("item_gap").value_or(0)));

	css.add(item, "display", "flex");
	css.add(item, "flex-direction", horizontal ? "column" : "row");
	css.add(item, "align-items", "center");
	css.add(item, "gap", "8px");
	css.add(item, "height", "auto");
	css.add(item, "margin", "0");
	// 登録していない参加者は、登録した参加者の後ろに並べる
	css.add(item, "order", std::to_string(registered.size() + 1));

	css.add(avatar, "float", "none");
	css.add(avatar, "flex-shrink", "0");
	css.add(avatar, "margin", "0");
	css.add(avatar, "width", css::px(height));
	css.add(avatar, "height", css::px(height));
	css.add(avatar, "filter",
		"brightness(" + css::ratio_text(reader.number("idle_brightness").value_or(100)) + ")");
	css.add(avatar, "transition", "filter " + std::to_string(reader.number("transition_ms").value_or(0)) + "ms");

	css.add(speaking_avatar, "filter",
		"brightness(" + css::ratio_text(reader.number("speaking_brightness").value_or(100)) + ")");
	if (!reader.toggle("speaking_border_visible"))
		css.add(speaking_avatar, "border-color", "transparent");

	css.add(user, "padding-top", "0");
	if (!reader.toggle("name_visible")) {
		css.add(user, "display", "none");
	} else {
		// StreamKit のコンテナの line-height: 19px が残るため、インラインの span では大きな文字の背景が
		// 行の外へはみ出し、立ち絵や隣の参加者に重なる。span 自身の高さで行を取らせる
		css.add(name, "display", "inline-block");
		css.add(name, "line-height", "normal");
		const std::string font = reader.text("name_font");
		if (!font.empty())
			css.add(name, "font-family", css_string(font));
		if (const auto size = reader.number("name_size"))
			css.add(name, "font-size", css::px(*size));
		if (const auto color = reader.color("name_color"))
			css.add(name, "color", css::rgb_text(*color));
		if (const auto background = reader.color("name_background"))
			css.add(name, "background-color", css::rgb_text(*background));
	}

	for (size_t i = 0; i < registered.size(); ++i) {
		const Portrait &portrait = *registered[i];
		const std::string target = sk::item_of_user(portrait.user_id);
		const std::string image = css::under(target, sk::AVATAR);
		css.add(target, "order", std::to_string(i + 1));
		css.add(image, "content",
			"url(\"data:" + std::string(mime_type(portrait.format)) + ";base64," + portrait.base64 + "\")");
		// 立ち絵の縦横比を保つため、幅は高さから決める
		css.add(image, "width", "auto");
		css.add(image, "height", css::px(height));
		css.add(image, "border", "0");
		css.add(image, "border-radius", "0");
	}

	return css.str();
}

std::string abbreviate_data_uris(std::string_view css, std::string_view count_format)
{
	static constexpr std::string_view DATA = "data:";
	static constexpr std::string_view BASE64 = ";base64,";
	std::string out;
	size_t pos = 0;
	while (true) {
		const size_t start = css.find(DATA, pos);
		if (start == std::string_view::npos)
			break;
		const size_t marker = css.find(BASE64, start);
		// MIME タイプに使わない文字を挟む ;base64, は、この data URI のものではない
		const std::string_view mime = css.substr(
			start + DATA.size(), marker == std::string_view::npos ? 0 : marker - start - DATA.size());
		const bool valid = marker != std::string_view::npos &&
				   std::all_of(mime.begin(), mime.end(), [](char c) {
					   return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '/' ||
						  c == '+' || c == '-' || c == '.';
				   });
		if (!valid) {
			out.append(css.substr(pos, start + DATA.size() - pos));
			pos = start + DATA.size();
			continue;
		}
		const size_t body = marker + BASE64.size();
		size_t end = body;
		while (end < css.size() && is_base64_char(css[end]))
			++end;
		out.append(css.substr(pos, body - pos));
		std::string label(count_format);
		const size_t slot = label.find("%1");
		if (slot != std::string::npos)
			label.replace(slot, 2, group_digits(end - body));
		out += label;
		pos = end;
	}
	out.append(css.substr(pos));
	return out;
}

} // namespace vspot::core
