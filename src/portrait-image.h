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

// 立ち絵の元の画像を読み、縮小して埋め込む形式に書き出す。OBS に依存せず、計測用の実行ファイルとも共有する

#include "core/portrait-embed.h"
#include "core/portrait.h"

#include <QImage>
#include <QString>

#include <optional>

namespace sspot {

// ファイルの大きさ、拡張子、先頭のバイト列、寸法を確かめる。画素は読まない
core::ImageProblem inspect_source_image(const QString &path);

struct LoadedImage {
	core::ImageProblem problem = core::ImageProblem::None;
	QImage image;
};

// inspect_source_image() と同じ検査をしたうえで画素を読む。JPEG の Exif の向きは画素に反映する
LoadedImage load_source_image(const QString &path);

// この環境の Qt が WebP を書き出せるときは WebP、書き出せないときは PNG
core::ImageFormat embed_format();

// 高さを target_height まで縮小し (元より大きくはしない)、format で書き出す。quality は WebP にだけ使う
std::optional<core::EncodedImage> encode_portrait(const QImage &image, int target_height, core::ImageFormat format,
						  int quality);

// 縮小だけを行う。計測で縮小と書き出しの時間を分けて測るため、encode_portrait() と別に置く
QImage scale_portrait(const QImage &image, int target_height);

// 縮小済みの画像を format で書き出す
std::optional<core::EncodedImage> write_portrait(const QImage &image, core::ImageFormat format, int quality);

} // namespace sspot
