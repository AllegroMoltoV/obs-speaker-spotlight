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

#include "speaker-preview.h"

#include <obs-module.h>

#include <QByteArray>
#include <QPainter>

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace sspot {

namespace {

// StreamKit のスタイルシートの値
constexpr int AVATAR_BORDER = 3;
constexpr core::Rgb SPEAKING_GREEN{0x3b, 0xa5, 0x3b};
// アバターの画像は通話に入るまで分からないため、Discord の既定のアバターの色の円で代える
constexpr core::Rgb AVATAR_PLACEHOLDER{0x58, 0x65, 0xf2};

constexpr int PANEL_GAP = 16;
constexpr int CAPTION_GAP = 4;
constexpr int CHECKER = 8;

QString text(const char *key)
{
	return QString::fromUtf8(obs_module_text(key));
}

QColor brightened(const core::Rgb &color, int percent)
{
	return QColor(core::brighten(color.r, percent), core::brighten(color.g, percent),
		      core::brighten(color.b, percent));
}

} // namespace

SpeakerPreview::SpeakerPreview(QWidget *parent) : QWidget(parent)
{
	QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	policy.setHeightForWidth(true);
	setSizePolicy(policy);
}

void SpeakerPreview::set_look(const core::VoiceLook &look)
{
	look_ = look;
	updateGeometry();
	update();
}

void SpeakerPreview::set_portrait(const std::optional<core::PortraitImage> &portrait)
{
	portrait_ = QImage();
	if (portrait && core::portrait_usable(*portrait)) {
		const QByteArray bytes = QByteArray::fromBase64(QByteArray::fromStdString(portrait->base64));
		const QImage decoded = QImage::fromData(bytes);
		if (!decoded.isNull())
			portrait_ = decoded.convertToFormat(QImage::Format_ARGB32);
	}
	scaled_ = QImage();
	idle_ = {};
	speaking_ = {};
	updateGeometry();
	update();
}

SpeakerPreview::Metrics SpeakerPreview::measure() const
{
	Metrics l{};
	const int height = look_.avatar_height;
	if (!portrait_.isNull()) {
		// CSS は立ち絵の高さだけを決め、幅は縦横比から決まる。枠は 0 にする
		const int width = std::max(1, static_cast<int>(std::lround(static_cast<double>(height) *
									   portrait_.width() / portrait_.height())));
		l.avatar = QSize(width, height);
		l.border = 0;
	} else {
		l.avatar = QSize(height + 2 * AVATAR_BORDER, height + 2 * AVATAR_BORDER);
		l.border = AVATAR_BORDER;
	}
	l.panel = l.avatar;
	l.caption = fontMetrics().height() + CAPTION_GAP;
	l.total_width = 2 * l.panel.width() + PANEL_GAP;
	return l;
}

double SpeakerPreview::scale_for(const Metrics &l, int width)
{
	if (l.total_width <= 0)
		return 1.0;
	return std::min(1.0, static_cast<double>(std::max(width, 1)) / l.total_width);
}

int SpeakerPreview::heightForWidth(int width) const
{
	const Metrics l = measure();
	return l.caption + static_cast<int>(std::ceil(l.panel.height() * scale_for(l, width)));
}

QSize SpeakerPreview::sizeHint() const
{
	const int width = measure().total_width;
	return QSize(width, heightForWidth(width));
}

QSize SpeakerPreview::minimumSizeHint() const
{
	return QSize(0, measure().caption);
}

const QImage &SpeakerPreview::portrait_image(const QSize &size, int percent, bool speaking)
{
	if (scaled_.size() != size) {
		scaled_ = portrait_.scaled(size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)
				  .convertToFormat(QImage::Format_ARGB32);
		idle_ = {};
		speaking_ = {};
	}
	Brightened &cache = speaking ? speaking_ : idle_;
	if (cache.percent != percent || cache.image.isNull()) {
		cache.image = scaled_.copy();
		for (int y = 0; y < cache.image.height(); ++y)
			core::brighten_pixels(reinterpret_cast<std::uint32_t *>(cache.image.scanLine(y)),
					      static_cast<std::size_t>(cache.image.width()), percent);
		cache.percent = percent;
	}
	return cache.image;
}

void SpeakerPreview::paintEvent(QPaintEvent *)
{
	const Metrics l = measure();
	const double scale = scale_for(l, width());
	const int body_top = l.caption;

	QPainter painter(this);

	// 透明を表す市松模様。ブラウザーソースの背景は透明のため
	const QColor light(0x80, 0x80, 0x80);
	const QColor dark(0x5a, 0x5a, 0x5a);
	for (int y = body_top; y < height(); y += CHECKER)
		for (int x = 0; x < width(); x += CHECKER)
			painter.fillRect(x, y, CHECKER, CHECKER,
					 ((x / CHECKER + (y - body_top) / CHECKER) % 2) ? dark : light);

	const QFont caption_font = font();

	for (int i = 0; i < 2; ++i) {
		const bool speaking = i == 1;
		const int percent = speaking ? look_.speaking_brightness : look_.idle_brightness;
		const double left = i * (l.panel.width() + PANEL_GAP) * scale;

		painter.setFont(caption_font);
		painter.setPen(palette().color(QPalette::WindowText));
		painter.drawText(QRectF(left, 0, l.panel.width() * scale + PANEL_GAP, l.caption),
				 Qt::AlignLeft | Qt::AlignVCenter,
				 text(speaking ? "Preview.Speaking" : "Preview.Idle"));

		painter.save();
		painter.translate(left, body_top);
		painter.scale(scale, scale);
		painter.setRenderHint(QPainter::Antialiasing);
		painter.setRenderHint(QPainter::SmoothPixmapTransform);

		const QRectF avatar(0, (l.panel.height() - l.avatar.height()) / 2.0, l.avatar.width(),
				    l.avatar.height());
		if (!portrait_.isNull()) {
			const double ratio = scale * devicePixelRatioF();
			const QSize pixels(std::max(1, static_cast<int>(std::lround(avatar.width() * ratio))),
					   std::max(1, static_cast<int>(std::lround(avatar.height() * ratio))));
			painter.drawImage(avatar, portrait_image(pixels, percent, speaking));
		} else {
			// CSS の filter は枠にも掛かるため、枠の色にも明るさを掛ける
			painter.setPen(Qt::NoPen);
			painter.setBrush(brightened(AVATAR_PLACEHOLDER, percent));
			painter.drawEllipse(avatar.adjusted(l.border, l.border, -l.border, -l.border));
			if (speaking && look_.speaking_border_visible) {
				const double half = l.border / 2.0;
				painter.setBrush(Qt::NoBrush);
				painter.setPen(QPen(brightened(SPEAKING_GREEN, percent), l.border));
				painter.drawEllipse(avatar.adjusted(half, half, -half, -half));
			}
		}

		painter.restore();
	}
}

} // namespace sspot
