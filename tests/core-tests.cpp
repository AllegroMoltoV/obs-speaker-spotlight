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

// OBS に依存しない処理の単体テスト。失敗した検査を標準エラーに書き、終了コード 1 で終わる

#include "core/css-builder.h"
#include "core/managed-css.h"
#include "core/portrait.h"
#include "core/streamkit-url.h"
#include "core/style-schema.h"
#include "core/voice-style.h"

#include <iostream>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace {

int failures = 0;

void check(bool condition, const char *what, int line)
{
	if (condition)
		return;
	++failures;
	std::cerr << "FAILED (line " << line << "): " << what << '\n';
}

#define CHECK(expr) check((expr), #expr, __LINE__)

using namespace sspot::core;

const std::string DEFAULT_CSS = "body { background-color: rgba(0, 0, 0, 0); margin: 0px auto; overflow: hidden; }";

size_t count(const std::string &text, const std::string &needle)
{
	size_t n = 0;
	for (size_t pos = text.find(needle); pos != std::string::npos; pos = text.find(needle, pos + 1))
		++n;
	return n;
}

void test_markers_name_this_plugin()
{
	CHECK(BLOCK_BEGIN == "/* speaker-spotlight begin */");
	CHECK(BLOCK_END == "/* speaker-spotlight end */");
}

void test_replace_appends_when_absent()
{
	const auto result = replace_block(DEFAULT_CSS, ".a { color: red; }");
	CHECK(result.has_value());
	CHECK(result->rfind(DEFAULT_CSS + "\n", 0) == 0);
	CHECK(result->find(".a { color: red; }") != std::string::npos);
	CHECK(inspect_block(*result) == BlockState::Intact);
}

void test_replace_on_empty_css()
{
	const auto result = replace_block("", ".a { color: red; }");
	CHECK(result.has_value());
	CHECK(result->rfind(std::string(BLOCK_BEGIN), 0) == 0);
}

void test_replace_keeps_user_css_outside_block()
{
	const std::string before = "/* mine */\n.user { color: blue; }\n";
	const std::string after = "\n.user2 { color: green; }\n";
	const std::string css =
		before + std::string(BLOCK_BEGIN) + "\n.old { color: red; }\n" + std::string(BLOCK_END) + after;

	const auto result = replace_block(css, ".new { color: white; }");
	CHECK(result.has_value());
	CHECK(result->rfind(before, 0) == 0);
	CHECK(result->size() >= after.size() &&
	      result->compare(result->size() - after.size(), after.size(), after) == 0);
	CHECK(result->find(".old") == std::string::npos);
	CHECK(result->find(".new { color: white; }") != std::string::npos);
}

void test_replace_twice_keeps_single_block()
{
	const auto first = replace_block(DEFAULT_CSS, ".a { color: red; }");
	const auto second = replace_block(*first, ".b { color: red; }");
	CHECK(second.has_value());
	CHECK(count(*second, std::string(BLOCK_BEGIN)) == 1);
	CHECK(count(*second, std::string(BLOCK_END)) == 1);
	CHECK(second->find(".a {") == std::string::npos);
	CHECK(second->rfind(DEFAULT_CSS, 0) == 0);
}

void test_replace_refuses_broken_markers()
{
	const std::string begin(BLOCK_BEGIN);
	const std::string end(BLOCK_END);
	const std::string cases[] = {
		DEFAULT_CSS + "\n" + begin + "\n.x {}\n",
		DEFAULT_CSS + "\n.x {}\n" + end + "\n",
		DEFAULT_CSS + "\n" + end + "\n.x {}\n" + begin + "\n",
		DEFAULT_CSS + "\n" + begin + "\n" + end + "\n" + begin + "\n" + end + "\n",
	};
	for (const auto &css : cases) {
		CHECK(inspect_block(css) == BlockState::Broken);
		CHECK(!replace_block(css, ".y {}").has_value());
	}
}

void test_replace_ignores_other_plugin_markers()
{
	// 兄弟のプラグインの目印は利用者の CSS として扱い、残す
	const std::string other = "/* comment-skin begin */\n.c {}\n/* comment-skin end */";
	const auto result = replace_block(DEFAULT_CSS + "\n" + other, ".v {}");
	CHECK(result.has_value());
	CHECK(result->find(other) != std::string::npos);
	CHECK(count(*result, std::string(BLOCK_BEGIN)) == 1);
}

