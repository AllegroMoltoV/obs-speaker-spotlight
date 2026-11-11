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

VoiceLook resolve_look(const StyleValues &values)
{
	const css::Reader reader(voice_sections(), values);
	VoiceLook look;
	look.avatar_height = reader.number("avatar_height").value_or(150);
	look.idle_brightness = reader.number("idle_brightness").value_or(100);
	look.speaking_brightness = reader.number("speaking_brightness").value_or(100);
	look.transition_ms = reader.number("transition_ms").value_or(0);
	look.speaking_border_visible = reader.toggle("speaking_border_visible");
	look.name_visible = reader.toggle("name_visible");
	look.name_font = reader.text("name_font");
	look.name_size = reader.number("name_size");
	look.name_color = reader.color("name_color");
	look.name_background = reader.color("name_background");
	return look;
}

bool portrait_usable(const PortraitImage &portrait)
{
	return !mime_type(portrait.format).empty() && is_base64_text(portrait.base64);
}

std::uint8_t brighten(std::uint8_t channel, int percent)
{
	const int value = (channel * std::max(percent, 0) + 50) / 100;
	return static_cast<std::uint8_t>(std::min(value, 255));
}

void brighten_pixels(std::uint32_t *pixels, std::size_t count, int percent)
{
	for (std::size_t i = 0; i < count; ++i) {
		const std::uint32_t p = pixels[i];
		const std::uint32_t r = brighten(static_cast<std::uint8_t>(p >> 16), percent);
		const std::uint32_t g = brighten(static_cast<std::uint8_t>(p >> 8), percent);
		const std::uint32_t b = brighten(static_cast<std::uint8_t>(p), percent);
		pixels[i] = (p & 0xff000000u) | (r << 16) | (g << 8) | b;
	}
}

std::string generate_css(const StyleValues &values, std::string_view user_id,
			 const std::optional<PortraitImage> &portrait)
{
	const VoiceLook look = resolve_look(values);
	css::CssWriter css;

	const std::string list(sk::LIST);
	const std::string item(sk::ITEM);
	const std::string target = sk::item_of_user(user_id);
	const std::string avatar = css::under(item, sk::AVATAR);
	const std::string speaking_avatar = css::under(item + std::string(sk::SPEAKING), sk::AVATAR);
	const std::string user = css::under(item, sk::USER);
	const std::string name = css::under(user, sk::NAME);

	const int height = look.avatar_height;

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
	css.add(avatar, "filter", "brightness(" + css::ratio_text(look.idle_brightness) + ")");
	css.add(avatar, "transition", "filter " + std::to_string(look.transition_ms) + "ms");

	css.add(speaking_avatar, "filter", "brightness(" + css::ratio_text(look.speaking_brightness) + ")");
	if (!look.speaking_border_visible)
		css.add(speaking_avatar, "border-color", "transparent");

	css.add(user, "padding-top", "0");
	if (!look.name_visible) {
		css.add(user, "display", "none");
	} else {
		// StreamKit のコンテナの line-height: 19px が残るため、インラインの span では大きな文字の背景が
		// 行の外へはみ出し、立ち絵に重なる。span 自身の高さで行を取らせる
		css.add(name, "display", "inline-block");
		css.add(name, "line-height", "normal");
		if (!look.name_font.empty())
			css.add(name, "font-family", css_string(look.name_font));
		if (look.name_size)
			css.add(name, "font-size", css::px(*look.name_size));
		if (look.name_color)
			css.add(name, "color", css::rgb_text(*look.name_color));
		if (look.name_background)
			css.add(name, "background-color", css::rgb_text(*look.name_background));
	}

	if (portrait && portrait_usable(*portrait)) {
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
	const bool changed = *updated != current_css;
	return {CssUpdateProblem::None, std::move(*updated), changed};
}

} // namespace sspot::core
