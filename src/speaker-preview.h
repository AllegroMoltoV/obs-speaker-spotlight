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

#include "core/portrait.h"
#include "core/voice-style.h"

#include <QImage>
#include <QSize>
#include <QWidget>

#include <optional>

namespace sspot {

// 待機中と発話中の見た目を横に並べて描くプレビュー。ソースへの反映を待たずに、設定の操作のたびに描き直す。
// StreamKit のページを描くのではなく、生成する CSS が指定する値を Qt で近似する
class SpeakerPreview : public QWidget {
	Q_OBJECT

public:
	explicit SpeakerPreview(QWidget *parent = nullptr);

	void set_look(const core::VoiceLook &look);
	// 画像の復号はここで 1 回だけ行う。CSS に埋め込めない立ち絵は、CSS と同じくアバターのまま描く
	void set_portrait(const std::optional<core::PortraitImage> &portrait);

	bool hasHeightForWidth() const override { return true; }
	int heightForWidth(int width) const override;
	QSize sizeHint() const override;
	QSize minimumSizeHint() const override;

protected:
	void paintEvent(QPaintEvent *event) override;

private:
	struct Metrics {
		QSize avatar; // 枠を含むアバターの外形 (CSS の px)
		int border;   // 枠の幅
		QSize panel;  // 1 つの状態の外形
		int caption;  // 見出しの高さ (画面の px)
		int total_width;
	};

	Metrics measure() const;
	// ドックの幅に収まるよう全体を縮小する倍率。拡大はしない
	static double scale_for(const Metrics &l, int width);
	// 描く大きさに縮小し、明るさを掛けた立ち絵。大きさと明るさが同じ間は作り直さない
	const QImage &portrait_image(const QSize &size, int percent, bool speaking);

	core::VoiceLook look_;
	QImage portrait_; // 復号した立ち絵。無いとき、使えないときは null
	QImage scaled_;   // portrait_ を描く大きさに縮小したもの
	struct Brightened {
		QImage image;
		int percent = -1;
	};
	Brightened idle_;
	Brightened speaking_;
};

} // namespace sspot
