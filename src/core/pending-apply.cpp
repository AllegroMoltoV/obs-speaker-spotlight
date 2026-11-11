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

#include "pending-apply.h"

#include <utility>

namespace sspot::core {

std::optional<std::string> PendingApply::mark(std::string_view source_id)
{
	std::optional<std::string> previous;
	if (source_id_ && *source_id_ != source_id)
		previous = std::move(source_id_);
	source_id_ = std::string(source_id);
	return previous;
}

std::optional<std::string> PendingApply::take()
{
	std::optional<std::string> taken = std::move(source_id_);
	source_id_.reset();
	return taken;
}

} // namespace sspot::core