void test_undo_decision()
{
	CHECK(decide_undo(false, "a", "a") == UndoDecision::Nothing);
	CHECK(decide_undo(true, "applied", "applied") == UndoDecision::Restore);
	CHECK(decide_undo(true, "applied + edit", "applied") == UndoDecision::RestoreWithConfirm);
}

void test_streamkit_voice_url()
{
	CHECK(is_streamkit_voice_url(
		"https://streamkit.discord.com/overlay/voice/123456789012345678/876543210987654321"));
	CHECK(is_streamkit_voice_url("https://streamkit.discord.com/overlay/voice/0/0?icon=true&online=true"));
	CHECK(is_streamkit_voice_url("  https://STREAMKIT.DISCORD.COM/overlay/voice/1/2/  "));
	CHECK(is_streamkit_voice_url("https://streamkit.discordapp.com/overlay/voice/1/2"));
	CHECK(is_streamkit_voice_url("streamkit.discord.com/overlay/voice/1/2"));
	CHECK(!is_streamkit_voice_url("https://streamkit.discord.com/overlay/chat/1/2"));
	CHECK(!is_streamkit_voice_url("https://streamkit.discord.com/overlay/voice/1"));
	CHECK(!is_streamkit_voice_url("https://streamkit.discord.com/overlay/voice/a/2"));
	CHECK(!is_streamkit_voice_url("https://example.com/overlay/voice/1/2"));
	CHECK(!is_streamkit_voice_url("https://streamkit.discord.com.example.com/overlay/voice/1/2"));
	CHECK(!is_streamkit_voice_url(""));
}

void test_hex_color()
{
	CHECK((parse_hex_color("#1a2B3c") == Rgb{0x1a, 0x2b, 0x3c}));
	CHECK((to_hex_color(Rgb{0x1a, 0x2b, 0x3c}) == "#1a2b3c"));
	CHECK(!parse_hex_color("1a2b3c").has_value());
	CHECK(!parse_hex_color("#1a2b3").has_value());
	CHECK(!parse_hex_color("#1a2b3g").has_value());
	CHECK(!parse_hex_color("red").has_value());
	CHECK(!parse_hex_color("#000000; } @import url(x) {").has_value());
}

void test_sanitize()
{
	// 項目の表の型と範囲で値を捨てることを、テストの中の表で確かめる
	ItemDef shape{"shape", ItemKind::Choice, "Item.Shape", 0, 0, std::nullopt, {}, {}, {}};
	shape.choices = {"circle", "square"};
	const std::vector<SectionDef> sections = {
		{"Section.Test",
		 {
			 ItemDef{"size", ItemKind::Number, "Item.Size", 8, 96, std::nullopt, "px", {}, {}},
			 shape,
			 ItemDef{"font", ItemKind::Font, "Item.Font", 0, 0, std::nullopt, {}, {}, {}},
		 }},
	};
	StyleValues values;
	values.numbers["size"] = 1;
	values.numbers["unknown_key"] = 5;
	values.texts["shape"] = "triangle";
	values.texts["font"] = "  Yu Gothic; } ";
	values.colors["size"] = Rgb{1, 2, 3};
	const StyleValues clean = sanitize(sections, values);
	CHECK(clean.numbers.at("size") == 8);
	CHECK(clean.numbers.count("unknown_key") == 0);
	CHECK(clean.texts.count("shape") == 0);
	CHECK(clean.texts.at("font") == "Yu Gothic");
	CHECK(clean.colors.empty());
}

void test_sanitize_font_name()
{
	CHECK(sanitize_font_name("Yu Gothic UI") == "Yu Gothic UI");
	CHECK(sanitize_font_name("  游ゴシック  ") == "游ゴシック");
	CHECK(sanitize_font_name("a\"; } @import url(x) {") == "a  import urlx");
	CHECK(sanitize_font_name("();{}").empty());
}

void test_css_string()
{
	CHECK(css_string("Yu Gothic") == "\"Yu Gothic\"");
	CHECK(css_string("a\"b\\c\nd") == "\"a\\\"b\\\\cd\"");
}

const std::string PNG_BYTES = std::string("\x89PNG\r\n\x1a\n", 8) + "rest-of-png";
const std::string JPEG_BYTES = std::string("\xff\xd8\xff\xe0", 4) + "rest-of-jpeg";

