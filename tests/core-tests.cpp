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

// OBS に依存しない処理の単体テスト。失敗した検査を標準エラーに書き、終了コード 1 で終わる

#include "core/css-builder.h"
#include "core/managed-css.h"
#include "core/streamkit-url.h"
#include "core/style-schema.h"

#include <iostream>
#include <optional>
#include <string>

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

using namespace vspot::core;

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
	CHECK(BLOCK_BEGIN == "/* voice-spotlight begin */");
	CHECK(BLOCK_END == "/* voice-spotlight end */");
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

	if (failures > 0) {
		std::cerr << failures << " check(s) failed\n";
		return 1;
	}
	std::cout << "all checks passed\n";
	return 0;
}
