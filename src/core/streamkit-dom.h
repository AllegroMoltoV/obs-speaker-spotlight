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

#pragma once

// StreamKit の音声ウィジェットのセレクター。StreamKit の DOM が変わったときに直す場所をここだけにする。
// ハッシュの無いクラスと data-userid を使い、ハッシュ付きのクラスしか無い要素は部分一致で選ぶ

#include <string>
#include <string_view>

namespace vspot::core::streamkit {

inline constexpr std::string_view BODY = "body";
inline constexpr std::string_view LIST = ".voice_states";             // 参加者の一覧の ul
inline constexpr std::string_view ITEM = ".voice_state";              // 1 人分の li
inline constexpr std::string_view SPEAKING = ".wrapper_speaking";     // 話している人の li に付く
inline constexpr std::string_view AVATAR = ".voice_avatar";           // アバターの img
inline constexpr std::string_view USER = ".voice_username";           // 名前を包む div
inline constexpr std::string_view NAME = "[class*=\"Voice_name__\"]"; // 名前の span。ハッシュ付きのクラスだけを持つ
inline constexpr std::string_view USER_ID_ATTRIBUTE = "data-userid";

// 指定のユーザーの li。user_id は is_valid_user_id() を通った値だけを渡す
inline std::string item_of_user(std::string_view user_id)
{
	return std::string(ITEM) + "[" + std::string(USER_ID_ATTRIBUTE) + "=\"" + std::string(user_id) + "\"]";
}

} // namespace vspot::core::streamkit
