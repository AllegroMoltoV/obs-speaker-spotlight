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

#include "voice-style.h"

#include "css-builder.h"

namespace vspot::core {

std::string generate_css(const StyleValues &values)
{
	// 項目の表がまだ空のため、値を読んでも規則は生まれない
	const css::Reader reader(voice_sections(), values);
	(void)reader;
	css::CssWriter css;
	return css.str();
}

} // namespace vspot::core
