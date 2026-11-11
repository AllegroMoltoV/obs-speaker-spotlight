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

#include <utility>

namespace sspot::core {

std::vector<EmbedStep> embed_steps(bool lossy)
{
	if (!lossy)
		return {{100, 100}, {75, 100}, {50, 100}};
	std::vector<EmbedStep> steps = {{100, WEBP_DEFAULT_QUALITY},
					{75, WEBP_DEFAULT_QUALITY},
					{50, WEBP_DEFAULT_QUALITY}};
	for (const int quality : {75, 60, 50})
		if (quality < WEBP_DEFAULT_QUALITY)
			steps.push_back({50, quality});
	return steps;
}

EmbedChoice choose_embedding(const std::vector<EmbedStep> &steps, const EmbedEncoder &encode)
{
	EmbedChoice choice;
	for (std::size_t step = 0; step < steps.size(); ++step) {
		std::optional<EncodedImage> image = encode(steps[step]);
		if (!image) {
			choice.problem = EmbedProblem::EncodeFailed;
			return choice;
		}
		if (image->bytes.size() <= MAX_IMAGE_BYTES) {
			choice.step_index = step;
			choice.image = std::move(*image);
			return choice;
		}
	}
	choice.problem = steps.empty() ? EmbedProblem::EncodeFailed : EmbedProblem::TooLarge;
	return choice;
}

} // namespace sspot::core