void test_user_id()
{
	CHECK(is_valid_user_id("1"));
	CHECK(is_valid_user_id("123456789012345678"));
	CHECK(is_valid_user_id("18446744073709551615"));
	CHECK(!is_valid_user_id("184467440737095516150"));
	CHECK(!is_valid_user_id(""));
	CHECK(!is_valid_user_id("1\" ] body{"));
	CHECK(!is_valid_user_id("123\"] , body { display: none } [x=\""));
	CHECK(!is_valid_user_id(" 123"));
	CHECK(!is_valid_user_id("12a"));
	CHECK(!is_valid_user_id("\xEF\xBC\x91\xEF\xBC\x92")); // 全角の「１２」
}

void test_image_format()
{
	CHECK(check_image("a.png", PNG_BYTES) == ImageProblem::None);
	CHECK(check_image("C:/pics/A.PNG", PNG_BYTES) == ImageProblem::None);
	CHECK(check_image("a.jpg", JPEG_BYTES) == ImageProblem::None);
	CHECK(check_image("a.JpEg", JPEG_BYTES) == ImageProblem::None);
	CHECK(check_image("a.jpg", PNG_BYTES) == ImageProblem::ExtensionMismatch);
	CHECK(check_image("a.png", JPEG_BYTES) == ImageProblem::ExtensionMismatch);
	CHECK(check_image("a.png", "GIF89a....") == ImageProblem::UnknownContent);
	CHECK(check_image("a.png", std::string("\x89PNG", 4)) == ImageProblem::UnknownContent);
	CHECK(check_image("a.png", "") == ImageProblem::UnknownContent);
	CHECK(check_image("a.gif", PNG_BYTES) == ImageProblem::UnsupportedExtension);
	CHECK(check_image("a.png.txt", PNG_BYTES) == ImageProblem::UnsupportedExtension);
	CHECK(check_image("png", PNG_BYTES) == ImageProblem::UnsupportedExtension);
	CHECK(mime_type(detect_image_format(PNG_BYTES)) == "image/png");
	CHECK(mime_type(detect_image_format(JPEG_BYTES)) == "image/jpeg");
}

void test_image_size_limits()
{
	std::string at_limit = PNG_BYTES;
	at_limit.resize(MAX_IMAGE_BYTES, '\0');
	CHECK(check_image("a.png", at_limit) == ImageProblem::None);
	CHECK(check_image("a.png", at_limit + '\0') == ImageProblem::TooLarge);
	CHECK(check_image_size(MAX_IMAGE_BYTES) == ImageProblem::None);
	CHECK(check_image_size(MAX_IMAGE_BYTES + 1) == ImageProblem::TooLarge);

	const std::vector<std::size_t> three = {MAX_IMAGE_BYTES, MAX_IMAGE_BYTES, MAX_IMAGE_BYTES};
	CHECK(check_total_image_size(three, MAX_IMAGE_BYTES) == ImageProblem::None);
	CHECK(check_total_image_size(three, MAX_IMAGE_BYTES + 1) == ImageProblem::TotalTooLarge);
	const std::vector<std::size_t> four = {MAX_IMAGE_BYTES, MAX_IMAGE_BYTES, MAX_IMAGE_BYTES, MAX_IMAGE_BYTES};
	CHECK(check_total_image_size(four, 1) == ImageProblem::TotalTooLarge);
	CHECK(check_total_image_size({}, 0) == ImageProblem::None);
	CHECK(check_total_image_size({static_cast<std::size_t>(-1)}, 1) == ImageProblem::TotalTooLarge);
}

void test_base64()
{
	// RFC 4648 の 10 節の値
	CHECK(base64_encode("") == "");
	CHECK(base64_encode("f") == "Zg==");
	CHECK(base64_encode("fo") == "Zm8=");
	CHECK(base64_encode("foo") == "Zm9v");
	CHECK(base64_encode("foob") == "Zm9vYg==");
	CHECK(base64_encode("fooba") == "Zm9vYmE=");
	CHECK(base64_encode("foobar") == "Zm9vYmFy");
	CHECK(base64_encode(std::string("\xff\xfe\x00", 3)) == "//4A");
}

void test_voice_sections_table()
{
	// 既定値が範囲の外にあると、sanitize() が保存した値を既定と違う値に変える
	std::set<std::string> keys;
	for (const auto &section : voice_sections()) {
		for (const auto &item : section.items) {
			CHECK(keys.insert(std::string(item.key)).second);
			if (item.default_value)
				CHECK(*item.default_value >= item.min && *item.default_value <= item.max);
			for (const auto &condition : item.enabled_when)
				CHECK(find_item(voice_sections(), condition.key) != nullptr);
		}
	}
	CHECK(!keys.empty());
}

