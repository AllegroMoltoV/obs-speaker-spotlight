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

// 埋め込む画像の縮小の倍率と画質の段を選ぶ。画像の処理は呼び出し側のエンコーダーに任せ、ここでは段の選び方だけを決める

#include "portrait.h"

#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace sspot::core {

// 埋め込む画像の高さの上限 (px)。表示の高さの上限 600px の 2 倍。
// 表示の高さを変えても画像を作り直さずに済むよう、表示の高さではなくこの上限を基準に縮小する
inline constexpr int EMBED_HEIGHT_LIMIT = 1200;

// EMBED_HEIGHT_LIMIT に掛ける倍率 (百分率) と、WebP の画質。画質 100 は可逆
struct EmbedStep {
	int scale_percent = 100;
	int quality = 100;

	bool operator==(const EmbedStep &other) const
	{
		return scale_percent == other.scale_percent && quality == other.quality;
	}
};

inline constexpr int WEBP_DEFAULT_QUALITY = 90;

// 上限を超えるときに試す順の段。lossy が true (WebP) では倍率を 1、0.75、0.5 と下げた後に画質を下げる。
// false (PNG) では画質を持たないため、倍率だけを下げる
std::vector<EmbedStep> embed_steps(bool lossy);

struct EncodedImage {
	ImageFormat format = ImageFormat::Unknown;
	std::string bytes;
	int width = 0;
	int height = 0;
};

// 画像を step の段で書き出す。書き出せないときは nullopt を返す
using EmbedEncoder = std::function<std::optional<EncodedImage>(const EmbedStep &step)>;

enum class EmbedProblem {
	None,
	EncodeFailed, // エンコーダーが書き出せなかった
	TooLarge,     // 最後の段でも上限を超える
};

struct EmbedChoice {
	EmbedProblem problem = EmbedProblem::None;
	std::size_t step_index = 0;
	EncodedImage image;
};

// MAX_IMAGE_BYTES に収まる最初の段で書き出す。収まった段より後の段は書き出さない
EmbedChoice choose_embedding(const std::vector<EmbedStep> &steps, const EmbedEncoder &encode);

} // namespace sspot::core
