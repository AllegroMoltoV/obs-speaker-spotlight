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

#pragma once

// 立ち絵の登録と、その入力の検証

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace sspot::core {

// 埋め込む画像の大きさの上限。元の画像ではなく、縮小して書き出した後の画像に掛ける。
// 1 枚の上限は base64 にして約 140 万文字で、Chromium の URL の上限 2,097,152 文字を下回る
inline constexpr std::size_t MAX_IMAGE_BYTES = 1048576;
inline constexpr std::size_t MAX_TOTAL_IMAGE_BYTES = 4194304;

// 元の画像の上限。画素を読む前にファイルの大きさと寸法で確かめ、OBS のメモリを使い切らないようにする
inline constexpr std::size_t MAX_SOURCE_FILE_BYTES = 50 * 1048576;
inline constexpr unsigned long long MAX_SOURCE_PIXELS = 100000000;

// Discord のユーザー ID の桁数の上限。64 ビットの符号なし整数の最大値が 20 桁のため
inline constexpr std::size_t MAX_USER_ID_DIGITS = 20;

// 半角数字だけで 1〜20 桁のときに true を返す。値は CSS のセレクターに入るため、ほかの文字は受け付けない
bool is_valid_user_id(std::string_view user_id);

enum class ImageFormat {
	Unknown,
	Png,
	Jpeg,
	Webp,
};

// ファイルの先頭のバイト列から形式を判定する
ImageFormat detect_image_format(std::string_view bytes);

// ファイル名の拡張子から形式を判定する。.png、.jpg、.jpeg を大文字小文字を区別せずに受け付ける
ImageFormat format_from_file_name(std::string_view file_name);

// data URI に書く MIME タイプ。Unknown では空を返す
std::string_view mime_type(ImageFormat format);

enum class ImageProblem {
	None,
	UnsupportedExtension, // 拡張子が .png、.jpg、.jpeg のどれでもない
	UnknownContent,       // 中身が png でも jpg でもない
	ExtensionMismatch,    // 拡張子と中身の形式が違う
	TooLarge,             // ファイルが元の画像の上限を超える
	TooManyPixels,        // 寸法が元の画像の画素数の上限を超える
	Unreadable,           // ファイルを開けない、または画像として読めない
};

// 元の画像のファイルの大きさを、読む前に確かめる
ImageProblem check_source_size(std::size_t size);

// 元の画像の寸法を、画素を読む前に確かめる。寸法が分からないときは 0 以下を渡す
ImageProblem check_source_dimensions(long long width, long long height);

// 1 枚の画像のファイル名と先頭のバイト列を確かめる。拡張子、中身、両者の一致の順に確かめ、最初の問題を返す
ImageProblem check_image(std::string_view file_name, std::string_view header);

std::string base64_encode(std::string_view bytes);

// ドックの立ち絵の一覧の 1 行。保存とプリセットにはこの形で置き、画像の中身は持たない
struct PortraitEntry {
	std::string user_id;
	std::string image_path;
	std::string memo;

	bool operator==(const PortraitEntry &other) const
	{
		return user_id == other.user_id && image_path == other.image_path && memo == other.memo;
	}
};

enum class EntryProblem {
	None,
	MissingUserId,   // 画像はあるがユーザー ID が空
	InvalidUserId,   // ユーザー ID が is_valid_user_id() を通らない
	DuplicateUserId, // 先の行と同じユーザー ID
	MissingImage,    // ユーザー ID はあるが画像のファイルが選ばれていない
};

struct EntryCheck {
	EntryProblem problem = EntryProblem::None;
	std::size_t index = 0; // 問題のある行の位置
};

// 適用の前に一覧の行を確かめ、最初の問題を返す。ユーザー ID も画像も空の行は、追加しただけの行として読み飛ばす
EntryCheck check_entries(const std::vector<PortraitEntry> &entries);

// 1 人分の立ち絵
struct Portrait {
	std::string user_id;
	ImageFormat format = ImageFormat::Unknown;
	std::string base64; // 埋め込む画像を base64 にしたもの
	int order = 0;      // 表示の順。小さいほど前
	std::string memo;   // ドックの中だけで使う覚え書きの名前。CSS には出さない
};

} // namespace sspot::core
