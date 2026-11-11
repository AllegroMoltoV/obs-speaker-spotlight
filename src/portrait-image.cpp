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

#include "portrait-image.h"

#include <QBuffer>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QImageWriter>

#include <algorithm>
#include <cstddef>
#include <string_view>

namespace sspot {

namespace {

// 先頭のバイト列で形式を判定するのに足りる長さ
constexpr qint64 HEADER_BYTES = 16;

// 16 ビットの PNG は 1 画素 8 バイトで読まれるため、その大きさを許す
constexpr unsigned long long MAX_BYTES_PER_PIXEL = 8;

core::ImageProblem check_file(const QString &path, QSize *size)
{
	const QFileInfo info(path);
	if (!info.isFile())
		return core::ImageProblem::Unreadable;
	const core::ImageProblem by_size =
		core::check_source_size(static_cast<std::size_t>(std::max<qint64>(info.size(), 0)));
	if (by_size != core::ImageProblem::None)
		return by_size;

	QFile file(path);
	if (!file.open(QIODevice::ReadOnly))
		return core::ImageProblem::Unreadable;
	const QByteArray header = file.read(HEADER_BYTES);
	file.close();
	const core::ImageProblem by_content =
		core::check_image(info.fileName().toStdString(),
				  std::string_view(header.constData(), static_cast<std::size_t>(header.size())));
	if (by_content != core::ImageProblem::None)
		return by_content;

	QImageReader reader(path);
	*size = reader.size();
	return core::check_source_dimensions(size->width(), size->height());
}

} // namespace

core::ImageProblem inspect_source_image(const QString &path)
{
	QSize size;
	return check_file(path, &size);
}

LoadedImage load_source_image(const QString &path)
{
	QSize size;
	const core::ImageProblem problem = check_file(path, &size);
	if (problem != core::ImageProblem::None)
		return {problem, {}};

	QImageReader reader(path);
	reader.setAutoTransform(true);
	// Qt 6 は既定で 256 MB を超える画像を読まない。上限の 1 億画素を読めるよう、この画像の分だけ一時的に上げる
	const int previous_limit = QImageReader::allocationLimit();
	const unsigned long long needed_mb = static_cast<unsigned long long>(size.width()) *
						     static_cast<unsigned long long>(size.height()) *
						     MAX_BYTES_PER_PIXEL / 1048576 +
					     1;
	if (previous_limit > 0 && needed_mb > static_cast<unsigned long long>(previous_limit))
		QImageReader::setAllocationLimit(static_cast<int>(needed_mb));
	QImage image = reader.read();
	QImageReader::setAllocationLimit(previous_limit);
	if (image.isNull())
		return {core::ImageProblem::Unreadable, {}};
	return {core::ImageProblem::None, std::move(image)};
}

core::ImageFormat embed_format()
{
	static const bool webp = QImageWriter::supportedImageFormats().contains("webp");
	return webp ? core::ImageFormat::Webp : core::ImageFormat::Png;
}

QImage scale_portrait(const QImage &image, int target_height)
{
	if (target_height <= 0 || image.height() <= target_height)
		return image;
	return image.scaledToHeight(target_height, Qt::SmoothTransformation);
}

std::optional<core::EncodedImage> write_portrait(const QImage &image, core::ImageFormat format, int quality)
{
	const char *name = format == core::ImageFormat::Webp  ? "webp"
			   : format == core::ImageFormat::Png ? "png"
							      : nullptr;
	if (!name || image.isNull())
		return std::nullopt;

	QByteArray bytes;
	QBuffer buffer(&bytes);
	buffer.open(QIODevice::WriteOnly);
	QImageWriter writer(&buffer, name);
	if (format == core::ImageFormat::Webp)
		writer.setQuality(quality);
	if (!writer.write(image))
		return std::nullopt;
	return core::EncodedImage{format, std::string(bytes.constData(), static_cast<std::size_t>(bytes.size()))};
}

std::optional<core::EncodedImage> encode_portrait(const QImage &image, int target_height, core::ImageFormat format,
						  int quality)
{
	return write_portrait(scale_portrait(image, target_height), format, quality);
}

} // namespace sspot
