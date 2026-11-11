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

// 表示の高さに掛ける倍率 (百分率) と、WebP の画質。画質 100 は可逆
struct EmbedStep {
	int scale_percent = 100;
	int quality = 100;

	bool operator==(const EmbedStep &other) const
	{
		return scale_percent == other.scale_percent && quality == other.quality;
	}
};

inline constexpr int WEBP_DEFAULT_QUALITY = 90;

// 上限を超えるときに試す順の段。lossy が true (WebP) では倍率を 2、1.5、1 と下げた後に画質を下げる。
// false (PNG) では画質を持たないため、倍率だけを下げる
std::vector<EmbedStep> embed_steps(bool lossy);

struct EncodedImage {
	ImageFormat format = ImageFormat::Unknown;
	std::string bytes;
};

// index 番目の画像を step の段で書き出す。書き出せないときは nullopt を返す
using EmbedEncoder = std::function<std::optional<EncodedImage>(std::size_t index, const EmbedStep &step)>;

enum class EmbedProblem {
	None,
	EncodeFailed,  // エンコーダーが書き出せなかった
	TooLarge,      // 最後の段でも 1 枚の上限を超える
	TotalTooLarge, // 最後の段でも合計の上限を超える
};

struct EmbeddedImage {
	std::size_t step_index = 0;
	EncodedImage image;
};

struct EmbedPlan {
	EmbedProblem problem = EmbedProblem::None;
	std::size_t index = 0; // EncodeFailed と TooLarge で、問題のある画像の位置
	std::vector<EmbeddedImage> images;
};

// count 枚の画像の段を選ぶ。各画像は 1 枚の上限に収まる最初の段を使う。合計が上限を超えるときは、
// 全員の段の下限を 1 段ずつ下げる。下限より前の段に収まっていた画像も、下限の段で書き出し直す
EmbedPlan plan_embedding(std::size_t count, const std::vector<EmbedStep> &steps, const EmbedEncoder &encode);

} // namespace sspot::core
