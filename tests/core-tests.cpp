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
#include "core/pending-apply.h"
#include "core/portrait-embed.h"
#include "core/portrait.h"
#include "core/streamkit-url.h"
#include "core/style-schema.h"
#include "core/voice-style.h"

#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <utility>
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

void test_remove_block()
{
	const std::string before = "/* mine */\n.user { color: blue; }\n";
	const std::string after = "\n.user2 { color: green; }\n";
	const std::string block = std::string(BLOCK_BEGIN) + "\n.old {}\n" + std::string(BLOCK_END);
	const auto removed = remove_block(before + block + after);
	CHECK(removed.has_value());
	CHECK(*removed == before + ".user2 { color: green; }\n");

	// 末尾に足した範囲を外すと、足す前の css に戻る
	CHECK(remove_block(*replace_block(DEFAULT_CSS, ".a {}")) == DEFAULT_CSS);
	CHECK(remove_block(DEFAULT_CSS) == DEFAULT_CSS);
	CHECK(!remove_block(DEFAULT_CSS + "\n" + std::string(BLOCK_BEGIN)).has_value());
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
const std::string WEBP_BYTES = std::string("RIFF\x10\x00\x00\x00WEBPVP8L", 16) + "rest-of-webp";

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
	// 元の画像として受け付けるのは PNG と JPEG だけ
	CHECK(check_image("a.webp", WEBP_BYTES) == ImageProblem::UnsupportedExtension);
	CHECK(check_image("a.png", WEBP_BYTES) == ImageProblem::ExtensionMismatch);
	CHECK(detect_image_format(std::string("RIFF\x10\x00\x00\x00WAVE", 12)) == ImageFormat::Unknown);
	CHECK(mime_type(detect_image_format(PNG_BYTES)) == "image/png");
	CHECK(mime_type(detect_image_format(JPEG_BYTES)) == "image/jpeg");
	CHECK(mime_type(detect_image_format(WEBP_BYTES)) == "image/webp");
	// 立ち絵は MIME タイプで保存するため、読み戻すと同じ形式になる
	for (const ImageFormat format : {ImageFormat::Png, ImageFormat::Jpeg, ImageFormat::Webp})
		CHECK(format_from_mime(mime_type(format)) == format);
	CHECK(format_from_mime("") == ImageFormat::Unknown);
	CHECK(format_from_mime("image/gif") == ImageFormat::Unknown);
}

void test_source_limits()
{
	CHECK(check_source_size(MAX_SOURCE_FILE_BYTES) == ImageProblem::None);
	CHECK(check_source_size(MAX_SOURCE_FILE_BYTES + 1) == ImageProblem::TooLarge);
	// 元の画像の 1 MB の上限は外したため、1 MB を超えるファイルも受け付ける
	CHECK(check_source_size(MAX_IMAGE_BYTES + 1) == ImageProblem::None);

	CHECK(check_source_dimensions(10000, 10000) == ImageProblem::None);
	CHECK(check_source_dimensions(10000, 10001) == ImageProblem::TooManyPixels);
	CHECK(check_source_dimensions(1, 100000000) == ImageProblem::None);
	CHECK(check_source_dimensions(2, 50000001) == ImageProblem::TooManyPixels);
	CHECK(check_source_dimensions(2147483647, 2147483647) == ImageProblem::TooManyPixels);
	CHECK(check_source_dimensions(0, 100) == ImageProblem::Unreadable);
	CHECK(check_source_dimensions(-1, -1) == ImageProblem::Unreadable);
}

// 段ごとに決まった大きさを返す偽のエンコーダー。sizes[段の位置] のバイト数の画像を返し、呼ばれた段を記録する
struct FakeEncoder {
	std::vector<std::size_t> sizes;
	std::vector<EmbedStep> steps;
	std::vector<std::size_t> calls;
	bool failing = false;

	EmbedChoice run()
	{
		return choose_embedding(steps, [this](const EmbedStep &step) -> std::optional<EncodedImage> {
			std::size_t position = 0;
			while (position < steps.size() && !(steps[position] == step))
				++position;
			calls.push_back(position);
			if (failing)
				return std::nullopt;
			return EncodedImage{ImageFormat::Webp, std::string(sizes[position], 'x'), 0, 0};
		});
	}
};

