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

#include "portrait-embed.h"

#include <algorithm>

namespace sspot::core {

std::vector<EmbedStep> embed_steps(bool lossy)
{
	if (!lossy)
		return {{200, 100}, {150, 100}, {100, 100}};
	std::vector<EmbedStep> steps = {{200, WEBP_DEFAULT_QUALITY},
					{150, WEBP_DEFAULT_QUALITY},
					{100, WEBP_DEFAULT_QUALITY}};
	for (const int quality : {75, 60, 50})
		if (quality < WEBP_DEFAULT_QUALITY)
			steps.push_back({100, quality});
	return steps;
}

EmbedPlan plan_embedding(std::size_t count, const std::vector<EmbedStep> &steps, const EmbedEncoder &encode)
{
	EmbedPlan plan;
	if (count == 0)
		return plan;
	if (steps.empty()) {
		plan.problem = EmbedProblem::EncodeFailed;
		return plan;
	}

	std::vector<std::size_t> chosen(count, 0);
	for (std::size_t floor = 0;; ++floor) {
		plan.images.assign(count, {});
		std::size_t total = 0;
		bool total_exceeded = false;
		for (std::size_t i = 0; i < count; ++i) {
			std::size_t step = std::max(chosen[i], floor);
			std::optional<EncodedImage> image;
			for (; step < steps.size(); ++step) {
				image = encode(i, steps[step]);
				if (!image) {
					plan.problem = EmbedProblem::EncodeFailed;
					plan.index = i;
					plan.images.clear();
					return plan;
				}
				if (image->bytes.size() <= MAX_IMAGE_BYTES)
					break;
			}
			if (step == steps.size()) {
				plan.problem = EmbedProblem::TooLarge;
				plan.index = i;
				plan.images.clear();
				return plan;
			}
			chosen[i] = step;
			// 足し算の桁あふれを避けるため、残りの枠と比べる。超えた時点で次の下限へ進み、残りは書き出さない
			if (image->bytes.size() > MAX_TOTAL_IMAGE_BYTES - total) {
				total_exceeded = true;
				break;
			}
			total += image->bytes.size();
			plan.images[i] = {step, std::move(*image)};
		}
		if (!total_exceeded)
			return plan;
		if (floor + 1 >= steps.size()) {
			plan.problem = EmbedProblem::TotalTooLarge;
			plan.images.clear();
			return plan;
		}
	}
}

} // namespace sspot::core