Portrait portrait_of(const std::string &user_id, const std::string &bytes, int order)
{
	return Portrait{user_id, detect_image_format(bytes), base64_encode(bytes), order, "memo-" + user_id};
}

StyleValues max_values()
{
	StyleValues values;
	for (const auto &section : voice_sections())
		for (const auto &item : section.items)
			if (item.kind == ItemKind::Number || item.kind == ItemKind::Toggle)
				values.numbers[std::string(item.key)] = item.max;
	values.texts["name_font"] = "x\"); } @import url(http://example.com/x.css); a {";
	values.texts["layout_direction"] = "horizontal";
	values.colors["name_color"] = Rgb{1, 2, 3};
	values.colors["name_background"] = Rgb{4, 5, 6};
	return values;
}

void test_css_has_no_external_resources()
{
	const std::vector<Portrait> portraits = {
		portrait_of("111", PNG_BYTES, 0),
		portrait_of("222", JPEG_BYTES, 0),
		// 不正な ID と、base64 でない本文は CSS に出ない
		portrait_of("1\" ] body{", PNG_BYTES, 0),
		Portrait{"333", ImageFormat::Png, "abc\") } @import url(http://example.com/y.css", 0, ""},
	};
	for (const StyleValues &values : {StyleValues{}, max_values()}) {
		const std::string css = generate_css(values, portraits);
		CHECK(css.find("@import") == std::string::npos);
		CHECK(css.find("body{") == std::string::npos);
		size_t urls = 0;
		for (size_t pos = css.find("url("); pos != std::string::npos; pos = css.find("url(", pos + 1)) {
			++urls;
			CHECK(css.compare(pos, 16, "url(\"data:image/") == 0);
		}
		CHECK(urls == 2);
	}
}

// 宣言 property を含む規則のセレクターをすべて返す
std::vector<std::string> selectors_with(const std::string &css, const std::string &property)
{
	std::vector<std::string> result;
	for (size_t pos = css.find("\n  " + property + ":"); pos != std::string::npos;
	     pos = css.find("\n  " + property + ":", pos + 1)) {
		const size_t open = css.rfind(" {\n", pos);
		const size_t close = css.rfind("}\n", open);
		const size_t start = close == std::string::npos ? 0 : close + 2;
		result.push_back(css.substr(start, open - start));
	}
	return result;
}

void test_css_portraits_target_registered_users_only()
{
	const std::vector<Portrait> portraits = {
		portrait_of("222", JPEG_BYTES, 5),
		portrait_of("111", PNG_BYTES, 1),
		// 同じ ID の 2 つ目は使わない
		portrait_of("111", JPEG_BYTES, 0),
	};
	const std::string css = generate_css(StyleValues{}, portraits);

	const std::vector<std::string> expected = {".voice_state[data-userid=\"111\"] .voice_avatar",
						   ".voice_state[data-userid=\"222\"] .voice_avatar"};
	CHECK(selectors_with(css, "content") == expected);
	CHECK(css.find("url(\"data:image/png;base64," + base64_encode(PNG_BYTES) + "\")") != std::string::npos);
	CHECK(css.find("url(\"data:image/jpeg;base64," + base64_encode(JPEG_BYTES) + "\")") != std::string::npos);
	// 明るさの切り替えは、登録の有無にかかわらず全員の img に掛かる
	CHECK(selectors_with(css, "filter") ==
	      (std::vector<std::string>{".voice_state .voice_avatar", ".voice_state.wrapper_speaking .voice_avatar"}));
	// 表示の順は order の小さい順に 1 から振り、登録していない参加者を最後に置く
	CHECK(css.find(".voice_state[data-userid=\"111\"] {\n  order: 1 !important;") != std::string::npos);
	CHECK(css.find(".voice_state[data-userid=\"222\"] {\n  order: 2 !important;") != std::string::npos);
	CHECK(css.find("order: 3 !important;") != std::string::npos);

	const std::string without = generate_css(StyleValues{}, {});
	CHECK(selectors_with(without, "content").empty());
	CHECK(without.find("data-userid") == std::string::npos);
}