void test_embed_steps()
{
	const std::vector<EmbedStep> webp = embed_steps(true);
	CHECK(webp.size() >= 4);
	CHECK((webp[0] == EmbedStep{100, WEBP_DEFAULT_QUALITY}));
	CHECK((webp[1] == EmbedStep{75, WEBP_DEFAULT_QUALITY}));
	CHECK((webp[2] == EmbedStep{50, WEBP_DEFAULT_QUALITY}));
	// 倍率を下げきった後は、倍率を保ったまま画質だけを下げる
	for (std::size_t i = 3; i < webp.size(); ++i) {
		CHECK(webp[i].scale_percent == 50);
		CHECK(webp[i].quality < webp[i - 1].quality);
	}
	const std::vector<EmbedStep> png = embed_steps(false);
	CHECK(png.size() == 3);
	CHECK(png[0].scale_percent == 100 && png[1].scale_percent == 75 && png[2].scale_percent == 50);
	// 最初の段は、表示の高さの上限 600px の 2 倍で書き出す
	CHECK(EMBED_HEIGHT_LIMIT * webp[0].scale_percent / 100 == 1200);
}

void test_choose_embedding()
{
	const std::size_t over = MAX_IMAGE_BYTES + 1;
	FakeEncoder first;
	first.steps = embed_steps(true);
	first.sizes.assign(first.steps.size(), 1000);
	const EmbedChoice fits = first.run();
	CHECK(fits.problem == EmbedProblem::None);
	CHECK(fits.step_index == 0);
	CHECK(fits.image.format == ImageFormat::Webp);
	// 収まった段より後の段は書き出さない
	CHECK((first.calls == std::vector<std::size_t>{0}));

	FakeEncoder exact = first;
	exact.calls.clear();
	exact.sizes[0] = over;
	exact.sizes[1] = MAX_IMAGE_BYTES;
	const EmbedChoice second = exact.run();
	CHECK(second.problem == EmbedProblem::None);
	CHECK(second.step_index == 1);
	CHECK(second.image.bytes.size() == MAX_IMAGE_BYTES);

	FakeEncoder last = first;
	last.calls.clear();
	last.sizes.assign(last.steps.size(), over);
	last.sizes.back() = 10;
	CHECK(last.run().step_index == last.steps.size() - 1);

	FakeEncoder too_large = first;
	too_large.sizes.assign(too_large.steps.size(), over);
	const EmbedChoice failed = too_large.run();
	CHECK(failed.problem == EmbedProblem::TooLarge);
	CHECK(failed.image.bytes.empty());

	FakeEncoder broken = first;
	broken.failing = true;
	CHECK(broken.run().problem == EmbedProblem::EncodeFailed);
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

const std::string USER_ID = "111";

PortraitImage portrait_of(const std::string &bytes)
{
	return PortraitImage{detect_image_format(bytes), base64_encode(bytes), 10, 20};
}

StyleValues max_values()
{
	StyleValues values;
	for (const auto &section : voice_sections())
		for (const auto &item : section.items)
			if (item.kind == ItemKind::Number || item.kind == ItemKind::Toggle)
				values.numbers[std::string(item.key)] = item.max;
	values.texts["name_font"] = "x\"); } @import url(http://example.com/x.css); a {";
	values.colors["name_color"] = Rgb{1, 2, 3};
	values.colors["name_background"] = Rgb{4, 5, 6};
	return values;
}

size_t count_urls(const std::string &css)
{
	size_t urls = 0;
	for (size_t pos = css.find("url("); pos != std::string::npos; pos = css.find("url(", pos + 1)) {
		++urls;
		CHECK(css.compare(pos, 10, "url(\"data:") == 0);
	}
	return urls;
}

void test_css_has_no_external_resources()
{
	for (const StyleValues &values : {StyleValues{}, max_values()}) {
		for (const std::string &bytes : {PNG_BYTES, JPEG_BYTES, WEBP_BYTES}) {
			const std::string css = generate_css(values, USER_ID, portrait_of(bytes));
			CHECK(css.find("@import") == std::string::npos);
			CHECK(count_urls(css) == 1);
			// 保存した形式の MIME タイプで埋め込む
			CHECK(css.find("url(\"data:" + std::string(mime_type(detect_image_format(bytes))) + ";base64," +
				       base64_encode(bytes) + "\")") != std::string::npos);
		}
		// base64 でない本文と、形式の分からない画像は CSS に出さない
		const PortraitImage injected{ImageFormat::Png, "abc\") } @import url(http://example.com/y.css", 1, 1};
		const PortraitImage unknown{ImageFormat::Unknown, base64_encode(PNG_BYTES), 1, 1};
		for (const PortraitImage &bad : {injected, unknown}) {
			const std::string css = generate_css(values, USER_ID, bad);
			CHECK(css.find("@import") == std::string::npos);
			CHECK(count_urls(css) == 0);
		}
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

// 規則 selector の本文
std::string rule_of(const std::string &css, const std::string &selector)
{
	const size_t start = css.find(selector + " {\n");
	if (start == std::string::npos)
		return {};
	return css.substr(start, css.find("}\n", start) - start);
}

void test_css_shows_only_registered_user()
{
	const std::string css = generate_css(StyleValues{}, USER_ID, portrait_of(PNG_BYTES));
	const std::string target = ".voice_state[data-userid=\"111\"]";
	// 登録した 1 人以外の li を隠し、その 1 人だけを表示する
	CHECK(rule_of(css, ".voice_state").find("display: none !important;") != std::string::npos);
	CHECK(rule_of(css, target).find("display: flex !important;") != std::string::npos);
	CHECK(selectors_with(css, "content") == std::vector<std::string>{target + " .voice_avatar"});
	// 明るさの切り替えは、立ち絵の有無にかかわらず img に掛かる
	CHECK(selectors_with(css, "filter") ==
	      (std::vector<std::string>{".voice_state .voice_avatar", ".voice_state.wrapper_speaking .voice_avatar"}));
	// 並べ方の規則は出さない
	CHECK(selectors_with(css, "order").empty());

	// 立ち絵が無くても、登録した 1 人だけを表示する
	const std::string without = generate_css(StyleValues{}, USER_ID);
	CHECK(selectors_with(without, "content").empty());
	CHECK(rule_of(without, ".voice_state").find("display: none !important;") != std::string::npos);
	CHECK(rule_of(without, target).find("display: flex !important;") != std::string::npos);
}

void test_update_css()
{
	const std::string before = DEFAULT_CSS + "\n/* mine */\n.voice_state { outline: 1px solid red; }\n";
	const std::string after = "\n.mine-after { color: blue; }\n";
	const std::string empty_block = std::string(BLOCK_BEGIN) + "\n" + std::string(BLOCK_END);
	const CssUpdate first =
		update_css(before + empty_block + after, StyleValues{}, USER_ID, portrait_of(PNG_BYTES));
	CHECK(first.problem == CssUpdateProblem::None);
	const CssUpdate second = update_css(first.css, max_values(), "222", std::nullopt);
	CHECK(second.problem == CssUpdateProblem::None);
	CHECK(second.css.rfind(before, 0) == 0);
	CHECK(second.css.size() >= after.size() &&
	      second.css.compare(second.css.size() - after.size(), after.size(), after) == 0);
	CHECK(second.css.find("data-userid=\"111\"") == std::string::npos);
	CHECK(second.css.find("data-userid=\"222\"") != std::string::npos);
	CHECK(inspect_block(second.css) == BlockState::Intact);

	// ID が未入力なら管理範囲を外し、範囲の外の CSS は残す
	const CssUpdate removed = update_css(second.css, StyleValues{}, "", portrait_of(PNG_BYTES));
	CHECK(removed.problem == CssUpdateProblem::None);
	CHECK(inspect_block(removed.css) == BlockState::Absent);
	CHECK(removed.css.rfind(before, 0) == 0);
	CHECK(removed.css.find(".mine-after { color: blue; }") != std::string::npos);
	CHECK(removed.css.find("data-userid") == std::string::npos);
	// 範囲が無ければ何も変えない
	CHECK(update_css(DEFAULT_CSS, StyleValues{}, "", std::nullopt).css == DEFAULT_CSS);

	// 目印が壊れているときと、ID が不正なときは書かない
	const std::string broken = DEFAULT_CSS + "\n" + std::string(BLOCK_END);
	CHECK(update_css(broken, StyleValues{}, USER_ID, std::nullopt).problem == CssUpdateProblem::BrokenMarkers);
	CHECK(update_css(broken, StyleValues{}, "", std::nullopt).problem == CssUpdateProblem::BrokenMarkers);
	CHECK(update_css(DEFAULT_CSS, StyleValues{}, "1\" ] body{", std::nullopt).problem ==
	      CssUpdateProblem::InvalidUserId);
}

// 書き込み済みと同じ css になるときは、ページを作り直させないよう書き込まない
void test_update_css_reports_unchanged()
{
	const CssUpdate first = update_css(DEFAULT_CSS, max_values(), USER_ID, portrait_of(PNG_BYTES));
	CHECK(first.changed);
	const CssUpdate again = update_css(first.css, max_values(), USER_ID, portrait_of(PNG_BYTES));
	CHECK(again.problem == CssUpdateProblem::None);
	CHECK(!again.changed);
	CHECK(update_css(first.css, StyleValues{}, USER_ID, portrait_of(PNG_BYTES)).changed);
	CHECK(update_css(first.css, max_values(), USER_ID, std::nullopt).changed);
	CHECK(update_css(first.css, max_values(), "222", portrait_of(PNG_BYTES)).changed);
	// ID を消すと管理範囲を外す。外した後にもう一度反映しても書かない
	const CssUpdate removed = update_css(first.css, max_values(), "", std::nullopt);
	CHECK(removed.changed);
	CHECK(!update_css(removed.css, max_values(), "", std::nullopt).changed);
	// 書かないときは changed にしない
	const std::string broken = DEFAULT_CSS + "\n" + std::string(BLOCK_END);
	CHECK(!update_css(broken, StyleValues{}, USER_ID, std::nullopt).changed);
}

// 反映待ちはソースの ID に結び付き、別のソースへ移ると元のソースを書き込む対象として返す
void test_pending_apply()
{
	PendingApply pending;
	CHECK(!pending.pending());
	CHECK(!pending.take().has_value());

	// 同じソースへの操作の繰り返しは 1 回の反映にまとめる
	CHECK(!pending.mark("a").has_value());
	CHECK(!pending.mark("a").has_value());
	CHECK(pending.pending());
	CHECK(pending.take() == std::optional<std::string>("a"));
	CHECK(!pending.pending());
	CHECK(!pending.take().has_value());

	// 別のソースへの操作は、前のソースを先に書き込ませる
	pending.mark("a");
	CHECK(pending.mark("b") == std::optional<std::string>("a"));
	CHECK(pending.take() == std::optional<std::string>("b"));
}

// 名前の背景が立ち絵や隣の参加者に重ならないよう、表示するときは span に行の高さを取らせる
void test_name_takes_its_own_line_height()
{
	for (const StyleValues &values : {StyleValues{}, max_values()}) {
		const std::string css = generate_css(values, USER_ID);
		CHECK(css.find("  display: inline-block !important;\n") != std::string::npos);
		CHECK(css.find("  line-height: normal !important;\n") != std::string::npos);
	}
	StyleValues hidden;
	hidden.numbers["name_visible"] = 0;
	const std::string css = generate_css(hidden, USER_ID);
	CHECK(css.find("inline-block") == std::string::npos);
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
	test_remove_block();
	test_streamkit_voice_url();
	test_hex_color();
	test_sanitize();
	test_sanitize_font_name();
	test_css_string();
	test_user_id();
	test_image_format();
	test_source_limits();
	test_embed_steps();
	test_choose_embedding();
	test_base64();
	test_voice_sections_table();
	test_css_has_no_external_resources();
	test_css_shows_only_registered_user();
	test_update_css();
	test_update_css_reports_unchanged();
	test_pending_apply();
	test_name_takes_its_own_line_height();

	if (failures > 0) {
		std::cerr << failures << " check(s) failed\n";
		return 1;
	}
	std::cout << "all checks passed\n";
	return 0;
}
