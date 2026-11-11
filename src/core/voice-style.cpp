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

#include "voice-style.h"

#include "css-builder.h"
#include "managed-css.h"
#include "streamkit-dom.h"

#include <algorithm>
#include <utility>

namespace sspot::core {

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

} // namespace

int avatar_height(const StyleValues &values)
{
	return css::Reader(voice_sections(), values).number("avatar_height").value_or(150);
}

std::string generate_css(const StyleValues &values, std::string_view user_id,
			 const std::optional<PortraitImage> &portrait)
{
	const css::Reader reader(voice_sections(), values);
	css::CssWriter css;

	const std::string list(sk::LIST);
	const std::string item(sk::ITEM);
	const std::string target = sk::item_of_user(user_id);
	const std::string avatar = css::under(item, sk::AVATAR);
	const std::string speaking_avatar = css::under(item + std::string(sk::SPEAKING), sk::AVATAR);
	const std::string user = css::under(item, sk::USER);
	const std::string name = css::under(user, sk::NAME);

	const int height = avatar_height(values);

	css.add(std::string(sk::BODY), "background-color", "rgba(0, 0, 0, 0)");

	// 1 人だけを残し、一覧の余白を除いて左上に置く。配置と重なり順は OBS のシーンで決める
	css.add(list, "margin", "0");
	css.add(list, "padding", "0");
	css.add(item, "display", "none");
	// StreamKit の li は高さが 50px に固定され、img は float で並ぶため、立ち絵の高さに合わせて li を flex にする
	css.add(target, "display", "flex");
	css.add(target, "flex-direction", "row");
	css.add(target, "align-items", "center");
	css.add(target, "gap", "8px");
	css.add(target, "height", "auto");
	css.add(target, "margin", "0");

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
		// 行の外へはみ出し、立ち絵に重なる。span 自身の高さで行を取らせる
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

	if (portrait && !mime_type(portrait->format).empty() && is_base64_text(portrait->base64)) {
		const std::string image = css::under(target, sk::AVATAR);
		css.add(image, "content",
			"url(\"data:" + std::string(mime_type(portrait->format)) + ";base64," + portrait->base64 +
				"\")");
		// 立ち絵の縦横比を保つため、幅は高さから決める
		css.add(image, "width", "auto");
		css.add(image, "height", css::px(height));
		css.add(image, "border", "0");
		css.add(image, "border-radius", "0");
	}

	return css.str();
}

CssUpdate update_css(std::string_view current_css, const StyleValues &values, std::string_view user_id,
		     const std::optional<PortraitImage> &portrait)
{
	std::optional<std::string> updated;
	if (user_id.empty()) {
		updated = remove_block(current_css);
	} else {
		if (!is_valid_user_id(user_id))
			return {CssUpdateProblem::InvalidUserId, {}};
		updated = replace_block(current_css, generate_css(values, user_id, portrait));
	}
	if (!updated)
		return {CssUpdateProblem::BrokenMarkers, {}};
	return {CssUpdateProblem::None, std::move(*updated)};
}

} // namespace sspot::core