void test_generated_css_in_managed_block()
{
	const std::string before = DEFAULT_CSS + "\n/* mine */\n.voice_state { outline: 1px solid red; }\n";
	const std::string after = "\n.mine-after { color: blue; }\n";
	const auto first = replace_block(before + std::string(BLOCK_BEGIN) + "\n" + std::string(BLOCK_END) + after,
					 generate_css(StyleValues{}, {portrait_of("111", PNG_BYTES, 0)}));
	CHECK(first.has_value());
	const auto second = replace_block(*first, generate_css(max_values(), {portrait_of("222", JPEG_BYTES, 0)}));
	CHECK(second.has_value());
	CHECK(second->rfind(before, 0) == 0);
	CHECK(second->size() >= after.size() &&
	      second->compare(second->size() - after.size(), after.size(), after) == 0);
	CHECK(second->find("data-userid=\"111\"") == std::string::npos);
	CHECK(second->find("data-userid=\"222\"") != std::string::npos);
	CHECK(inspect_block(*second) == BlockState::Intact);
}

// 名前の背景が立ち絵や隣の参加者に重ならないよう、表示するときは span に行の高さを取らせる
void test_name_takes_its_own_line_height()
{
	for (const StyleValues &values : {StyleValues{}, max_values()}) {
		const std::string css = generate_css(values, {});
		CHECK(css.find("  display: inline-block !important;\n") != std::string::npos);
		CHECK(css.find("  line-height: normal !important;\n") != std::string::npos);
	}
	StyleValues hidden;
	hidden.numbers["name_visible"] = 0;
	const std::string css = generate_css(hidden, {});
	CHECK(css.find("inline-block") == std::string::npos);
}

void test_abbreviate_data_uris()
{
	const std::string image = PNG_BYTES + std::string(3000, 'x');
	const std::string css = generate_css(StyleValues{}, {portrait_of("111", image, 0)});
	const std::string shown = abbreviate_data_uris(css, "...(%1)");
	const std::string encoded = base64_encode(image);
	CHECK(shown.find(encoded) == std::string::npos);
	const std::string digits = std::to_string(encoded.size());
	const std::string grouped = digits.substr(0, digits.size() - 3) + "," + digits.substr(digits.size() - 3);
	CHECK(shown.find("url(\"data:image/png;base64,...(" + grouped + ")\")") != std::string::npos);
	// data URI の外は変えない
	CHECK(abbreviate_data_uris(generate_css(StyleValues{}, {})) == generate_css(StyleValues{}, {}));
	CHECK(abbreviate_data_uris("a data: b;base64,QUJD") == "a data: b;base64,QUJD");
}

void test_check_entries()
{
	using P = EntryProblem;
	// 追加しただけの空の行は読み飛ばす
	CHECK(check_entries({{"", "", ""}, {"123", "a.png", ""}}).problem == P::None);
	CHECK(check_entries({}).problem == P::None);
	// 覚え書きだけの行も、ID と画像が空なら読み飛ばす
	CHECK(check_entries({{"", "", "memo"}}).problem == P::None);

	const EntryCheck missing_id = check_entries({{"123", "a.png", ""}, {"", "b.png", ""}});
	CHECK(missing_id.problem == P::MissingUserId && missing_id.index == 1);
	CHECK(check_entries({{"12a", "a.png", ""}}).problem == P::InvalidUserId);
	CHECK(check_entries({{"123", "", ""}}).problem == P::MissingImage);

	// 重複は後ろの行を指す
	const EntryCheck duplicate = check_entries({{"123", "a.png", ""}, {"456", "b.png", ""}, {"123", "c.png", ""}});
	CHECK(duplicate.problem == P::DuplicateUserId && duplicate.index == 2);
}

} // namespace

int main()
{
	test_markers_name_this_plugin();
	test_replace_appends_when_absent();
	test_replace_on_empty_css();
	test_replace_keeps_user_css_outside_block();
	test_replace_twice_keeps_single_block();
	test_replace_refuses_broken_markers();
	test_replace_ignores_other_plugin_markers();
	test_undo_decision();
	test_streamkit_voice_url();
	test_hex_color();
	test_sanitize();
	test_sanitize_font_name();
	test_css_string();
	test_user_id();
	test_image_format();
	test_image_size_limits();
	test_base64();
	test_voice_sections_table();
	test_css_has_no_external_resources();
	test_css_portraits_target_registered_users_only();
	test_generated_css_in_managed_block();
	test_name_takes_its_own_line_height();
	test_abbreviate_data_uris();
	test_check_entries();

	if (failures > 0) {
		std::cerr << failures << " check(s) failed\n";
		return 1;
	}
	std::cout << "all checks passed\n";
	return 0;
}
