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

#include <string_view>

namespace sspot::core {

// StreamKit の音声ウィジェットの URL の形に当てはまるときに true を返す。
// 形は https://streamkit.discord.com/overlay/voice/<サーバー ID>/<チャンネル ID> で、
// 旧ドメインの streamkit.discordapp.com も受け付ける。
// 当てはまらなくても適用は止めず、ドックの警告にだけ使う
bool is_streamkit_voice_url(std::string_view url);

} // namespace sspot::core
